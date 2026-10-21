#pragma once

#include "request_settings.hpp"

#include <zpew/data/snapshot.hpp>
#include <zpew/evaluation/asset_evaluation.hpp>
#include <zpew/evaluation/improvement.hpp>
#include <zpew/var/backtest.hpp>

#include <cstddef>
#include <napi.h>
#include <optional>
#include <string>
#include <vector>

namespace zpew::native {

struct ModelData {
    std::string name;
    std::optional<std::string> failure;
    std::size_t refits{};
    std::size_t boundary_refits{};
    std::vector<double> volatility_percent;
    std::vector<double> var_price;
    std::vector<double> breach_times;
    std::vector<double> breach_prices;
    std::optional<evaluation::ModelLosses> losses;
    std::optional<evaluation::Improvement> squared_return_improvement;
    std::optional<evaluation::Improvement> parkinson_improvement;
    std::optional<var::BacktestResult> backtest;
};

struct AnalysisData {
    std::string symbol;
    std::string baseline;
    AnalysisSettings settings;
    std::vector<double> times;
    std::vector<ModelData> models;
    double seconds{};
};

[[nodiscard]] AnalysisData build_analysis_data(const data::Snapshot& snapshot,
                                               const AnalysisSettings& settings);

[[nodiscard]] Napi::Value analyse(const Napi::CallbackInfo& info);

}
