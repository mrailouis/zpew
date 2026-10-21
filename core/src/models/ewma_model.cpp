#include <zpew/error.hpp>
#include <zpew/models/ewma_model.hpp>

#include <cmath>
#include <cstddef>
#include <format>
#include <span>
#include <string>

namespace zpew::models {

namespace {

void require_finite(double r) {
    if (!std::isfinite(r)) {
        throw InvalidArgument{std::format("return {} must be finite", r)};
    }
}

}

EwmaModel::EwmaModel(double lambda, std::size_t seed_window)
    : lambda_{lambda}, seed_window_{seed_window} {
    const bool in_open_unit_interval = lambda > 0.0 && lambda < 1.0;
    if (!in_open_unit_interval) {
        throw InvalidArgument{std::format("EWMA lambda must be in (0, 1), got {}", lambda)};
    }
    if (seed_window == 0) {
        throw InvalidArgument{"EWMA seed window must be at least 1"};
    }
}

double EwmaModel::next_variance(double variance, double r) const noexcept {
    return (lambda_ * variance) + ((1.0 - lambda_) * (r * r));
}

void EwmaModel::fit(std::span<const double> returns) {
    if (returns.size() < seed_window_) {
        throw InvalidArgument{std::format("EWMA needs at least {} returns to fit, got {}",
                                          seed_window_, returns.size())};
    }
    double sum_of_squares = 0.0;
    for (const double r : returns.first(seed_window_)) {
        require_finite(r);
        sum_of_squares += r * r;
    }
    double variance = sum_of_squares / static_cast<double>(seed_window_);
    if (!(variance > 0.0)) {
        throw InvalidArgument{"EWMA seed window contains only zero returns"};
    }
    for (const double r : returns.subspan(seed_window_)) {
        require_finite(r);
        variance = next_variance(variance, r);
    }
    if (!(variance > 0.0)) {
        throw Error{"EWMA variance underflowed to zero during fit"};
    }
    variance_ = variance;
}

double EwmaModel::forecast_variance() const {
    if (!variance_) {
        throw Error{"EWMA forecast requested before fit"};
    }
    return *variance_;
}

void EwmaModel::update(double new_return) {
    if (!variance_) {
        throw Error{"EWMA update before fit"};
    }
    require_finite(new_return);
    const double variance = next_variance(*variance_, new_return);
    if (!(variance > 0.0) || !std::isfinite(variance)) {
        throw Error{std::format("EWMA variance left the positive finite range: {}", variance)};
    }
    variance_ = variance;
}

std::string EwmaModel::name() const {
    return std::format("EWMA(lambda={}, seed={})", lambda_, seed_window_);
}

double EwmaModel::lambda() const noexcept {
    return lambda_;
}

std::size_t EwmaModel::seed_window() const noexcept {
    return seed_window_;
}

}
