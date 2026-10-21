#include "data/bar_builders.hpp"
#include "models/garch_simulation.hpp"

#include <zpew/data/bar.hpp>
#include <zpew/data/snapshot.hpp>
#include <zpew/error.hpp>
#include <zpew/evaluation/asset_evaluation.hpp>
#include <zpew/evaluation/improvement.hpp>
#include <zpew/evaluation/rolling_forecast.hpp>
#include <zpew/models/model_spec.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <vector>

using zpew::evaluation::evaluate_asset;
using zpew::evaluation::WindowMode;

namespace {

zpew::data::Snapshot simulated_snapshot(std::size_t returns) {
    const auto r =
        zpew::test::simulate_garch({.omega = 2e-6, .alpha = 0.08, .beta = 0.90}, returns, 5U);
    zpew::data::Snapshot snapshot;
    snapshot.asset.symbol = "SIM";
    auto date = zpew::test::day(2000, 1, 3);
    double close = 100.0;
    snapshot.bars.push_back({.date = date,
                             .open = close,
                             .high = close * 1.005,
                             .low = close * 0.995,
                             .close = close,
                             .volume = 1.0});
    for (const double ret : r) {
        date += std::chrono::days{1};
        const double open = close;
        close *= std::exp(ret);
        snapshot.bars.push_back({.date = date,
                                 .open = open,
                                 .high = std::max(open, close) * 1.002,
                                 .low = std::min(open, close) * 0.998,
                                 .close = close,
                                 .volume = 1.0});
    }
    return snapshot;
}

const std::vector<zpew::models::ModelSpec>& specs() {
    static const std::vector<zpew::models::ModelSpec> kSpecs{
        {.name = "rv21", .parameters = zpew::models::RealisedSpec{.window = 21}},
        {.name = "ewma", .parameters = zpew::models::EwmaSpec{.lambda = 0.94, .seed_window = 21}},
        {.name = "garch", .parameters = zpew::models::GarchSpec{.variance_targeting = false}},
    };
    return kSpecs;
}

void require_complete(const zpew::evaluation::ModelEvaluation& model, std::size_t rows) {
    REQUIRE_FALSE(model.failure.has_value());
    REQUIRE(model.forecasts.size() == rows);
    REQUIRE(model.losses.has_value());
    REQUIRE(model.refits == 6);
}

}

TEST_CASE("asset evaluation aligns dates, returns, proxies and forecasts", "[evaluation]") {
    const auto snapshot = simulated_snapshot(900);
    const auto asset = evaluate_asset(snapshot, specs(), {.window = 600, .refit_every = 50});
    REQUIRE(asset.symbol == "SIM");
    REQUIRE(asset.dates.size() == 300);
    REQUIRE(asset.dates.front() == snapshot.bars[601].date);
    REQUIRE(asset.dates.back() == snapshot.bars.back().date);
    REQUIRE(asset.returns.size() == 300);
    REQUIRE(asset.squared_return_proxy.front() == asset.returns.front() * asset.returns.front());
    REQUIRE(asset.parkinson_proxy.size() == 300);
}

TEST_CASE("every model that fits the window produces a full forecast series", "[evaluation]") {
    const auto snapshot = simulated_snapshot(900);
    const auto asset = evaluate_asset(snapshot, specs(), {.window = 600, .refit_every = 50});
    REQUIRE(asset.models.size() == 3);
    for (const auto& model : asset.models) {
        require_complete(model, 300);
    }
}

TEST_CASE("a model that cannot fit the window is reported, not thrown", "[evaluation]") {
    const auto snapshot = simulated_snapshot(450);
    const auto asset = evaluate_asset(snapshot, specs(), {.window = 300, .refit_every = 50});
    REQUIRE_FALSE(asset.models[0].failure.has_value());
    REQUIRE(asset.models[2].failure.value_or("") == "needs a window of at least 500 returns");
}

TEST_CASE("an asset shorter than the window fails loudly", "[evaluation]") {
    const auto snapshot = simulated_snapshot(100);
    REQUIRE_THROWS_WITH(evaluate_asset(snapshot, specs(), {.window = 600}),
                        Catch::Matchers::ContainsSubstring("SIM has 100 returns"));
}

TEST_CASE("improvements compare each model with the baseline on both proxies",
          "[evaluation][improvement]") {
    const auto snapshot = simulated_snapshot(900);
    const auto asset = evaluate_asset(snapshot, specs(), {.window = 600, .refit_every = 50});
    const auto improvements = zpew::evaluation::improvements_over_baseline(asset, "rv21");
    REQUIRE(improvements.size() == 4);
    const auto& ewma_vs_rv = improvements.front();
    REQUIRE(ewma_vs_rv.model == "ewma");
    REQUIRE(ewma_vs_rv.proxy == "squared_return");
    const auto rv = asset.models[0].losses.value_or(zpew::evaluation::ModelLosses{}).squared_return;
    const auto ewma =
        asset.models[1].losses.value_or(zpew::evaluation::ModelLosses{}).squared_return;
    REQUIRE_THAT(ewma_vs_rv.mse_percent,
                 Catch::Matchers::WithinRel(100.0 * (1.0 - (ewma.mse / rv.mse)), 1e-12));
    REQUIRE_THROWS_AS(zpew::evaluation::improvements_over_baseline(asset, "missing"),
                      zpew::InvalidArgument);
}

TEST_CASE("pooling reports mean, median and win counts", "[evaluation][improvement]") {
    using zpew::evaluation::Improvement;
    const std::vector<Improvement> items{
        {.symbol = "A", .model = "m", .proxy = "p", .mse_percent = 10.0, .qlike_percent = -5.0},
        {.symbol = "B", .model = "m", .proxy = "p", .mse_percent = -2.0, .qlike_percent = 1.0},
        {.symbol = "C", .model = "m", .proxy = "p", .mse_percent = 4.0, .qlike_percent = 3.0},
    };
    const auto pooled = zpew::evaluation::pool_improvements(items);
    REQUIRE(pooled.size() == 2);
    const auto& mse = pooled[0].metric == "mse" ? pooled[0] : pooled[1];
    REQUIRE(mse.assets == 3);
    REQUIRE(mse.mean_percent == 4.0);
    REQUIRE(mse.median_percent == 4.0);
    REQUIRE(mse.wins == 2);
}

TEST_CASE("median handles odd and even counts", "[evaluation][improvement]") {
    REQUIRE(zpew::evaluation::median({3.0, 1.0, 2.0}) == 2.0);
    REQUIRE(zpew::evaluation::median({4.0, 1.0, 3.0, 2.0}) == 2.5);
    REQUIRE_THROWS_AS(zpew::evaluation::median({}), zpew::InvalidArgument);
}
