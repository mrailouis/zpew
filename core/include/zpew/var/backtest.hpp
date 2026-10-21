#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace zpew::var {

inline constexpr double kDefaultConfidence = 0.95;

struct KupiecTest {
    double likelihood_ratio{};
    double p_value{};
};

struct BacktestResult {
    double confidence{};
    std::size_t observations{};
    std::size_t breaches{};
    double breach_rate{};
    double expected_rate{};
    KupiecTest kupiec;
};

[[nodiscard]] double value_at_risk(double variance, double confidence);
[[nodiscard]] std::vector<double> value_at_risk_series(std::span<const double> variances,
                                                       double confidence);
[[nodiscard]] KupiecTest kupiec_test(std::size_t observations, std::size_t breaches,
                                     double expected_rate);
[[nodiscard]] BacktestResult backtest(std::span<const double> variances,
                                      std::span<const double> returns, double confidence);

}
