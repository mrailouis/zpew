#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace zpew::test {

inline nlohmann::json valid_asset_json(const std::string& symbol = "SPY") {
    return {
        {"symbol", symbol},
        {"name", "SPDR S&P 500 ETF Trust"},
        {"asset_class", "us_equity_broad"},
        {"calendar", "weekdays"},
        {"source", "tiingo"},
        {"source_url", "https://api.tiingo.com/tiingo/daily/spy/prices"},
        {"terms_url", "https://www.tiingo.com/about/pricing"},
        {"fetched_at", "2026-10-21T12:00:00Z"},
        {"first_date", "2024-01-04"},
        {"last_date", "2024-01-08"},
        {"rows", 3},
        {"file", "snapshots/" + symbol + ".csv"},
        {"sha256", std::string(64, 'a')},
    };
}

inline nlohmann::json valid_manifest_json() {
    return {{"schema_version", 1}, {"assets", nlohmann::json::array({valid_asset_json()})}};
}

}
