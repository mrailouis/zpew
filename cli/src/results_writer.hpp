#pragma once

#include "eval_config.hpp"

#include <zpew/evaluation/asset_evaluation.hpp>
#include <zpew/evaluation/improvement.hpp>

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

}
