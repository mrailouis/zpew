#include "fit_command.hpp"

#include "eval_config.hpp"
#include "exit_code.hpp"
#include "json_text.hpp"
#include "manifest_path.hpp"
#include "options.hpp"
#include "parallel.hpp"

#include <zpew/data/bar.hpp>
#include <zpew/data/iso_date.hpp>
#include <zpew/data/snapshot.hpp>
#include <zpew/data/snapshot_data_source.hpp>
#include <zpew/error.hpp>
#include <zpew/models/garch_model.hpp>
#include <zpew/models/model_spec.hpp>
#include <zpew/returns/log_returns.hpp>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <mutex>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace zpew::cli {

namespace {

struct FitRow {
    std::string symbol;
    std::string model;
    std::optional<models::GarchFit> fit;
    std::string failure;
};

int run_fit_all(const EvalConfig& config, std::ostream& out, std::ostream& err) {
    const data::SnapshotDataSource source{config.manifest};
    std::vector<std::pair<std::string, const models::ModelSpec*>> jobs;
    for (const auto& asset : source.manifest().assets()) {
        if (!config.assets.empty() &&
            std::ranges::find(config.assets, asset.symbol) == config.assets.end()) {
            continue;
        }
        for (const auto& spec : config.models) {
            if (std::holds_alternative<models::GarchSpec>(spec.parameters)) {
                jobs.emplace_back(asset.symbol, &spec);
            }
        }
    }
    std::vector<FitRow> rows(jobs.size());
    std::mutex progress_mutex;
    std::size_t finished = 0;
    run_parallel(jobs.size(), config.threads, [&](std::size_t i) {
        FitRow row{
            .symbol = jobs[i].first, .model = jobs[i].second->name, .fit = {}, .failure = {}};
        try {
            const data::Snapshot snapshot = source.load_snapshot(row.symbol);
            const auto returns = returns::log_returns(std::span<const data::Bar>{snapshot.bars});
            models::GarchModel model{
                {.variance_targeting =
                     std::get<models::GarchSpec>(jobs[i].second->parameters).variance_targeting}};
            model.fit(returns);
            row.fit = model.fitted();
        } catch (const Error& error) {
            row.failure = error.what();
        }
        const std::scoped_lock lock{progress_mutex};
        ++finished;
        err << std::format("[{}/{}] {} {} {}\n", finished, jobs.size(), row.symbol, row.model,
                           row.failure.empty() ? "done" : "FAILED");
        rows[i] = std::move(row);
    });

    const std::filesystem::path file = config.run_directory() / "fits.csv";
    std::filesystem::create_directories(file.parent_path());
    std::ofstream csv{file, std::ios::binary | std::ios::trunc};
    csv << "symbol,model,observations,omega,alpha,beta,persistence,long_run_variance,"
           "log_likelihood,converged_starts,starts,at_boundary,failure\n";
    std::size_t failures = 0;
    for (const FitRow& row : rows) {
        if (!row.fit) {
            ++failures;
            csv << std::format("{},{},,,,,,,,,,,{}\n", row.symbol, row.model,
                               json_string(row.failure));
            continue;
        }
        const models::GarchFit& f = *row.fit;
        csv << std::format("{},{},{},{},{},{},{},{},{},{},{},{},\n", row.symbol, row.model,
                           f.observations, f.params.omega, f.params.alpha, f.params.beta,
                           f.params.persistence(), f.params.long_run_variance(), f.log_likelihood,
                           f.converged_starts, f.starts, f.at_boundary);
    }
    out << std::format("fitted {} asset-model pairs; {} failures; results in {}\n", rows.size(),
                       failures, file.generic_string());
    return kExitSuccess;
}

}

int run_fit(const Options& options, std::ostream& out, std::ostream& err) {
    if (const auto config = options.get("config")) {
        return run_fit_all(load_eval_config(*config), out, err);
    }
    if (options.require("model") != "garch") {
        throw UsageError{"fit supports --model garch only; EWMA and realised volatility have no "
                         "estimated parameters"};
    }
    const std::string targeting = options.get("targeting").value_or("off");
    if (targeting != "on" && targeting != "off") {
        throw UsageError{std::format("--targeting expects on or off, got '{}'", targeting)};
    }
    const data::SnapshotDataSource source{resolve_manifest(options.get("manifest"))};
    const data::Snapshot snapshot = source.load_snapshot(options.require("symbol"));
    const std::vector<double> returns =
        returns::log_returns(std::span<const data::Bar>{snapshot.bars});

    models::GarchModel model{{.variance_targeting = targeting == "on"}};
    model.fit(returns);
    const models::GarchFit& fit = model.fitted();
    out << std::format(
        "{{\n"
        "  \"symbol\": \"{}\",\n"
        "  \"model\": \"{}\",\n"
        "  \"first_return_date\": \"{}\",\n"
        "  \"last_return_date\": \"{}\",\n"
        "  \"observations\": {},\n"
        "  \"omega\": {},\n"
        "  \"alpha\": {},\n"
        "  \"beta\": {},\n"
        "  \"persistence\": {},\n"
        "  \"long_run_variance\": {},\n"
        "  \"log_likelihood\": {},\n"
        "  \"backcast\": {},\n"
        "  \"starts\": {},\n"
        "  \"converged_starts\": {},\n"
        "  \"iterations\": {},\n"
        "  \"at_boundary\": {}\n"
        "}}\n",
        snapshot.asset.symbol, model.name(), data::format_iso_date(snapshot.bars[1].date),
        data::format_iso_date(snapshot.bars.back().date), fit.observations, fit.params.omega,
        fit.params.alpha, fit.params.beta, fit.params.persistence(), fit.params.long_run_variance(),
        fit.log_likelihood, fit.backcast, fit.starts, fit.converged_starts, fit.iterations,
        fit.at_boundary);
    return kExitSuccess;
}

}
