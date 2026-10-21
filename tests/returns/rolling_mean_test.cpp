#include "returns/naive_reference.hpp"

#include <zpew/error.hpp>
#include <zpew/returns/rolling_mean.hpp>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cstddef>
#include <limits>
#include <random>
#include <vector>

using Catch::Matchers::WithinRel;
using zpew::returns::RollingMean;

namespace {

double tolerance_for_window(std::size_t window) {
    return 4.0 * static_cast<double>(window) * std::numeric_limits<double>::epsilon();
}

std::vector<double> push_all(RollingMean& rolling, const std::vector<double>& values) {
    std::vector<double> means;
    for (const double value : values) {
        rolling.push(value);
        if (rolling.full()) {
            means.push_back(rolling.mean());
        }
    }
    return means;
}

}

TEST_CASE("the rolling mean matches hand-calculated values", "[returns][rolling]") {
    RollingMean rolling{2};
    REQUIRE_FALSE(rolling.full());
    rolling.push(1e-4);
    REQUIRE_FALSE(rolling.full());
    rolling.push(4e-4);
    REQUIRE(rolling.full());
    REQUIRE_THAT(rolling.mean(), WithinRel(2.5e-4, tolerance_for_window(2)));
    rolling.push(9e-4);
    REQUIRE_THAT(rolling.mean(), WithinRel(6.5e-4, tolerance_for_window(2)));
}

TEST_CASE("the rolling mean matches the naive reference on random squared returns",
          "[returns][rolling]") {
    const std::size_t window = GENERATE(std::size_t{1}, std::size_t{21}, std::size_t{63});
    CAPTURE(window);
    std::mt19937_64 rng{20261021U};
    std::student_t_distribution<double> fat_tails{3.0};
    std::vector<double> returns(5000);
    for (double& value : returns) {
        value = 0.01 * fat_tails(rng);
    }
    const auto squares = zpew::test::squares_of(returns);

    RollingMean rolling{window};
    const auto streamed = push_all(rolling, squares);
    const auto reference = zpew::test::naive_rolling_mean(squares, window);
    REQUIRE(streamed.size() == reference.size());
    for (std::size_t i = 0; i < reference.size(); ++i) {
        CAPTURE(i);
        REQUIRE_THAT(streamed[i], WithinRel(reference[i], tolerance_for_window(window)));
    }
}

TEST_CASE("a huge value leaving the window does not leave cancellation error behind",
          "[returns][rolling]") {
    constexpr std::size_t kWindow = 21;
    std::vector<double> values(kWindow, 1e-6);
    values.push_back(1e12);
    values.insert(values.end(), 3 * kWindow, 1e-6);

    RollingMean rolling{kWindow};
    const auto streamed = push_all(rolling, values);
    const auto reference = zpew::test::naive_rolling_mean(values, kWindow);
    for (std::size_t i = 0; i < reference.size(); ++i) {
        CAPTURE(i);
        REQUIRE_THAT(streamed[i], WithinRel(reference[i], tolerance_for_window(kWindow)));
    }
    REQUIRE_THAT(streamed.back(), WithinRel(1e-6, tolerance_for_window(kWindow)));
}

TEST_CASE("zero inputs give an exact zero mean", "[returns][rolling]") {
    RollingMean rolling{3};
    for (int i = 0; i < 10; ++i) {
        rolling.push(i < 5 ? 0.25 : 0.0);
    }
    REQUIRE(rolling.mean() == 0.0);
}

TEST_CASE("invalid windows, inputs and early reads fail loudly", "[returns][rolling]") {
    REQUIRE_THROWS_AS(RollingMean{0}, zpew::InvalidArgument);
    RollingMean rolling{2};
    REQUIRE_THROWS_AS(rolling.mean(), zpew::Error);
    REQUIRE_THROWS_AS(rolling.push(std::numeric_limits<double>::quiet_NaN()),
                      zpew::InvalidArgument);
    REQUIRE_THROWS_AS(rolling.push(std::numeric_limits<double>::infinity()), zpew::InvalidArgument);
    REQUIRE_THROWS_AS(rolling.push(-1e-9), zpew::InvalidArgument);
    REQUIRE_FALSE(rolling.full());
}
