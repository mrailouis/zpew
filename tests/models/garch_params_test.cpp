#include <zpew/error.hpp>
#include <zpew/models/garch_params.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <vector>

using Catch::Matchers::WithinRel;
using zpew::models::GarchParams;

namespace {

constexpr double kFewRoundings = 16.0 * std::numeric_limits<double>::epsilon();

}

TEST_CASE("the GARCH log-likelihood matches a hand calculation", "[models][garch]") {
    const std::vector<double> returns{0.1, -0.2};
    const GarchParams params{.omega = 0.01, .alpha = 0.1, .beta = 0.8};
    const double backcast = zpew::models::mean_square(returns);
    REQUIRE_THAT(backcast, WithinRel(0.025, kFewRoundings));
    REQUIRE_THAT(zpew::models::garch_log_likelihood(returns, params, backcast),
                 WithinRel(2.667289583605484, kFewRoundings));
}

TEST_CASE("parameters survive the round trip through the unconstrained transform",
          "[models][garch]") {
    const GarchParams params{.omega = 2e-6, .alpha = 0.08, .beta = 0.9};
    const auto theta = zpew::models::theta_from_params(params);
    const GarchParams back = zpew::models::params_from_theta(theta);
    REQUIRE_THAT(back.omega, WithinRel(params.omega, kFewRoundings));
    REQUIRE_THAT(back.alpha, WithinRel(params.alpha, kFewRoundings));
    REQUIRE_THAT(back.beta, WithinRel(params.beta, kFewRoundings));
}

TEST_CASE("every unconstrained point maps to valid GARCH parameters", "[models][garch]") {
    std::vector<GarchParams> mapped;
    for (const double t1 : {-30.0, -5.0, 0.0, 5.0, 30.0}) {
        for (const double t2 : {-30.0, -5.0, 0.0, 5.0, 30.0}) {
            mapped.push_back(zpew::models::params_from_theta(std::array<double, 3>{-12.0, t1, t2}));
        }
    }
    const bool all_valid = std::ranges::all_of(mapped, [](const GarchParams& p) {
        return p.omega > 0.0 && p.alpha > 0.0 && p.beta > 0.0 && p.persistence() < 1.0;
    });
    REQUIRE(all_valid);
}

TEST_CASE("long-run variance follows from the parameters", "[models][garch]") {
    const GarchParams params{.omega = 2e-6, .alpha = 0.08, .beta = 0.9};
    REQUIRE_THAT(params.long_run_variance(), WithinRel(1e-4, kFewRoundings));
}

TEST_CASE("invalid GARCH parameters are rejected by the transform", "[models][garch]") {
    for (const GarchParams bad : {GarchParams{.omega = 0.0, .alpha = 0.1, .beta = 0.8},
                                  GarchParams{.omega = 1e-6, .alpha = 0.0, .beta = 0.8},
                                  GarchParams{.omega = 1e-6, .alpha = 0.1, .beta = 0.0},
                                  GarchParams{.omega = 1e-6, .alpha = 0.3, .beta = 0.7}}) {
        REQUIRE_THROWS_AS(zpew::models::theta_from_params(bad), zpew::InvalidArgument);
    }
    REQUIRE_THROWS_AS(zpew::models::mean_square(std::vector<double>{}), zpew::InvalidArgument);
}

TEST_CASE("a non-positive variance makes the likelihood minus infinity", "[models][garch]") {
    const std::vector<double> returns{0.1, -0.2};
    const GarchParams negative{.omega = -1.0, .alpha = 0.1, .beta = 0.8};
    REQUIRE(zpew::models::garch_log_likelihood(returns, negative, 0.025) ==
            -std::numeric_limits<double>::infinity());
}
