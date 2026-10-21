#include "eval_command.hpp"

#include "eval_config.hpp"
#include "evaluation_runner.hpp"
#include "exit_code.hpp"
#include "options.hpp"
#include "results_writer.hpp"

#include <zpew/data/snapshot_data_source.hpp>
#include <zpew/error.hpp>
#include <zpew/evaluation/asset_evaluation.hpp>
#include <zpew/evaluation/improvement.hpp>
#include <zpew/evaluation/rolling_forecast.hpp>

#include <chrono>
#include <filesystem>
#include <format>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace zpew::cli {

int run_eval(const Options& options, std::ostream& out, std::ostream& err) {
    EvalConfig config = load_eval_config(options.require("config"));
    if (const auto output = options.get("output")) {
        config.output = std::filesystem::path{*output};
    }
    const data::SnapshotDataSource source{config.manifest};
    const std::vector<std::string> symbols = selected_symbols(config, source.manifest());
    const std::string started = utc_now();
    const auto start = std::chrono::steady_clock::now();
    const std::vector<EvaluationJob> jobs = run_evaluations(config, source, symbols, err);

    const std::filesystem::path directory = config.run_directory() / "eval";
    std::filesystem::remove_all(directory);
    std::vector<evaluation::AssetEvaluation> evaluations;
    std::vector<evaluation::Improvement> improvements;
    std::vector<AssetFailure> failures;
    for (const EvaluationJob& job : jobs) {
        const std::string mode{evaluation::to_string(job.mode)};
        const std::optional<evaluation::AssetEvaluation>& evaluated = job.result;
        if (!evaluated) {
            failures.push_back(
                {.symbol = job.symbol, .mode = mode, .model = "*", .reason = job.failure});
            continue;
        }
        const evaluation::AssetEvaluation& asset = *evaluated;
        write_forecasts(directory / "forecasts", asset);
        for (const auto& model : asset.models) {
            if (model.failure) {
                failures.push_back({.symbol = asset.symbol,
                                    .mode = mode,
                                    .model = model.model,
                                    .reason = *model.failure});
            }
        }
        try {
            const auto asset_improvements =
                evaluation::improvements_over_baseline(asset, config.baseline);
            improvements.insert(improvements.end(), asset_improvements.begin(),
                                asset_improvements.end());
        } catch (const Error& error) {
            failures.push_back({.symbol = asset.symbol,
                                .mode = mode,
                                .model = "improvement",
                                .reason = error.what()});
        }
        evaluations.push_back(asset);
    }
    write_losses(directory / "losses.csv", evaluations);
    write_improvements(directory / "improvements.csv", improvements);
    write_pooled(directory / "pooled.csv", evaluation::pool_improvements(improvements));
    write_failures(directory / "failures.csv", failures);
    const double seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    write_run_json(directory, config, source.manifest(), symbols,
                   {.command = "eval",
                    .started_at = started,
                    .seconds = seconds,
                    .failures = failures.size()});

    out << std::format("evaluated {} assets x {} window modes in {:.1f} s; {} failures; results "
                       "in {}\n",
                       symbols.size(), config.window_modes.size(), seconds, failures.size(),
                       directory.generic_string());
    warn_if_dirty(out);
    return kExitSuccess;
}

}
