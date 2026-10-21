#include <zpew/error.hpp>
#include <zpew/models/garch_params.hpp>

#include <cmath>
#include <format>
#include <limits>
#include <span>

namespace zpew::models {

GarchParams params_from_theta(std::span<const double, 3> theta) noexcept {
    const double e1 = std::exp(theta[1]);
    const double e2 = std::exp(theta[2]);
    const double denominator = 1.0 + e1 + e2;
    return {.omega = std::exp(theta[0]), .alpha = e1 / denominator, .beta = e2 / denominator};
}

GarchTheta theta_from_params(const GarchParams& params) {
    const double slack = 1.0 - params.alpha - params.beta;
    const bool valid = params.omega > 0.0 && params.alpha > 0.0 && params.beta > 0.0 &&
                       slack > 0.0 && std::isfinite(params.omega);
    if (!valid) {
        throw InvalidArgument{
            std::format("GARCH parameters need omega > 0, alpha > 0, beta > 0 and alpha + beta "
                        "< 1; got omega {}, alpha {}, beta {}",
                        params.omega, params.alpha, params.beta)};
    }
    return {std::log(params.omega), std::log(params.alpha / slack), std::log(params.beta / slack)};
}

double mean_square(std::span<const double> returns) {
    if (returns.empty()) {
        throw InvalidArgument{"mean square of an empty sample"};
    }
    double sum = 0.0;
    for (const double r : returns) {
        sum += r * r;
    }
    return sum / static_cast<double>(returns.size());
}

double garch_next_variance(const GarchParams& params, double variance, double r) noexcept {
    return params.omega + (params.alpha * (r * r)) + (params.beta * variance);
}

double garch_log_likelihood(std::span<const double> returns, const GarchParams& params,
                            double backcast) {
    double variance = params.omega + ((params.alpha + params.beta) * backcast);
    double sum = 0.0;
    for (const double r : returns) {
        if (!(variance > 0.0) || !std::isfinite(variance)) {
            return -std::numeric_limits<double>::infinity();
        }
        sum += std::log(variance) + ((r * r) / variance);
        variance = garch_next_variance(params, variance, r);
    }
    return -0.5 * sum;
}

}
