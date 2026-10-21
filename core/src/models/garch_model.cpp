#include <zpew/error.hpp>
#include <zpew/models/garch_model.hpp>
#include <zpew/models/garch_params.hpp>
#include <zpew/optimiser/nelder_mead.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace zpew::models {

namespace {

constexpr std::array<double, 3> kStartPersistence{0.90, 0.95, 0.98};
constexpr std::array<double, 3> kStartAlpha{0.03, 0.08, 0.15};

const optimiser::NelderMeadOptions kOptimiserOptions{
    .initial_step = 0.25, .x_tolerance = 1e-9, .f_tolerance = 1e-13, .max_iterations = 10000};

struct Candidate {
    GarchParams params;
    double log_likelihood;
    int iterations;
};

void require_finite(std::span<const double> returns) {
    for (const double r : returns) {
        if (!std::isfinite(r)) {
            throw InvalidArgument{std::format("return {} must be finite", r)};
        }
    }
}

GarchParams targeted(double alpha, double beta, double backcast) {
    return {.omega = backcast * (1.0 - alpha - beta), .alpha = alpha, .beta = beta};
}

std::optional<Candidate> optimise_from(const GarchParams& start, std::span<const double> returns,
                                       double backcast, bool variance_targeting) {
    const GarchTheta theta = theta_from_params(start);
    const auto negative_log_likelihood = [&](std::span<const double> x) {
        const std::array<double, 3> full = variance_targeting
                                               ? std::array<double, 3>{0.0, x[0], x[1]}
                                               : std::array<double, 3>{x[0], x[1], x[2]};
        GarchParams params = params_from_theta(full);
        if (variance_targeting) {
            params = targeted(params.alpha, params.beta, backcast);
        }
        return -garch_log_likelihood(returns, params, backcast);
    };
    const std::vector<double> x0 = variance_targeting
                                       ? std::vector<double>{theta[1], theta[2]}
                                       : std::vector<double>(theta.begin(), theta.end());
    const optimiser::NelderMeadResult result =
        optimiser::nelder_mead(negative_log_likelihood, x0, kOptimiserOptions);
    if (!result.converged || !std::isfinite(result.f)) {
        return std::nullopt;
    }
    const std::array<double, 3> full =
        variance_targeting ? std::array<double, 3>{0.0, result.x[0], result.x[1]}
                           : std::array<double, 3>{result.x[0], result.x[1], result.x[2]};
    GarchParams params = params_from_theta(full);
    if (variance_targeting) {
        params = targeted(params.alpha, params.beta, backcast);
    }
    return Candidate{
        .params = params, .log_likelihood = -result.f, .iterations = result.iterations};
}

}

GarchModel::GarchModel(GarchOptions options) : options_{options} {
    if (options.minimum_fit_length < 2) {
        throw InvalidArgument{"GARCH minimum fit length must be at least 2"};
    }
}

void GarchModel::fit(std::span<const double> returns) {
    if (returns.size() < options_.minimum_fit_length) {
        throw InvalidArgument{std::format("GARCH needs at least {} returns to fit, got {}",
                                          options_.minimum_fit_length, returns.size())};
    }
    require_finite(returns);
    const double backcast = mean_square(returns);
    if (!(backcast > 0.0)) {
        throw InvalidArgument{"GARCH fit sample contains only zero returns"};
    }

    std::optional<Candidate> best;
    int converged_starts = 0;
    int iterations = 0;
    for (const double persistence : kStartPersistence) {
        for (const double alpha : kStartAlpha) {
            const GarchParams start = targeted(alpha, persistence - alpha, backcast);
            const auto candidate =
                optimise_from(start, returns, backcast, options_.variance_targeting);
            if (!candidate) {
                continue;
            }
            ++converged_starts;
            iterations += candidate->iterations;
            if (!best || candidate->log_likelihood > best->log_likelihood) {
                best = candidate;
            }
        }
    }
    const int starts = static_cast<int>(kStartPersistence.size() * kStartAlpha.size());
    if (!best) {
        throw ConvergenceError{
            std::format("GARCH fit did not converge from any of {} starts", starts)};
    }
    if (best->params.persistence() > kGarchMaxPersistence) {
        throw ConvergenceError{
            std::format("GARCH fit is at the integrated boundary: alpha + beta = {}",
                        best->params.persistence())};
    }

    double variance = best->params.omega + (best->params.persistence() * backcast);
    for (const double r : returns) {
        variance = garch_next_variance(best->params, variance, r);
    }
    fit_ = GarchFit{.params = best->params,
                    .log_likelihood = best->log_likelihood,
                    .backcast = backcast,
                    .observations = returns.size(),
                    .starts = starts,
                    .converged_starts = converged_starts,
                    .iterations = iterations};
    variance_ = variance;
}

double GarchModel::forecast_variance() const {
    if (!variance_) {
        throw Error{"GARCH forecast requested before fit"};
    }
    return *variance_;
}

void GarchModel::update(double new_return) {
    if (!variance_ || !fit_) {
        throw Error{"GARCH update before fit"};
    }
    if (!std::isfinite(new_return)) {
        throw InvalidArgument{std::format("return {} must be finite", new_return)};
    }
    variance_ = garch_next_variance(fit_->params, *variance_, new_return);
}

std::string GarchModel::name() const {
    return options_.variance_targeting ? "GARCH(1,1, variance targeting)" : "GARCH(1,1)";
}

const GarchFit& GarchModel::fitted() const {
    if (!fit_) {
        throw Error{"GARCH parameters requested before fit"};
    }
    return *fit_;
}

}
