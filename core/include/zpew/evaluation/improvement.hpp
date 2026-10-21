#pragma once

#include <zpew/evaluation/asset_evaluation.hpp>
#include <zpew/evaluation/rolling_forecast.hpp>

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace zpew::evaluation {

struct Improvement {
    std::string symbol;
    WindowMode mode{WindowMode::kSliding};
    std::string model;
    std::string proxy;
    double mse_percent{};
    double qlike_percent{};
};

struct PooledImprovement {
    WindowMode mode{WindowMode::kSliding};
    std::string model;
    std::string proxy;
    std::string metric;
    std::size_t assets{};
    double mean_percent{};
    double median_percent{};
    std::size_t wins{};
};

[[nodiscard]] std::vector<Improvement> improvements_over_baseline(const AssetEvaluation& asset,
                                                                  std::string_view baseline);

[[nodiscard]] std::vector<PooledImprovement>
pool_improvements(std::span<const Improvement> improvements);

[[nodiscard]] double median(std::vector<double> values);

}
