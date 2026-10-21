#include "check_data_command.hpp"

#include "exit_code.hpp"

#include <zpew/data/gap.hpp>
#include <zpew/data/iso_date.hpp>
#include <zpew/data/manifest.hpp>
#include <zpew/data/snapshot.hpp>
#include <zpew/data/snapshot_data_source.hpp>
#include <zpew/error.hpp>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <ostream>
#include <string>

namespace zpew::cli {

namespace {

std::string describe_gaps(const data::Snapshot& snapshot) {
    if (snapshot.gaps.empty()) {
        return "no gaps";
    }
    const auto largest = std::ranges::max_element(snapshot.gaps, {}, &data::Gap::missing_sessions);
    return std::format("{} {}, largest {} sessions after {}", snapshot.gaps.size(),
                       snapshot.gaps.size() == 1 ? "gap" : "gaps", largest->missing_sessions,
                       data::format_iso_date(largest->last_before));
}

}

int run_check_data(const std::filesystem::path& manifest_path, std::ostream& out,
                   std::ostream& err) {
    const data::SnapshotDataSource source{manifest_path};
    std::size_t failures = 0;
    for (const data::ManifestAsset& asset : source.manifest().assets()) {
        try {
            const data::Snapshot snapshot = source.load_snapshot(asset.symbol);
            out << std::format("ok    {:<8} {:>6} bars  {} to {}  {}\n", asset.symbol,
                               snapshot.bars.size(), data::format_iso_date(asset.first_date),
                               data::format_iso_date(asset.last_date), describe_gaps(snapshot));
        } catch (const Error& error) {
            ++failures;
            err << std::format("FAIL  {:<8} {}\n", asset.symbol, error.what());
        }
    }
    const std::size_t total = source.manifest().assets().size();
    out << std::format("{} of {} snapshots verified\n", total - failures, total);
    return failures == 0 ? kExitSuccess : kExitDataError;
}

}
