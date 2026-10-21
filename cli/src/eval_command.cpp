#include "eval_command.hpp"

#include "build_info.hpp"
#include "eval_config.hpp"
#include "exit_code.hpp"
#include "json_text.hpp"
#include "options.hpp"
#include "parallel.hpp"
#include "results_writer.hpp"

#include <zpew/data/manifest.hpp>
#include <zpew/data/snapshot.hpp>
#include <zpew/data/snapshot_data_source.hpp>
#include <zpew/error.hpp>
#include <zpew/evaluation/asset_evaluation.hpp>
#include <zpew/evaluation/improvement.hpp>
#include <zpew/evaluation/rolling_forecast.hpp>
#include <zpew/models/model_spec.hpp>
#include <zpew/version.hpp>

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <mutex>
#include <optional>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace zpew::cli {

namespace {

struct Job {
    std::string symbol;
    evaluation::WindowMode mode;
};

struct JobResult {
    std::optional<evaluation::AssetEvaluation> evaluation;
    std::string failure;
    double seconds{};
};

std::vector<std::string> selected_symbols(const EvalConfig& config,
                                          const data::Manifest& manifest) {
    std::vector<std::string> symbols;
    if (config.assets.empty()) {
        for (const auto& asset : manifest.assets()) {
            symbols.push_back(asset.symbol);
        }
        return symbols;
    }
    for (const std::string& symbol : config.assets) {
        static_cast<void>(manifest.find(symbol));
        symbols.push_back(symbol);
    }
    return symbols;
}

std::string utc_now() {
    return std::format("{:%Y-%m-%dT%H:%M:%SZ}",
                       std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
}

std::string model_json(const models::ModelSpec& spec) {
    return std::visit(
        [&](const auto& p) -> std::string {
            using Parameters = std::decay_t<decltype(p)>;
            if constexpr (std::is_same_v<Parameters, models::RealisedSpec>) {
                return std::format(R"({{"name": {}, "type": "realised", "window": {}}})",
                                   json_string(spec.name), p.window);
            } else if constexpr (std::is_same_v<Parameters, models::EwmaSpec>) {
                return std::format(
                    R"({{"name": {}, "type": "ewma", "lambda": {}, "seed_window": {}}})",
                    json_string(spec.name), p.lambda, p.seed_window);
            } else {
                return std::format(R"({{"name": {}, "type": "garch", "variance_targeting": {}}})",
                                   json_string(spec.name), p.variance_targeting);
            }
        },
        spec.parameters);
}

void write_run_json(const EvalConfig& config, const data::Manifest& manifest,
                    const std::vector<std::string>& symbols, const std::string& started,
                    double seconds, std::size_t failures) {
    const BuildInfo build = build_info();
    std::string modes;
    for (const auto mode : config.window_modes) {
        modes += (modes.empty() ? "" : ", ") + json_string(evaluation::to_string(mode));
    }
    std::string model_list;
    for (const auto& spec : config.models) {
        model_list += std::format("{}    {}", model_list.empty() ? "" : ",\n", model_json(spec));
    }
    std::string snapshots;
    for (const std::string& symbol : symbols) {
        const auto& asset = manifest.find(symbol);
        snapshots +=
            std::format(R"({}    {{"symbol": {}, "file": {}, "sha256": {}}})",
                        snapshots.empty() ? "" : ",\n", json_string(symbol),
                        json_string(asset.file.generic_string()), json_string(asset.sha256));
    }
    std::ofstream out{config.run_directory() / "run.json", std::ios::binary | std::ios::trunc};
    out << std::format("{{\n"
                       "  \"name\": {},\n"
                       "  \"zpew_version\": {},\n"
                       "  \"git_commit\": {},\n"
                       "  \"git_dirty\": {},\n"
                       "  \"compiler\": {},\n"
                       "  \"config_file\": {},\n"
                       "  \"config_sha256\": {},\n"
                       "  \"started_at\": {},\n"
                       "  \"duration_seconds\": {:.1f},\n"
                       "  \"window\": {},\n"
                       "  \"refit_every\": {},\n"
                       "  \"window_modes\": [{}],\n"
                       "  \"baseline\": {},\n"
                       "  \"failures\": {},\n"
                       "  \"models\": [\n{}\n  ],\n"
                       "  \"snapshots\": [\n{}\n  ]\n"
                       "}}\n",
                       json_string(config.name), json_string(version()), json_string(build.commit),
                       build.dirty, json_string(build.compiler),
                       json_string(config.path.generic_string()), json_string(config.sha256),
                       json_string(started), seconds, config.window, config.refit_every, modes,
                       json_string(config.baseline), failures, model_list, snapshots);
}

}

int run_eval(const Options& options, std::ostream& out, std::ostream& err) {
    EvalConfig config = load_eval_config(options.require("config"));
    if (const auto output = options.get("output")) {
        config.output = std::filesystem::path{*output};
    }
    const data::SnapshotDataSource source{config.manifest};
    const std::vector<std::string> symbols = selected_symbols(config, source.manifest());
    const std::string started = utc_now();
    const auto start = std::chrono::steady_clock::now();

    std::vector<Job> jobs;
    for (const auto mode : config.window_modes) {
        for (const std::string& symbol : symbols) {
            jobs.push_back({.symbol = symbol, .mode = mode});
        }
    }
    std::vector<JobResult> results(jobs.size());
    std::mutex progress_mutex;
    std::size_t finished = 0;
    run_parallel(jobs.size(), config.threads, [&](std::size_t i) {
        const auto job_start = std::chrono::steady_clock::now();
        JobResult result;
        try {
            const data::Snapshot snapshot = source.load_snapshot(jobs[i].symbol);
            result.evaluation = evaluation::evaluate_asset(
                snapshot, config.models,
                {.window = config.window, .refit_every = config.refit_every, .mode = jobs[i].mode});
        } catch (const Error& error) {
            result.failure = error.what();
        }
        result.seconds =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - job_start).count();
        const std::scoped_lock lock{progress_mutex};
        ++finished;
        err << std::format("[{}/{}] {} {} {} ({:.1f} s)\n", finished, jobs.size(), jobs[i].symbol,
                           evaluation::to_string(jobs[i].mode),
                           result.failure.empty() ? "done" : "FAILED", result.seconds);
        results[i] = std::move(result);
    });

