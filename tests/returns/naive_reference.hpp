#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace zpew::test {

inline std::vector<double> naive_rolling_mean_of_squares(std::span<const double> values,
                                                         std::size_t window) {
    std::vector<double> means;
    for (std::size_t end = window; end <= values.size(); ++end) {
        double sum = 0.0;
        for (std::size_t i = end - window; i < end; ++i) {
            sum += values[i] * values[i];
        }
        means.push_back(sum / static_cast<double>(window));
    }
    return means;
}

}
