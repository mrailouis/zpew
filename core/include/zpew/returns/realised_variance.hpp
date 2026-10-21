#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace zpew::returns {

inline constexpr double kTradingDaysPerYear = 252.0;

[[nodiscard]] std::vector<double> rolling_realised_variance(std::span<const double> returns,
                                                            std::size_t window);

[[nodiscard]] double annualise_volatility(double daily_variance,
                                          double periods_per_year = kTradingDaysPerYear);

}
