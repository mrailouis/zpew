#pragma once

#include <zpew/evaluation/rolling_forecast.hpp>
#include <zpew/var/backtest.hpp>

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace zpew::var {

inline constexpr double kKupiecSignificance = 0.05;

struct AssetBacktest {
    std::string symbol;
    evaluation::WindowMode mode{evaluation::WindowMode::kSliding};
    std::string model;
    BacktestResult result;
};

struct PooledBacktest {
    evaluation::WindowMode mode{evaluation::WindowMode::kSliding};
    std::string model;
    double confidence{};
    std::size_t assets{};
    std::size_t observations{};
    std::size_t breaches{};
    double pooled_breach_rate{};
    double mean_breach_rate{};
    double median_breach_rate{};
    std::size_t kupiec_rejections{};
};

[[nodiscard]] std::vector<PooledBacktest> pool_backtests(std::span<const AssetBacktest> backtests);

}
