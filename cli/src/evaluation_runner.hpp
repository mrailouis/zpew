#pragma once

#include "eval_config.hpp"

#include <zpew/data/manifest.hpp>
#include <zpew/data/snapshot_data_source.hpp>
#include <zpew/evaluation/asset_evaluation.hpp>
#include <zpew/evaluation/rolling_forecast.hpp>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace zpew::cli {

struct EvaluationJob {
    std::string symbol;
    evaluation::WindowMode mode{evaluation::WindowMode::kSliding};
    std::optional<evaluation::AssetEvaluation> result;
    std::string failure;
};

struct RunRecord {
    std::string command;
    std::string started_at;
    double seconds{};
    std::size_t failures{};
};

[[nodiscard]] std::vector<std::string> selected_symbols(const EvalConfig& config,
                                                        const data::Manifest& manifest);

[[nodiscard]] std::vector<EvaluationJob> run_evaluations(const EvalConfig& config,
                                                         const data::SnapshotDataSource& source,
                                                         const std::vector<std::string>& symbols,
                                                         std::ostream& progress);

[[nodiscard]] std::string utc_now();

void write_run_json(const std::filesystem::path& directory, const EvalConfig& config,
                    const data::Manifest& manifest, const std::vector<std::string>& symbols,
                    const RunRecord& record);

void warn_if_dirty(std::ostream& out);

}
