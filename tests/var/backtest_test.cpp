#include <zpew/error.hpp>
#include <zpew/var/backtest.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <limits>
#include <vector>

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using zpew::var::backtest;
using zpew::var::kupiec_test;

namespace {

constexpr double kZ95 = 1.644853626951472;
constexpr double kScipyAgreement = 1e-12;

}

TEST_CASE("VaR is the normal quantile times the forecast volatility", "[var]") {
    REQUIRE_THAT(zpew::var::value_at_risk(1e-4, 0.95), WithinRel(kZ95 * 0.01, 1e-15));
    REQUIRE_THROWS_AS(zpew::var::value_at_risk(0.0, 0.95), zpew::InvalidArgument);
    REQUIRE_THROWS_AS(zpew::var::value_at_risk(1e-4, 0.4), zpew::InvalidArgument);
    REQUIRE_THROWS_AS(zpew::var::value_at_risk(1e-4, 1.0), zpew::InvalidArgument);
}

TEST_CASE("a breach is a return strictly below minus VaR", "[var]") {
    const double var = zpew::var::value_at_risk(1e-4, 0.95);
    const std::vector<double> variances(4, 1e-4);
    const std::vector<double> returns{-0.03, -var, 0.05, -var - 1e-12};
    const auto result = backtest(variances, returns, 0.95);
    REQUIRE(result.observations == 4);
    REQUIRE(result.breaches == 2);
    REQUIRE(result.breach_rate == 0.5);
    REQUIRE_THAT(result.expected_rate, WithinAbs(0.05, 1e-15));
}

TEST_CASE("the Kupiec test matches scipy reference values", "[var][kupiec]") {
    const auto none = kupiec_test(250, 0, 0.05);
    REQUIRE_THAT(none.likelihood_ratio, WithinRel(25.64664719377529, kScipyAgreement));
    REQUIRE_THAT(none.p_value, WithinRel(4.1000723664166635e-07, 1e-9));

    const auto twenty = kupiec_test(250, 20, 0.05);
    REQUIRE_THAT(twenty.likelihood_ratio, WithinRel(4.039520476139202, kScipyAgreement));
    REQUIRE_THAT(twenty.p_value, WithinRel(0.04444644930605564, 1e-9));

    const auto all = kupiec_test(500, 500, 0.05);
    REQUIRE_THAT(all.likelihood_ratio, WithinRel(2995.732273553991, kScipyAgreement));
    REQUIRE(all.p_value == 0.0);
}

TEST_CASE("an exact breach rate gives a likelihood ratio of zero and p-value of one",
          "[var][kupiec]") {
    const auto exact = kupiec_test(1000, 50, 0.05);
    REQUIRE_THAT(exact.likelihood_ratio, WithinAbs(0.0, 1e-12));
    REQUIRE_THAT(exact.p_value, WithinAbs(1.0, 1e-6));
}

TEST_CASE("backtest inputs are validated", "[var]") {
    const std::vector<double> one{1e-4};
    const std::vector<double> two{0.01, 0.02};
    REQUIRE_THROWS_AS(backtest(one, two, 0.95), zpew::InvalidArgument);
    REQUIRE_THROWS_AS(backtest(std::vector<double>{}, std::vector<double>{}, 0.95),
                      zpew::InvalidArgument);
    const std::vector<double> nan_return{std::numeric_limits<double>::quiet_NaN()};
    REQUIRE_THROWS_AS(backtest(one, nan_return, 0.95), zpew::InvalidArgument);
    REQUIRE_THROWS_AS(kupiec_test(10, 11, 0.05), zpew::InvalidArgument);
    REQUIRE_THROWS_AS(kupiec_test(10, 1, 0.0), zpew::InvalidArgument);
}
