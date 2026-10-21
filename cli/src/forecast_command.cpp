#include "forecast_command.hpp"

#include "exit_code.hpp"
#include "manifest_path.hpp"
#include "model_factory.hpp"
#include "options.hpp"

#include <zpew/data/bar.hpp>
#include <zpew/data/iso_date.hpp>
#include <zpew/data/snapshot.hpp>
#include <zpew/data/snapshot_data_source.hpp>
#include <zpew/models/vol_model.hpp>
#include <zpew/returns/log_returns.hpp>

#include <algorithm>
#include <cstddef>
#include <format>
#include <ostream>
#include <span>
#include <vector>

namespace zpew::cli {

int run_forecast(const Options& options, std::ostream& out) {
    const data::SnapshotDataSource source{resolve_manifest(options.get("manifest"))};
    const data::Snapshot snapshot = source.load_snapshot(options.require("symbol"));
    const std::vector<double> returns =
        returns::log_returns(std::span<const data::Bar>{snapshot.bars});

    const ModelChoice choice = make_model(options);
    const std::size_t warm_up = choice.fit_length;
    models::VolModel& model = *choice.model;
    model.fit(std::span<const double>{returns}.first(std::min(warm_up, returns.size())));

    out << std::format("# symbol={} model={}\n", snapshot.asset.symbol, model.name());
    out << "date,return,forecast_variance\n";
    for (std::size_t t = warm_up; t < returns.size(); ++t) {
        out << std::format("{},{},{}\n", data::format_iso_date(snapshot.bars[t + 1].date),
                           returns[t], model.forecast_variance());
        model.update(returns[t]);
    }
    return kExitSuccess;
}

}
