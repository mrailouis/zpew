#pragma once

#include "eval_config.hpp"

#include <zpew/evaluation/asset_evaluation.hpp>
#include <zpew/evaluation/improvement.hpp>
#include <zpew/var/pooled_backtest.hpp>

#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace zpew::cli {

struct AssetFailure {
    std::string symbol;
    std::string mode;
    std::string model;
    std::string reason;
};

void write_forecasts(const std::filesystem::path& directory,
                     const evaluation::AssetEvaluation& asset);
void write_losses(const std::filesystem::path& file,
                  std::span<const evaluation::AssetEvaluation> assets);
void write_improvements(const std::filesystem::path& file,
                        std::span<const evaluation::Improvement> improvements);
void write_pooled(const std::filesystem::path& file,
                  std::span<const evaluation::PooledImprovement> pooled);
void write_failures(const std::filesystem::path& file, std::span<const AssetFailure> failures);
void write_var_series(const std::filesystem::path& directory,
                      const evaluation::AssetEvaluation& asset,
                      std::span<const double> confidences);
void write_backtests(const std::filesystem::path& file,
                     std::span<const var::AssetBacktest> backtests);
void write_pooled_backtests(const std::filesystem::path& file,
                            std::span<const var::PooledBacktest> pooled);

}
