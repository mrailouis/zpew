#include "returns/naive_reference.hpp"

#include <zpew/error.hpp>
#include <zpew/returns/moving_average.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <cstddef>
#include <limits>
#include <span>
#include <vector>

using Catch::Matchers::WithinRel;
using zpew::returns::simple_moving_average;

TEST_CASE("the simple moving average matches hand-calculated values", "[returns][sma]") {
    const std::vector<double> closes{10.0, 11.0, 12.0, 13.0, 20.0};
    const auto averages = simple_moving_average(closes, 3);
    REQUIRE(averages == std::vector<double>{11.0, 12.0, 15.0});
}

TEST_CASE("a window of one returns the input", "[returns][sma]") {
    const std::vector<double> closes{10.5, 11.25, 9.0};
    REQUIRE(simple_moving_average(closes, 1) == closes);
}

TEST_CASE("the simple moving average agrees with the naive reference on a price path",
          "[returns][sma]") {
    std::vector<double> closes(3000);
    double price = 100.0;
    for (std::size_t i = 0; i < closes.size(); ++i) {
        price *= std::exp(0.02 * std::sin(0.11 * static_cast<double>(i)));
        closes[i] = price;
    }
    for (const std::size_t window : {std::size_t{50}, std::size_t{200}}) {
        const auto averages = simple_moving_average(closes, window);
        const auto reference = zpew::test::naive_rolling_mean(closes, window);
        REQUIRE(averages.size() == reference.size());
        for (std::size_t i = 0; i < reference.size(); ++i) {
            REQUIRE_THAT(averages[i],
                         WithinRel(reference[i], 4.0 * static_cast<double>(window) *
                                                     std::numeric_limits<double>::epsilon()));
        }
    }
}

TEST_CASE("the simple moving average handles short input and rejects bad input", "[returns][sma]") {
    const std::vector<double> closes{10.0, 11.0};
    REQUIRE(simple_moving_average(closes, 3).empty());
    REQUIRE(simple_moving_average(std::span<const double>{}, 2).empty());
    REQUIRE_THROWS_AS(simple_moving_average(closes, 0), zpew::InvalidArgument);
    const std::vector<double> bad{10.0, std::numeric_limits<double>::quiet_NaN()};
    REQUIRE_THROWS_AS(simple_moving_average(bad, 1), zpew::InvalidArgument);
}
