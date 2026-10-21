#include "data/bar_builders.hpp"

#include <zpew/data/bar.hpp>
#include <zpew/error.hpp>
#include <zpew/returns/log_returns.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <limits>
#include <span>
#include <vector>

using Catch::Matchers::WithinRel;
using zpew::returns::log_returns;

namespace {

constexpr double kOneRoundingOfLog = 2.0 * std::numeric_limits<double>::epsilon();

}

TEST_CASE("log returns match hand-calculated values for exactly representable ratios",
          "[returns]") {
    const std::vector<double> closes{100.0, 125.0, 62.5, 62.5};
    const auto returns = log_returns(std::span<const double>{closes});
    REQUIRE(returns.size() == 3);
    REQUIRE_THAT(returns[0], WithinRel(0.22314355131420976, kOneRoundingOfLog));
    REQUIRE_THAT(returns[1], WithinRel(-0.6931471805599453, kOneRoundingOfLog));
    REQUIRE(returns[2] == 0.0);
}

TEST_CASE("log returns of a round trip sum to zero within rounding", "[returns]") {
    const std::vector<double> closes{100.0, 150.0, 100.0};
    const auto returns = log_returns(std::span<const double>{closes});
    REQUIRE(std::abs(returns[0] + returns[1]) <= 4.0 * std::numeric_limits<double>::epsilon());
}

TEST_CASE("log returns from bars use the close", "[returns]") {
    auto first = zpew::test::bar_on(zpew::test::day(2024, 1, 4));
    auto second = zpew::test::bar_on(zpew::test::day(2024, 1, 5));
    first.close = 50.0;
    second.close = 100.0;
    const std::vector<zpew::data::Bar> bars{first, second};
    const auto returns = log_returns(std::span<const zpew::data::Bar>{bars});
    REQUIRE(returns.size() == 1);
    REQUIRE_THAT(returns[0], WithinRel(std::log(2.0), kOneRoundingOfLog));
}

TEST_CASE("fewer than two closes give no returns", "[returns]") {
    REQUIRE(log_returns(std::span<const double>{}).empty());
    const std::vector<double> one{100.0};
    REQUIRE(log_returns(std::span<const double>{one}).empty());
}

TEST_CASE("invalid closes fail loudly", "[returns]") {
    for (const double bad : {0.0, -1.0, std::numeric_limits<double>::quiet_NaN(),
                             std::numeric_limits<double>::infinity()}) {
        const std::vector<double> closes{100.0, bad, 100.0};
        REQUIRE_THROWS_AS(log_returns(std::span<const double>{closes}), zpew::InvalidArgument);
    }
    const std::vector<double> single_bad{-5.0};
    REQUIRE_THROWS_AS(log_returns(std::span<const double>{single_bad}), zpew::InvalidArgument);
}
