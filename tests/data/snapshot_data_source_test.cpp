#include "data/bar_builders.hpp"
#include "data/manifest_builders.hpp"
#include "support/temporary_directory.hpp"

#include <zpew/data/data_source.hpp>
#include <zpew/data/gap.hpp>
#include <zpew/data/snapshot_data_source.hpp>
#include <zpew/error.hpp>
#include <zpew/io/sha256.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <filesystem>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>
#include <vector>

using Catch::Matchers::ContainsSubstring;
using zpew::data::Gap;
using zpew::data::SnapshotDataSource;
using zpew::test::day;
using zpew::test::TemporaryDirectory;
using zpew::test::write_bytes;

namespace {

constexpr std::string_view kSnapshot = "date,open,high,low,close,volume\n"
                                       "2024-01-04,100,102,99,101,1000\n"
                                       "2024-01-05,101,103,100,102,1100\n"
                                       "2024-01-11,102,104,101,103,1200\n";

class SnapshotFixture {
public:
    explicit SnapshotFixture(const std::string& name) : directory_{name} {}

    [[nodiscard]] std::filesystem::path write(std::string_view csv, nlohmann::json asset) const {
        write_bytes(directory_.path() / "snapshots" / "SPY.csv", csv);
        if (!asset.contains("sha256_override")) {
            asset["sha256"] = zpew::io::to_hex(zpew::io::sha256(csv));
        } else {
            asset["sha256"] = asset["sha256_override"];
            asset.erase("sha256_override");
        }
        const nlohmann::json manifest{{"schema_version", 1},
                                      {"assets", nlohmann::json::array({asset})}};
        const auto manifest_path = directory_.path() / "manifest.json";
        write_bytes(manifest_path, manifest.dump(2));
        return manifest_path;
    }

private:
    TemporaryDirectory directory_;
};

nlohmann::json asset_for_snapshot() {
    nlohmann::json asset = zpew::test::valid_asset_json();
    asset["first_date"] = "2024-01-04";
    asset["last_date"] = "2024-01-11";
    asset["rows"] = 3;
    return asset;
}

}

TEST_CASE("a verified snapshot loads with its bars and gaps", "[data][snapshot]") {
    const SnapshotFixture fixture{"zpew_snapshot_test_good"};
    const SnapshotDataSource source{fixture.write(kSnapshot, asset_for_snapshot())};

    const auto snapshot = source.load_snapshot("SPY");
    REQUIRE(snapshot.asset.symbol == "SPY");
    REQUIRE(snapshot.bars.size() == 3);
    REQUIRE(snapshot.bars.back().close == 103.0);
    REQUIRE(snapshot.gaps == std::vector<Gap>{Gap{.last_before = day(2024, 1, 5),
                                                  .first_after = day(2024, 1, 11),
                                                  .missing_sessions = 3}});

    const zpew::data::DataSource& interface = source;
    REQUIRE(interface.load("SPY") == snapshot.bars);
}

TEST_CASE("a single changed byte fails the hash check before parsing", "[data][snapshot]") {
    const SnapshotFixture fixture{"zpew_snapshot_test_tampered"};
    auto asset = asset_for_snapshot();
    asset["sha256_override"] = zpew::io::to_hex(zpew::io::sha256(kSnapshot));
    std::string tampered{kSnapshot};
    tampered[tampered.size() - 2] = '4';
    const SnapshotDataSource source{fixture.write(tampered, asset)};
    REQUIRE_THROWS_AS(source.load_snapshot("SPY"), zpew::DataError);
    REQUIRE_THROWS_WITH(source.load_snapshot("SPY"), ContainsSubstring("sha256 mismatch for SPY"));
}

TEST_CASE("row count and date range must match the manifest", "[data][snapshot]") {
    const SnapshotFixture fixture{"zpew_snapshot_test_mismatch"};

    auto wrong_rows = asset_for_snapshot();
    wrong_rows["rows"] = 4;
    const SnapshotDataSource rows_source{fixture.write(kSnapshot, wrong_rows)};
    REQUIRE_THROWS_WITH(rows_source.load_snapshot("SPY"),
                        ContainsSubstring("manifest has 4 rows for SPY, file has 3"));

    auto wrong_dates = asset_for_snapshot();
    wrong_dates["last_date"] = "2024-01-12";
    const SnapshotDataSource dates_source{fixture.write(kSnapshot, wrong_dates)};
    REQUIRE_THROWS_WITH(dates_source.load_snapshot("SPY"),
                        ContainsSubstring("file covers 2024-01-04 to 2024-01-11"));
}

TEST_CASE("invalid bars fail validation with the file name", "[data][snapshot]") {
    const SnapshotFixture fixture{"zpew_snapshot_test_invalid"};
    const std::string weekend = "date,open,high,low,close,volume\n"
                                "2024-01-05,100,102,99,101,1000\n"
                                "2024-01-06,101,103,100,102,1100\n";
    auto asset = asset_for_snapshot();
    asset["first_date"] = "2024-01-05";
    asset["last_date"] = "2024-01-06";
    asset["rows"] = 2;
    const SnapshotDataSource source{fixture.write(weekend, asset)};
    REQUIRE_THROWS_AS(source.load_snapshot("SPY"), zpew::ValidationError);
    REQUIRE_THROWS_WITH(source.load_snapshot("SPY"),
                        ContainsSubstring("SPY.csv: bar 2 (2024-01-06): date is not a session"));
}

TEST_CASE("missing snapshot files and unknown symbols fail loudly", "[data][snapshot]") {
    const SnapshotFixture fixture{"zpew_snapshot_test_missing"};
    const auto manifest_path = fixture.write(kSnapshot, asset_for_snapshot());
    std::filesystem::remove(manifest_path.parent_path() / "snapshots" / "SPY.csv");
    const SnapshotDataSource source{manifest_path};
    REQUIRE_THROWS_AS(source.load_snapshot("SPY"), zpew::DataError);
    REQUIRE_THROWS_WITH(source.load_snapshot("QQQ"), ContainsSubstring("unknown symbol 'QQQ'"));
}

TEST_CASE("a missing manifest fails at construction", "[data][snapshot]") {
    const TemporaryDirectory directory{"zpew_snapshot_test_no_manifest"};
    REQUIRE_THROWS_AS(SnapshotDataSource{directory.path() / "manifest.json"}, zpew::DataError);
}
