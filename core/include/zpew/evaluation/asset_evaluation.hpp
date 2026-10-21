#pragma once

#include <zpew/data/snapshot.hpp>
#include <zpew/evaluation/losses.hpp>
#include <zpew/evaluation/rolling_forecast.hpp>
#include <zpew/models/model_spec.hpp>

#include <chrono>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace zpew::evaluation {

struct ModelLosses {
    LossSummary squared_return;
    LossSummary parkinson;
};

struct ModelEvaluation {
    std::string model;
    std::vector<double> forecasts;
    std::size_t refits{};
    std::size_t boundary_refits{};
    std::optional<std::string> failure;
    std::optional<ModelLosses> losses;
};

struct AssetEvaluation {
    std::string symbol;
    WindowMode mode{WindowMode::kSliding};
    std::vector<std::chrono::sys_days> dates;
    std::vector<double> returns;
    std::vector<double> squared_return_proxy;
    std::vector<double> parkinson_proxy;
    std::vector<ModelEvaluation> models;
};

[[nodiscard]] AssetEvaluation evaluate_asset(const data::Snapshot& snapshot,
                                             std::span<const models::ModelSpec> specs,
                                             const RollingOptions& options);

}
