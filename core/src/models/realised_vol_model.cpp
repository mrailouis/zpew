#include <zpew/error.hpp>
#include <zpew/models/realised_vol_model.hpp>
#include <zpew/returns/rolling_mean.hpp>

#include <cmath>
#include <cstddef>
#include <format>
#include <span>
#include <string>

namespace zpew::models {

namespace {

double checked_square(double r) {
    if (!std::isfinite(r)) {
        throw InvalidArgument{std::format("return {} must be finite", r)};
    }
    return r * r;
}

}

RealisedVolModel::RealisedVolModel(std::size_t window) : window_{window} {
    if (window == 0) {
        throw InvalidArgument{"realised volatility window must be at least 1"};
    }
}

void RealisedVolModel::fit(std::span<const double> returns) {
    if (returns.size() < window_) {
        throw InvalidArgument{std::format("realised volatility needs at least {} returns, got {}",
                                          window_, returns.size())};
    }
    returns::RollingMean squares{window_};
    for (const double r : returns) {
        squares.push(checked_square(r));
    }
    squares_ = squares;
}

double RealisedVolModel::forecast_variance() const {
    if (!squares_) {
        throw Error{"realised volatility forecast requested before fit"};
    }
    const double variance = squares_->mean();
    if (!(variance > 0.0)) {
        throw Error{std::format("realised variance over the last {} returns is zero", window_)};
    }
    return variance;
}

void RealisedVolModel::update(double new_return) {
    if (!squares_) {
        throw Error{"realised volatility update before fit"};
    }
    squares_->push(checked_square(new_return));
}

std::string RealisedVolModel::name() const {
    return std::format("RealisedVol(window={})", window_);
}

std::size_t RealisedVolModel::window() const noexcept {
    return window_;
}

}
