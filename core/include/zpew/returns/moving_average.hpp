#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace zpew::returns {

[[nodiscard]] std::vector<double> simple_moving_average(std::span<const double> values,
                                                        std::size_t window);

}
