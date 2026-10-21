#include <zpew/error.hpp>
#include <zpew/returns/rolling_mean_of_squares.hpp>

#include <cmath>
#include <cstddef>
#include <format>
#include <numeric>

namespace zpew::returns {

RollingMeanOfSquares::RollingMeanOfSquares(std::size_t window) {
    if (window == 0) {
        throw InvalidArgument{"rolling window must be at least 1"};
    }
    squares_.resize(window, 0.0);
}

void RollingMeanOfSquares::push(double value) {
    if (!std::isfinite(value)) {
        throw InvalidArgument{std::format("rolling input {} must be finite", value)};
    }
    const double square = value * value;
    const double departing = squares_[next_];
    squares_[next_] = square;
    next_ = (next_ + 1) % squares_.size();
    if (count_ < squares_.size()) {
        ++count_;
    }

    sum_ += square - departing;
    ++updates_since_recompute_;
    if (updates_since_recompute_ >= squares_.size() || departing > sum_) {
        recompute_sum();
    }
}

std::size_t RollingMeanOfSquares::window() const noexcept {
    return squares_.size();
}

bool RollingMeanOfSquares::full() const noexcept {
    return count_ == squares_.size();
}

double RollingMeanOfSquares::mean() const {
    if (!full()) {
        throw Error{
            std::format("rolling window of {} has only {} values", squares_.size(), count_)};
    }
    return sum_ / static_cast<double>(squares_.size());
}

void RollingMeanOfSquares::recompute_sum() noexcept {
    sum_ = std::accumulate(squares_.begin(), squares_.end(), 0.0);
    updates_since_recompute_ = 0;
}

}
