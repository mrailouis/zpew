#include "forecast_command.hpp"

#include "exit_code.hpp"
#include "manifest_path.hpp"
#include "options.hpp"

#include <zpew/data/bar.hpp>
#include <zpew/data/iso_date.hpp>
#include <zpew/data/snapshot.hpp>
#include <zpew/data/snapshot_data_source.hpp>
#include <zpew/models/ewma_model.hpp>
#include <zpew/models/realised_vol_model.hpp>
#include <zpew/models/vol_model.hpp>
#include <zpew/returns/log_returns.hpp>

#include <algorithm>
#include <cstddef>
#include <format>
#include <memory>
#include <ostream>
#include <span>
#include <string>
#include <vector>

namespace zpew::cli {

namespace {

std::unique_ptr<models::VolModel> make_model(const Options& options, std::size_t& warm_up) {
    const std::string model = options.require("model");
    if (model == "ewma") {
        auto ewma = std::make_unique<models::EwmaModel>(
            options.number("lambda", models::kRiskMetricsDailyLambda),
            options.count("seed", models::kDefaultEwmaSeedWindow));
        warm_up = ewma->seed_window();
        return ewma;
    }
    if (model == "realised") {
        auto realised = std::make_unique<models::RealisedVolModel>(
            options.count("window", models::kDefaultRealisedWindow));
        warm_up = realised->window();
        return realised;
    }
    throw UsageError{std::format("unknown model '{}'; expected ewma or realised", model)};
}

}

int run_forecast(const Options& options, std::ostream& out) {
    const data::SnapshotDataSource source{resolve_manifest(options.get("manifest"))};
    const data::Snapshot snapshot = source.load_snapshot(options.require("symbol"));
    const std::vector<double> returns =
        returns::log_returns(std::span<const data::Bar>{snapshot.bars});

    std::size_t warm_up = 0;
    const std::unique_ptr<models::VolModel> model = make_model(options, warm_up);
    model->fit(std::span<const double>{returns}.first(std::min(warm_up, returns.size())));

    out << std::format("# symbol={} model={}\n", snapshot.asset.symbol, model->name());
    out << "date,return,forecast_variance\n";
    for (std::size_t t = warm_up; t < returns.size(); ++t) {
        out << std::format("{},{},{}\n", data::format_iso_date(snapshot.bars[t + 1].date),
                           returns[t], model->forecast_variance());
        model->update(returns[t]);
    }
    return kExitSuccess;
}

}
