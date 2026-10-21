#include <zpew/returns/moving_average.hpp>
#include <zpew/returns/rolling_mean.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace zpew::returns {

std::vector<double> simple_moving_average(std::span<const double> values, std::size_t window) {
    RollingMean rolling{window};
    std::vector<double> averages;
    if (values.size() >= window) {
        averages.reserve(values.size() - window + 1);
    }
    for (const double value : values) {
        rolling.push(value);
        if (rolling.full()) {
            averages.push_back(rolling.mean());
        }
    }
    return averages;
}

}
