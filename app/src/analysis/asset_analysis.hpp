#pragma once

#include "analysis/analysis_settings.hpp"

#include <zpew/data/snapshot.hpp>
#include <zpew/evaluation/asset_evaluation.hpp>
#include <zpew/evaluation/improvement.hpp>
#include <zpew/var/backtest.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace zpew::app {

struct ModelAnalysis {
    std::string name;
    std::optional<std::string> failure;
    std::vector<double> volatility_percent;
    std::vector<double> var_price;
    std::vector<double> breach_times;
    std::vector<double> breach_prices;
    std::optional<evaluation::ModelLosses> losses;
    std::optional<evaluation::Improvement> squared_return_improvement;
    std::optional<evaluation::Improvement> parkinson_improvement;
    std::optional<var::BacktestResult> backtest;
    std::size_t refits{};
    std::size_t boundary_refits{};
};

struct AssetAnalysis {
    std::string symbol;
    AnalysisSettings settings;
    std::string baseline;
    std::vector<double> times;
    std::vector<ModelAnalysis> models;
    double seconds{};
};

[[nodiscard]] AssetAnalysis analyse_asset(const data::Snapshot& snapshot,
                                          const AnalysisSettings& settings);

}
