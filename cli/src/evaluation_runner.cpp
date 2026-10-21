#include "evaluation_runner.hpp"

#include "build_info.hpp"
#include "eval_config.hpp"
#include "json_text.hpp"
#include "parallel.hpp"

#include <zpew/data/manifest.hpp>
#include <zpew/data/snapshot.hpp>
#include <zpew/data/snapshot_data_source.hpp>
#include <zpew/error.hpp>
#include <zpew/evaluation/asset_evaluation.hpp>
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
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace zpew::cli {

namespace {

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

template <typename T, typename Format>
std::string join(const std::vector<T>& items, std::string_view separator, Format format) {
    std::string joined;
    for (const T& item : items) {
        if (!joined.empty()) {
            joined += separator;
        }
        joined += format(item);
    }
    return joined;
}

}

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

std::vector<EvaluationJob> run_evaluations(const EvalConfig& config,
                                           const data::SnapshotDataSource& source,
                                           const std::vector<std::string>& symbols,
                                           std::ostream& progress) {
    std::vector<EvaluationJob> jobs;
    for (const auto mode : config.window_modes) {
        for (const std::string& symbol : symbols) {
            jobs.push_back({.symbol = symbol, .mode = mode, .result = {}, .failure = {}});
        }
    }
    std::mutex progress_mutex;
    std::size_t finished = 0;
    run_parallel(jobs.size(), config.threads, [&](std::size_t i) {
        EvaluationJob& job = jobs[i];
        const auto start = std::chrono::steady_clock::now();
        try {
            const data::Snapshot snapshot = source.load_snapshot(job.symbol);
            job.result = evaluation::evaluate_asset(
                snapshot, config.models,
                {.window = config.window, .refit_every = config.refit_every, .mode = job.mode});
        } catch (const Error& error) {
            job.failure = error.what();
        }
        const double seconds =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        const std::scoped_lock lock{progress_mutex};
        ++finished;
        progress << std::format("[{}/{}] {} {} {} ({:.1f} s)\n", finished, jobs.size(), job.symbol,
                                evaluation::to_string(job.mode),
                                job.failure.empty() ? "done" : "FAILED", seconds);
    });
    return jobs;
}

std::string utc_now() {
    return std::format("{:%Y-%m-%dT%H:%M:%SZ}",
                       std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
}

void write_run_json(const std::filesystem::path& directory, const EvalConfig& config,
                    const data::Manifest& manifest, const std::vector<std::string>& symbols,
                    const RunRecord& record) {
    const BuildInfo build = build_info();
    const std::string modes = join(config.window_modes, ", ", [](evaluation::WindowMode mode) {
        return json_string(evaluation::to_string(mode));
    });
    const std::string confidences =
        join(config.confidence_levels, ", ", [](double level) { return std::format("{}", level); });
    const std::string models = join(config.models, ",\n", [](const models::ModelSpec& spec) {
        return "    " + model_json(spec);
    });
    const std::string snapshots = join(symbols, ",\n", [&](const std::string& symbol) {
        const auto& asset = manifest.find(symbol);
        return std::format(R"(    {{"symbol": {}, "file": {}, "sha256": {}}})", json_string(symbol),
                           json_string(asset.file.generic_string()), json_string(asset.sha256));
    });

    std::filesystem::create_directories(directory);
    std::ofstream out{directory / "run.json", std::ios::binary | std::ios::trunc};
    out << std::format("{{\n"
                       "  \"name\": {},\n"
                       "  \"command\": {},\n"
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
                       "  \"confidence_levels\": [{}],\n"
                       "  \"baseline\": {},\n"
                       "  \"failures\": {},\n"
                       "  \"models\": [\n{}\n  ],\n"
                       "  \"snapshots\": [\n{}\n  ]\n"
                       "}}\n",
                       json_string(config.name), json_string(record.command),
                       json_string(version()), json_string(build.commit), build.dirty,
                       json_string(build.compiler), json_string(config.path.generic_string()),
                       json_string(config.sha256), json_string(record.started_at), record.seconds,
                       config.window, config.refit_every, modes, confidences,
                       json_string(config.baseline), record.failures, models, snapshots);
}

void warn_if_dirty(std::ostream& out) {
    if (build_info().dirty) {
        out << "warning: built from a working tree with uncommitted changes; do not publish "
               "these results\n";
    }
}

}
