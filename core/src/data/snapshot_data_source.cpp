#include <zpew/data/bar.hpp>
#include <zpew/data/gap.hpp>
#include <zpew/data/iso_date.hpp>
#include <zpew/data/manifest.hpp>
#include <zpew/data/snapshot.hpp>
#include <zpew/data/snapshot_csv.hpp>
#include <zpew/data/snapshot_data_source.hpp>
#include <zpew/data/validation.hpp>
#include <zpew/error.hpp>
#include <zpew/io/binary_file.hpp>
#include <zpew/io/sha256.hpp>

#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace zpew::data {

namespace {

void verify_hash(const ManifestAsset& asset, std::string_view bytes, const std::string& path) {
    const std::string actual = io::to_hex(io::sha256(bytes));
    if (actual != asset.sha256) {
        throw DataError{std::format("{}: sha256 mismatch for {}: manifest has {}, file has {}",
                                    path, asset.symbol, asset.sha256, actual)};
    }
}

void verify_against_manifest(const ManifestAsset& asset, const std::vector<Bar>& bars,
                             const std::string& path) {
    if (bars.size() != asset.rows) {
        throw DataError{std::format("{}: manifest has {} rows for {}, file has {}", path,
                                    asset.rows, asset.symbol, bars.size())};
    }
    if (bars.front().date != asset.first_date || bars.back().date != asset.last_date) {
        throw DataError{std::format(
            "{}: manifest dates for {} are {} to {}, file covers {} to {}", path, asset.symbol,
            format_iso_date(asset.first_date), format_iso_date(asset.last_date),
            format_iso_date(bars.front().date), format_iso_date(bars.back().date))};
    }
}

}

SnapshotDataSource::SnapshotDataSource(const std::filesystem::path& manifest_path)
    : manifest_{load_manifest(manifest_path)} {}

std::vector<Bar> SnapshotDataSource::load(std::string_view symbol) const {
    return load_snapshot(symbol).bars;
}

Snapshot SnapshotDataSource::load_snapshot(std::string_view symbol) const {
    const ManifestAsset& asset = manifest_.find(symbol);
    const std::filesystem::path path = manifest_.snapshot_path(asset);
    const std::string path_text = path.generic_string();

    const std::string bytes = io::read_binary_file(path);
    verify_hash(asset, bytes, path_text);

    std::vector<Bar> bars = parse_snapshot_csv(bytes, path_text);
    try {
        validate_bars(bars, asset.calendar);
    } catch (const ValidationError& error) {
        throw ValidationError{std::format("{}: {}", path_text, error.what())};
    }
    verify_against_manifest(asset, bars, path_text);

    std::vector<Gap> gaps = find_gaps(bars, asset.calendar);
    return Snapshot{.asset = asset, .bars = std::move(bars), .gaps = std::move(gaps)};
}

const Manifest& SnapshotDataSource::manifest() const noexcept {
    return manifest_;
}

}
