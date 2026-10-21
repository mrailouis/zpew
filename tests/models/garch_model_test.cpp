#include "models/garch_simulation.hpp"

#include <zpew/error.hpp>
#include <zpew/models/garch_model.hpp>
#include <zpew/models/garch_params.hpp>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using zpew::models::GarchModel;
using zpew::models::GarchParams;

namespace {

constexpr GarchParams kTrue{.omega = 2e-6, .alpha = 0.08, .beta = 0.90};
constexpr std::size_t kSimulationLength = 10000;
constexpr double kAlphaTolerance = 0.02;
constexpr double kBetaTolerance = 0.03;
constexpr double kPersistenceTolerance = 0.01;
constexpr double kOmegaRelativeTolerance = 0.5;

}

TEST_CASE("GARCH recovers simulated parameters within about three standard errors",
          "[models][garch][simulation]") {
    const std::uint64_t seed = GENERATE(std::uint64_t{1}, std::uint64_t{2}, std::uint64_t{3});
    CAPTURE(seed);
    const auto returns = zpew::test::simulate_garch(kTrue, kSimulationLength, seed);
    GarchModel model;
    model.fit(returns);
    const auto& fit = model.fitted();
    CAPTURE(fit.params.omega, fit.params.alpha, fit.params.beta);
    REQUIRE_THAT(fit.params.alpha, WithinAbs(kTrue.alpha, kAlphaTolerance));
    REQUIRE_THAT(fit.params.beta, WithinAbs(kTrue.beta, kBetaTolerance));
    REQUIRE_THAT(fit.params.persistence(), WithinAbs(kTrue.persistence(), kPersistenceTolerance));
    REQUIRE_THAT(fit.params.omega, WithinRel(kTrue.omega, kOmegaRelativeTolerance));
}

TEST_CASE("the fitted likelihood is at least the likelihood at the true parameters",
          "[models][garch][simulation]") {
    const auto returns = zpew::test::simulate_garch(kTrue, 3000, 7);
    GarchModel model;
    model.fit(returns);
    const auto& fit = model.fitted();
    const double at_truth = zpew::models::garch_log_likelihood(returns, kTrue, fit.backcast);
    REQUIRE(fit.log_likelihood >= at_truth);
    REQUIRE_THAT(
        fit.log_likelihood,
        WithinRel(zpew::models::garch_log_likelihood(returns, fit.params, fit.backcast), 1e-12));
    REQUIRE(fit.converged_starts >= 1);
    REQUIRE(fit.starts == 9);
    REQUIRE(fit.observations == 3000);
}

TEST_CASE("the forecast continues the variance recursion with the fitted parameters",
          "[models][garch]") {
    const auto returns = zpew::test::simulate_garch(kTrue, 1200, 11);
    const std::span<const double> all{returns};
    GarchModel model;
    model.fit(all.first(1000));
    const auto& fit = model.fitted();

    double variance = fit.params.omega + (fit.params.persistence() * fit.backcast);
    for (const double r : all.first(1000)) {
        variance = zpew::models::garch_next_variance(fit.params, variance, r);
    }
    REQUIRE(model.forecast_variance() == variance);
    for (const double r : all.subspan(1000)) {
        model.update(r);
        variance = zpew::models::garch_next_variance(fit.params, variance, r);
    }
    REQUIRE(model.forecast_variance() == variance);
}

TEST_CASE("variance targeting fixes omega to the sample variance", "[models][garch]") {
    const auto returns = zpew::test::simulate_garch(kTrue, 3000, 5);
    GarchModel model{{.variance_targeting = true}};
    model.fit(returns);
    const auto& fit = model.fitted();
    REQUIRE_THAT(fit.params.long_run_variance(), WithinRel(fit.backcast, 1e-12));
    REQUIRE(model.name() == "GARCH(1,1, variance targeting)");

    GarchModel unrestricted;
    unrestricted.fit(returns);
    REQUIRE(unrestricted.fitted().log_likelihood >= fit.log_likelihood - 1e-9);
}

TEST_CASE("GARCH refuses short, all-zero and non-finite samples", "[models][garch]") {
    GarchModel model;
    REQUIRE_THROWS_AS(model.fitted(), zpew::Error);
    REQUIRE_THROWS_WITH(model.fit(std::vector<double>(499, 0.01)),
                        ContainsSubstring("at least 500 returns"));
    REQUIRE_THROWS_WITH(model.fit(std::vector<double>(600, 0.0)),
                        ContainsSubstring("only zero returns"));
    std::vector<double> with_nan(600, 0.01);
    with_nan[300] = std::numeric_limits<double>::quiet_NaN();
    REQUIRE_THROWS_AS(model.fit(with_nan), zpew::InvalidArgument);
    REQUIRE_THROWS_AS(GarchModel{{.minimum_fit_length = 1}}, zpew::InvalidArgument);
}

TEST_CASE("returns of constant size give a long-run variance equal to their square",
          "[models][garch]") {
    std::vector<double> constant_magnitude(800);
    for (std::size_t i = 0; i < constant_magnitude.size(); ++i) {
        constant_magnitude[i] = (i % 2 == 0) ? 0.01 : -0.01;
    }
    GarchModel model;
    model.fit(constant_magnitude);
    const auto& fit = model.fitted();
    REQUIRE_THAT(fit.params.long_run_variance(), WithinRel(1e-4, 1e-3));
}
