#include "models/garch_simulation.hpp"
#include "returns/naive_reference.hpp"

#include <zpew/error.hpp>
#include <zpew/evaluation/rolling_forecast.hpp>
#include <zpew/models/model_spec.hpp>
#include <zpew/models/realised_vol_model.hpp>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cstddef>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <vector>

using Catch::Matchers::WithinRel;
using zpew::evaluation::rolling_forecasts;
using zpew::evaluation::RollingOptions;
using zpew::evaluation::WindowMode;

namespace {

std::vector<double> sample_returns(std::size_t count) {
    return zpew::test::simulate_garch({.omega = 2e-6, .alpha = 0.08, .beta = 0.90}, count, 99U);
}

const std::vector<zpew::models::ModelSpec>& all_specs() {
    static const std::vector<zpew::models::ModelSpec> kSpecs{
        {.name = "rv21", .parameters = zpew::models::RealisedSpec{.window = 21}},
        {.name = "ewma", .parameters = zpew::models::EwmaSpec{.lambda = 0.94, .seed_window = 21}},
        {.name = "garch", .parameters = zpew::models::GarchSpec{.variance_targeting = false}},
    };
    return kSpecs;
}

}

TEST_CASE("rolling forecasts follow the refit schedule on a small example", "[evaluation]") {
    const std::vector<double> returns{0.01, -0.02, 0.03, 0.04, -0.05, 0.06};
    zpew::models::RealisedVolModel model{2};
    const auto series = rolling_forecasts(
        model, returns, {.window = 3, .refit_every = 2, .mode = WindowMode::kSliding});
    REQUIRE(series.first_index == 3);
    REQUIRE(series.refits == 2);
    REQUIRE(series.forecasts.size() == 3);
    const double tolerance = 8.0 * std::numeric_limits<double>::epsilon();
    REQUIRE_THAT(series.forecasts[0], WithinRel((4e-4 + 9e-4) / 2.0, tolerance));
    REQUIRE_THAT(series.forecasts[1], WithinRel((9e-4 + 16e-4) / 2.0, tolerance));
    REQUIRE_THAT(series.forecasts[2], WithinRel((16e-4 + 25e-4) / 2.0, tolerance));
}

TEST_CASE("a forecast for day t never changes when returns from day t onward change",
          "[evaluation][lookahead]") {
    const auto& spec = GENERATE(Catch::Generators::from_range(all_specs()));
    const WindowMode mode = GENERATE(WindowMode::kSliding, WindowMode::kExpanding);
    CAPTURE(spec.name, std::string{zpew::evaluation::to_string(mode)});
    const RollingOptions options{.window = 600, .refit_every = 50, .mode = mode};
    const auto returns = sample_returns(1400);

    const auto reference_model = zpew::models::make_model(spec);
    const auto reference = rolling_forecasts(*reference_model, returns, options);

    const auto different_future =
        zpew::test::simulate_garch({.omega = 5e-6, .alpha = 0.12, .beta = 0.85}, 1400, 4242U);
    for (const std::size_t cut : {std::size_t{600}, std::size_t{649}, std::size_t{650},
                                  std::size_t{1000}, std::size_t{1399}}) {
        CAPTURE(cut);
        auto corrupted = returns;
        for (std::size_t s = cut; s < corrupted.size(); ++s) {
            corrupted[s] = different_future[s];
        }
        const auto model = zpew::models::make_model(spec);
        const auto series = rolling_forecasts(*model, corrupted, options);
        for (std::size_t t = options.window; t <= cut; ++t) {
            REQUIRE(series.forecasts[t - options.window] ==
                    reference.forecasts[t - options.window]);
        }
    }
}

TEST_CASE("refitting every day gives the same forecasts as a fresh fit each day", "[evaluation]") {
    const auto returns = sample_returns(80);
    zpew::models::RealisedVolModel rolling_model{21};
    const auto series = rolling_forecasts(
        rolling_model, returns, {.window = 40, .refit_every = 1, .mode = WindowMode::kSliding});
    REQUIRE(series.refits == 40);
    for (std::size_t t = 40; t < returns.size(); ++t) {
        zpew::models::RealisedVolModel fresh{21};
        fresh.fit(std::span<const double>{returns}.subspan(t - 40, 40));
        REQUIRE(series.forecasts[t - 40] == fresh.forecast_variance());
    }
}

TEST_CASE("the expanding window fits on all returns so far", "[evaluation]") {
    const auto returns = sample_returns(60);
    zpew::models::RealisedVolModel model{21};
    const auto series = rolling_forecasts(
        model, returns, {.window = 30, .refit_every = 10, .mode = WindowMode::kExpanding});
    REQUIRE(series.refits == 3);
    const auto reference = zpew::test::naive_rolling_mean_of_squares(returns, 21);
    const double tolerance = 84.0 * std::numeric_limits<double>::epsilon();
    for (std::size_t t = 30; t < returns.size(); ++t) {
        REQUIRE_THAT(series.forecasts[t - 30], WithinRel(reference[t - 21], tolerance));
    }
}

TEST_CASE("rolling evaluation rejects bad options and short samples", "[evaluation]") {
    const auto returns = sample_returns(50);
    zpew::models::RealisedVolModel model{5};
    REQUIRE_THROWS_AS(rolling_forecasts(model, returns, {.window = 0}), zpew::InvalidArgument);
    REQUIRE_THROWS_AS(rolling_forecasts(model, returns, {.window = 10, .refit_every = 0}),
                      zpew::InvalidArgument);
    REQUIRE_THROWS_AS(rolling_forecasts(model, returns, {.window = 50}), zpew::InvalidArgument);
}

TEST_CASE("model failures carry the return index", "[evaluation]") {
    std::vector<double> returns(100, 0.0);
    returns[0] = 0.01;
    zpew::models::RealisedVolModel model{5};
    REQUIRE_THROWS_WITH(rolling_forecasts(model, returns, {.window = 10, .refit_every = 5}),
                        Catch::Matchers::ContainsSubstring("at return 10"));
}

TEST_CASE("window modes parse and print", "[evaluation]") {
    REQUIRE(zpew::evaluation::parse_window_mode("sliding") == WindowMode::kSliding);
    REQUIRE(zpew::evaluation::parse_window_mode("expanding") == WindowMode::kExpanding);
    REQUIRE_FALSE(zpew::evaluation::parse_window_mode("rolling").has_value());
    REQUIRE(zpew::evaluation::to_string(WindowMode::kExpanding) == "expanding");
}
