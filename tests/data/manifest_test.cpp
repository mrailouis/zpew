#include "data/bar_builders.hpp"
#include "data/manifest_builders.hpp"
#include "support/temporary_directory.hpp"

#include <zpew/data/manifest.hpp>
#include <zpew/data/trading_calendar.hpp>
#include <zpew/error.hpp>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <filesystem>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>

using Catch::Matchers::ContainsSubstring;
using nlohmann::json;
using zpew::data::parse_manifest;
using zpew::data::TradingCalendar;
using zpew::test::day;
using zpew::test::valid_asset_json;
using zpew::test::valid_manifest_json;

namespace {

zpew::data::Manifest parse(const json& manifest) {
    return parse_manifest(manifest.dump(), "data", "manifest.json");
}

void require_rejected(const json& manifest, const std::string& message) {
    REQUIRE_THROWS_AS(parse(manifest), zpew::DataError);
    REQUIRE_THROWS_WITH(parse(manifest), ContainsSubstring(message));
}

json with_asset_field(const std::string& key, const json& value) {
    json manifest = valid_manifest_json();
    manifest["assets"][0][key] = value;
    return manifest;
}

}

TEST_CASE("a valid manifest parses into typed assets", "[data][manifest]") {
    const auto manifest = parse(valid_manifest_json());
    REQUIRE(manifest.directory() == std::filesystem::path{"data"});
    REQUIRE(manifest.assets().size() == 1);
    const auto& asset = manifest.find("SPY");
    REQUIRE(asset.name == "SPDR S&P 500 ETF Trust");
    REQUIRE(asset.calendar == TradingCalendar::kWeekdays);
    REQUIRE(asset.first_date == day(2024, 1, 4));
    REQUIRE(asset.last_date == day(2024, 1, 8));
    REQUIRE(asset.rows == 3);
    REQUIRE(asset.sha256 == std::string(64, 'a'));
    REQUIRE(manifest.snapshot_path(asset) == std::filesystem::path{"data/snapshots/SPY.csv"});
}

TEST_CASE("unknown symbols fail loudly", "[data][manifest]") {
    const auto manifest = parse(valid_manifest_json());
    REQUIRE_THROWS_WITH(manifest.find("QQQ"), ContainsSubstring("unknown symbol 'QQQ'"));
}

TEST_CASE("malformed JSON and wrong top-level shapes are rejected", "[data][manifest]") {
    REQUIRE_THROWS_WITH(parse_manifest("{", "data", "manifest.json"),
                        ContainsSubstring("manifest.json: invalid JSON"));
    require_rejected(json::array(), "top level must be an object");
    require_rejected({{"schema_version", 2}, {"assets", valid_manifest_json()["assets"]}},
                     "'schema_version' must be 1");
    require_rejected({{"schema_version", 1}, {"assets", json::array()}},
                     "'assets' must be a non-empty array");
    require_rejected({{"schema_version", 1}}, "missing key 'assets'");
    json extra = valid_manifest_json();
    extra["comment"] = "x";
    require_rejected(extra, "unexpected key 'comment'");
}

TEST_CASE("every asset field is required and unknown fields are rejected", "[data][manifest]") {
    const std::string key = GENERATE(as<std::string>{}, "symbol", "name", "asset_class", "calendar",
                                     "source", "source_url", "terms_url", "fetched_at",
                                     "first_date", "last_date", "rows", "file", "sha256");
    CAPTURE(key);
    json manifest = valid_manifest_json();
    manifest["assets"][0].erase(key);
    require_rejected(manifest, "assets[0]: missing key '" + key + "'");

    json extra = valid_manifest_json();
    extra["assets"][0]["notes"] = "x";
    require_rejected(extra, "assets[0]: unexpected key 'notes'");
}

TEST_CASE("asset field values are validated", "[data][manifest]") {
    require_rejected(with_asset_field("symbol", ""), "'symbol' must be a non-empty string");
    require_rejected(with_asset_field("name", 5), "'name' must be a non-empty string");
    require_rejected(with_asset_field("calendar", "monthly"),
                     "'calendar' must be 'weekdays' or 'daily'");
    require_rejected(with_asset_field("first_date", "2024-1-04"),
                     "'first_date' must be a YYYY-MM-DD");
    require_rejected(with_asset_field("last_date", "2024-01-04"), "'last_date' must be after");
    require_rejected(with_asset_field("rows", 1), "'rows' must be an integer of at least 2");
    require_rejected(with_asset_field("rows", -3), "'rows' must be an integer");
    require_rejected(with_asset_field("rows", 3.5), "'rows' must be an integer");
    require_rejected(with_asset_field("sha256", std::string(63, 'a')), "'sha256' must be 64");
    require_rejected(with_asset_field("sha256", std::string(64, 'A')), "'sha256' must be 64");
}

TEST_CASE("fetched_at must be a UTC timestamp", "[data][manifest]") {
    const std::string bad =
        GENERATE(as<std::string>{}, "2026-10-21", "2026-10-21 12:00:00Z", "2026-10-21T12:00:00",
                 "2026-10-21T12:00:00+01:00", "2026-10-21T24:00:00Z", "2026-10-21T12:60:00Z",
                 "2026-13-21T12:00:00Z", "2026-10-21T1a:00:00Z");
    CAPTURE(bad);
    require_rejected(with_asset_field("fetched_at", bad), "'fetched_at' must look like");
}

TEST_CASE("snapshot files must stay inside the manifest directory", "[data][manifest]") {
    const std::string bad = GENERATE(as<std::string>{}, "../secret.csv", "snapshots/../../x.csv",
                                     "/etc/passwd", "./snapshots/SPY.csv");
    CAPTURE(bad);
    require_rejected(with_asset_field("file", bad), "'file' must be a relative path inside");
}

TEST_CASE("symbols and files must be unique", "[data][manifest]") {
    json duplicate_symbol = valid_manifest_json();
    json second = valid_asset_json();
    second["file"] = "snapshots/other.csv";
    duplicate_symbol["assets"].push_back(second);
    require_rejected(duplicate_symbol, "assets[1]: duplicate symbol 'SPY'");

    json duplicate_file = valid_manifest_json();
    json third = valid_asset_json("QQQ");
    third["file"] = "snapshots/SPY.csv";
    duplicate_file["assets"].push_back(third);
    require_rejected(duplicate_file, "assets[1]: duplicate file 'snapshots/SPY.csv'");
}

TEST_CASE("find_manifest walks up from a nested directory", "[data][manifest]") {
    const zpew::test::TemporaryDirectory root{"zpew_find_manifest_test"};
    zpew::test::write_bytes(root.path() / "data" / "manifest.json", "{}");
    const auto nested = root.path() / "build" / "app" / "zpew-app.app" / "Contents" / "MacOS";
    std::filesystem::create_directories(nested);

    const std::filesystem::path found =
        zpew::data::find_manifest(nested).value_or(std::filesystem::path{});
    REQUIRE_FALSE(found.empty());
    REQUIRE(std::filesystem::equivalent(found, root.path() / "data" / "manifest.json"));
    REQUIRE(zpew::data::find_manifest(root.path()).has_value());
}

TEST_CASE("find_manifest returns nothing when no manifest exists above", "[data][manifest]") {
    const zpew::test::TemporaryDirectory root{"zpew_find_manifest_missing_test"};
    REQUIRE_FALSE(zpew::data::find_manifest(root.path()).has_value());
}
