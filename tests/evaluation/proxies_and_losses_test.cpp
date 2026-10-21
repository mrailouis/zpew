#include "data/bar_builders.hpp"

#include <zpew/data/bar.hpp>
#include <zpew/error.hpp>
#include <zpew/evaluation/losses.hpp>
#include <zpew/evaluation/proxies.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <limits>
#include <numbers>
#include <vector>

using Catch::Matchers::WithinRel;
using zpew::evaluation::relative_improvement_percent;
using zpew::evaluation::summarise_losses;

namespace {

constexpr double kFewRoundings = 16.0 * std::numeric_limits<double>::epsilon();

}

TEST_CASE("the squared-return proxy squares each return", "[evaluation][proxy]") {
    const std::vector<double> returns{0.5, -0.25};
    REQUIRE(zpew::evaluation::squared_return_proxy(returns) == std::vector<double>{0.25, 0.0625});
}

TEST_CASE("the Parkinson proxy uses each later bar's high-low range", "[evaluation][proxy]") {
    const auto first = zpew::test::bar_on(zpew::test::day(2024, 1, 4));
    auto second = zpew::test::bar_on(zpew::test::day(2024, 1, 5));
    second.high = 110.0;
    second.low = 100.0;
    second.open = 105.0;
    second.close = 105.0;
    auto flat = zpew::test::bar_on(zpew::test::day(2024, 1, 8));
    flat.high = flat.low = flat.open = flat.close = 100.0;
    const std::vector<zpew::data::Bar> bars{first, second, flat};
    const auto proxy = zpew::evaluation::parkinson_proxy(bars);
    REQUIRE(proxy.size() == 2);
    const double range = std::log(1.1);
    REQUIRE_THAT(proxy[0], WithinRel(range * range / (4.0 * std::numbers::ln2), kFewRoundings));
    REQUIRE(proxy[1] == 0.0);
}

TEST_CASE("losses match hand calculations, excluding zero proxies from QLIKE",
          "[evaluation][loss]") {
    const std::vector<double> forecasts{1.0, 2.0};
    const std::vector<double> proxies{2.0, 0.0};
    const auto losses = summarise_losses(forecasts, proxies);
    REQUIRE(losses.observations == 2);
    REQUIRE(losses.mse == 2.5);
    REQUIRE(losses.qlike_excluded == 1);
    REQUIRE_THAT(losses.qlike, WithinRel(1.0 - std::numbers::ln2, kFewRoundings));
    REQUIRE_THAT(losses.qlike_all_days, WithinRel((2.0 + std::numbers::ln2) / 2.0, kFewRoundings));
}

TEST_CASE("QLIKE is zero for a perfect forecast and positive otherwise", "[evaluation][loss]") {
    const std::vector<double> proxies{1e-4, 4e-4, 9e-4};
    REQUIRE(summarise_losses(proxies, proxies).qlike == 0.0);
    const std::vector<double> biased{2e-4, 8e-4, 18e-4};
    REQUIRE(summarise_losses(biased, proxies).qlike > 0.0);
}

TEST_CASE("QLIKE is undefined when every proxy is zero", "[evaluation][loss]") {
    const std::vector<double> forecasts{1e-4, 1e-4};
    const std::vector<double> zeros{0.0, 0.0};
    const auto losses = summarise_losses(forecasts, zeros);
    REQUIRE(std::isnan(losses.qlike));
    REQUIRE(losses.qlike_excluded == 2);
}

TEST_CASE("loss inputs are validated", "[evaluation][loss]") {
    const std::vector<double> one{1.0};
    const std::vector<double> two{1.0, 1.0};
    REQUIRE_THROWS_AS(summarise_losses(one, two), zpew::InvalidArgument);
    REQUIRE_THROWS_AS(summarise_losses(std::vector<double>{}, std::vector<double>{}),
                      zpew::InvalidArgument);
    const std::vector<double> zero_forecast{0.0};
    REQUIRE_THROWS_AS(summarise_losses(zero_forecast, one), zpew::InvalidArgument);
    const std::vector<double> negative_proxy{-1.0};
    REQUIRE_THROWS_AS(summarise_losses(one, negative_proxy), zpew::InvalidArgument);
}

TEST_CASE("relative improvement is positive when the model loss is lower", "[evaluation][loss]") {
    REQUIRE_THAT(relative_improvement_percent(0.9, 1.0), WithinRel(10.0, kFewRoundings));
    REQUIRE_THAT(relative_improvement_percent(1.2, 1.0), WithinRel(-20.0, kFewRoundings));
    REQUIRE_THROWS_AS(relative_improvement_percent(1.0, 0.0), zpew::InvalidArgument);
}
