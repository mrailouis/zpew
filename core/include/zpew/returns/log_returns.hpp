#pragma once

#include <zpew/data/bar.hpp>

#include <span>
#include <vector>

namespace zpew::returns {

[[nodiscard]] std::vector<double> log_returns(std::span<const double> closes);
[[nodiscard]] std::vector<double> log_returns(std::span<const data::Bar> bars);

}
