#include "returns/naive_reference.hpp"

#include <zpew/error.hpp>
#include <zpew/returns/realised_variance.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <cstddef>
#include <limits>
#include <span>
#include <vector>

using Catch::Matchers::WithinRel;
using zpew::returns::annualise_volatility;
using zpew::returns::rolling_realised_variance;

namespace {

constexpr double kFewRoundings = 8.0 * std::numeric_limits<double>::epsilon();

}

TEST_CASE("rolling realised variance has one value per full window", "[returns][variance]") {
    const std::vector<double> returns{0.01, -0.02, 0.03, 0.0};
    const auto variances = rolling_realised_variance(returns, 2);
    REQUIRE(variances.size() == 3);
    REQUIRE_THAT(variances[0], WithinRel(2.5e-4, kFewRoundings));
    REQUIRE_THAT(variances[1], WithinRel(6.5e-4, kFewRoundings));
    REQUIRE_THAT(variances[2], WithinRel(4.5e-4, kFewRoundings));
}

TEST_CASE("rolling realised variance agrees with the naive reference", "[returns][variance]") {
    std::vector<double> returns(500);
    for (std::size_t i = 0; i < returns.size(); ++i) {
        const auto x = static_cast<double>(i);
        returns[i] = (0.01 * std::sin(0.37 * x)) + (0.002 * std::cos(1.9 * x));
    }
    for (const std::size_t window : {std::size_t{21}, std::size_t{63}}) {
        const auto variances = rolling_realised_variance(returns, window);
        const auto reference = zpew::test::naive_rolling_mean_of_squares(returns, window);
        REQUIRE(variances.size() == reference.size());
        for (std::size_t i = 0; i < reference.size(); ++i) {
            REQUIRE_THAT(variances[i],
                         WithinRel(reference[i], 4.0 * static_cast<double>(window) *
                                                     std::numeric_limits<double>::epsilon()));
        }
    }
}

TEST_CASE("a window longer than the data gives no values", "[returns][variance]") {
    const std::vector<double> returns{0.01, 0.02};
    REQUIRE(rolling_realised_variance(returns, 3).empty());
    REQUIRE(rolling_realised_variance(std::span<const double>{}, 1).empty());
}

TEST_CASE("rolling realised variance rejects a zero window and non-finite returns",
          "[returns][variance]") {
    const std::vector<double> returns{0.01, std::numeric_limits<double>::quiet_NaN()};
    REQUIRE_THROWS_AS(rolling_realised_variance(returns, 0), zpew::InvalidArgument);
    REQUIRE_THROWS_AS(rolling_realised_variance(returns, 1), zpew::InvalidArgument);
}

TEST_CASE("annualised volatility scales daily variance by the square root of time",
          "[returns][annualise]") {
    REQUIRE_THAT(annualise_volatility(1e-4), WithinRel(std::sqrt(0.0252), kFewRoundings));
    REQUIRE_THAT(annualise_volatility(1e-4, 365.0), WithinRel(std::sqrt(0.0365), kFewRoundings));
    REQUIRE(annualise_volatility(0.0) == 0.0);
}

TEST_CASE("annualisation rejects invalid inputs", "[returns][annualise]") {
    REQUIRE_THROWS_AS(annualise_volatility(-1e-4), zpew::InvalidArgument);
    REQUIRE_THROWS_AS(annualise_volatility(std::numeric_limits<double>::infinity()),
                      zpew::InvalidArgument);
    REQUIRE_THROWS_AS(annualise_volatility(1e-4, 0.0), zpew::InvalidArgument);
}
