#pragma once

#include <cstddef>
#include <vector>

namespace zpew::returns {

class RollingMeanOfSquares {
public:
    explicit RollingMeanOfSquares(std::size_t window);

    void push(double value);

    [[nodiscard]] std::size_t window() const noexcept;
    [[nodiscard]] bool full() const noexcept;
    [[nodiscard]] double mean() const;

private:
    void recompute_sum() noexcept;

    std::vector<double> squares_;
    std::size_t next_ = 0;
    std::size_t count_ = 0;
    std::size_t updates_since_recompute_ = 0;
    double sum_ = 0.0;
};

}
