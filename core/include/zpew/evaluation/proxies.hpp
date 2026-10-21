#pragma once

#include <zpew/data/bar.hpp>

#include <span>
#include <vector>

namespace zpew::evaluation {

[[nodiscard]] std::vector<double> squared_return_proxy(std::span<const double> returns);
[[nodiscard]] std::vector<double> parkinson_proxy(std::span<const data::Bar> bars);

}
