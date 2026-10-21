#include "fit_command.hpp"

#include "exit_code.hpp"
#include "manifest_path.hpp"
#include "options.hpp"

#include <zpew/data/bar.hpp>
#include <zpew/data/iso_date.hpp>
#include <zpew/data/snapshot.hpp>
#include <zpew/data/snapshot_data_source.hpp>
#include <zpew/models/garch_model.hpp>
#include <zpew/returns/log_returns.hpp>

#include <format>
#include <ostream>
#include <span>
#include <string>
#include <vector>

namespace zpew::cli {

int run_fit(const Options& options, std::ostream& out) {
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
        "  \"iterations\": {}\n"
        "}}\n",
        snapshot.asset.symbol, model.name(), data::format_iso_date(snapshot.bars[1].date),
        data::format_iso_date(snapshot.bars.back().date), fit.observations, fit.params.omega,
        fit.params.alpha, fit.params.beta, fit.params.persistence(), fit.params.long_run_variance(),
        fit.log_likelihood, fit.backcast, fit.starts, fit.converged_starts, fit.iterations);
    return kExitSuccess;
}

}
