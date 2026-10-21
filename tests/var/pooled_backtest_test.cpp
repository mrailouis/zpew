#include <zpew/evaluation/rolling_forecast.hpp>
#include <zpew/var/backtest.hpp>
#include <zpew/var/pooled_backtest.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <vector>

using zpew::var::AssetBacktest;

namespace {

AssetBacktest make(const char* symbol, std::size_t n, std::size_t x) {
    return AssetBacktest{.symbol = symbol,
                         .model = "garch",
                         .result = {.confidence = 0.95,
                                    .observations = n,
                                    .breaches = x,
                                    .breach_rate = static_cast<double>(x) / static_cast<double>(n),
                                    .expected_rate = 0.05,
                                    .kupiec = zpew::var::kupiec_test(n, x, 0.05)}};
}

}

TEST_CASE("pooled backtests sum breaches and count Kupiec rejections", "[var][pool]") {
    const std::vector<AssetBacktest> items{make("A", 1000, 50), make("B", 1000, 80),
                                           make("C", 500, 20)};
    const auto pooled = zpew::var::pool_backtests(items);
    REQUIRE(pooled.size() == 1);
    const auto& row = pooled.front();
    REQUIRE(row.assets == 3);
    REQUIRE(row.observations == 2500);
    REQUIRE(row.breaches == 150);
    REQUIRE(row.pooled_breach_rate == 0.06);
    REQUIRE(row.median_breach_rate == 0.05);
    REQUIRE(row.kupiec_rejections == 1);
}

TEST_CASE("backtests are pooled separately per mode, model and confidence", "[var][pool]") {
    const zpew::var::BacktestResult result{.confidence = 0.95,
                                           .observations = 100,
                                           .breaches = 5,
                                           .breach_rate = 0.05,
                                           .expected_rate = 0.05,
                                           .kupiec = zpew::var::kupiec_test(100, 5, 0.05)};
    auto at_99 = result;
    at_99.confidence = 0.99;
    const std::vector<AssetBacktest> items{
        {.symbol = "A",
         .mode = zpew::evaluation::WindowMode::kSliding,
         .model = "ewma",
         .result = result},
        {.symbol = "A",
         .mode = zpew::evaluation::WindowMode::kExpanding,
         .model = "ewma",
         .result = result},
        {.symbol = "A",
         .mode = zpew::evaluation::WindowMode::kSliding,
         .model = "garch",
         .result = result},
        {.symbol = "A",
         .mode = zpew::evaluation::WindowMode::kSliding,
         .model = "ewma",
         .result = at_99},
    };
    REQUIRE(zpew::var::pool_backtests(items).size() == 4);
}
