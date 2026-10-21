#include <zpew/error.hpp>
#include <zpew/returns/rolling_mean.hpp>

#include <cmath>
#include <cstddef>
#include <format>
#include <numeric>

namespace zpew::returns {

RollingMean::RollingMean(std::size_t window) {
    if (window == 0) {
        throw InvalidArgument{"rolling window must be at least 1"};
    }
    values_.resize(window, 0.0);
}

void RollingMean::push(double value) {
    if (!std::isfinite(value) || value < 0.0) {
        throw InvalidArgument{
            std::format("rolling mean input {} must be finite and non-negative", value)};
    }
    const double departing = values_[next_];
    values_[next_] = value;
    next_ = (next_ + 1) % values_.size();
    if (count_ < values_.size()) {
        ++count_;
    }

    sum_ += value - departing;
    ++updates_since_recompute_;
    if (updates_since_recompute_ >= values_.size() || departing > sum_) {
        recompute_sum();
    }
}

std::size_t RollingMean::window() const noexcept {
    return values_.size();
}

bool RollingMean::full() const noexcept {
    return count_ == values_.size();
}

double RollingMean::mean() const {
    if (!full()) {
        throw Error{std::format("rolling window of {} has only {} values", values_.size(), count_)};
    }
    return sum_ / static_cast<double>(values_.size());
}

void RollingMean::recompute_sum() noexcept {
    sum_ = std::accumulate(values_.begin(), values_.end(), 0.0);
    updates_since_recompute_ = 0;
}

}