    const std::filesystem::path run = config.run_directory();
    std::filesystem::remove_all(run);
    std::vector<evaluation::AssetEvaluation> evaluations;
    std::vector<evaluation::Improvement> improvements;
    std::vector<AssetFailure> failures;
    for (std::size_t i = 0; i < jobs.size(); ++i) {
        const std::string mode{evaluation::to_string(jobs[i].mode)};
        const std::optional<evaluation::AssetEvaluation>& evaluated = results[i].evaluation;
        if (!evaluated) {
            failures.push_back({.symbol = jobs[i].symbol,
                                .mode = mode,
                                .model = "*",
                                .reason = results[i].failure});
            continue;
        }
        const evaluation::AssetEvaluation& asset = *evaluated;
        write_forecasts(run / "forecasts", asset);
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
    const auto pooled = evaluation::pool_improvements(improvements);
    write_losses(run / "losses.csv", evaluations);
    write_improvements(run / "improvements.csv", improvements);
    write_pooled(run / "pooled.csv", pooled);
    write_failures(run / "failures.csv", failures);
    const double seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    write_run_json(config, source.manifest(), symbols, started, seconds, failures.size());

    out << std::format("evaluated {} assets x {} window modes in {:.1f} s; {} failures; results "
                       "in {}\n",
                       symbols.size(), config.window_modes.size(), seconds, failures.size(),
                       run.generic_string());
    if (build_info().dirty) {
        out << "warning: built from a working tree with uncommitted changes; do not publish "
               "these results\n";
    }
    return kExitSuccess;
}

}
