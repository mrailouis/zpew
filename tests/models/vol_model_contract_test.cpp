#include "models/garch_simulation.hpp"

#include <zpew/error.hpp>
#include <zpew/models/ewma_model.hpp>
#include <zpew/models/garch_model.hpp>
#include <zpew/models/realised_vol_model.hpp>
#include <zpew/models/vol_model.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstddef>
#include <limits>
#include <span>
#include <vector>

namespace {

constexpr std::size_t kSampleLength = 1500;
constexpr std::size_t kFitLength = 600;

std::vector<double> sample_returns() {
    return zpew::test::simulate_garch({.omega = 2e-6, .alpha = 0.08, .beta = 0.90}, kSampleLength,
                                      20261021U);
}

}

TEMPLATE_TEST_CASE("every model forecasts only after fit", "[models][contract]",
                   zpew::models::EwmaModel, zpew::models::RealisedVolModel,
                   zpew::models::GarchModel) {
    TestType model;
    REQUIRE_THROWS_AS(model.forecast_variance(), zpew::Error);
    REQUIRE_THROWS_AS(model.update(0.01), zpew::Error);
}

TEMPLATE_TEST_CASE("for parameter-free models, fitting a prefix then updating equals fitting "
                   "everything exactly",
                   "[models][contract]", zpew::models::EwmaModel, zpew::models::RealisedVolModel) {
    const auto returns = sample_returns();
    const std::span<const double> all{returns};

    TestType whole;
    whole.fit(all);

    TestType incremental;
    incremental.fit(all.first(kFitLength));
    for (const double r : all.subspan(kFitLength)) {
        incremental.update(r);
    }
    REQUIRE(incremental.forecast_variance() == whole.forecast_variance());
}

TEMPLATE_TEST_CASE("a forecast never depends on returns after the fit sample", "[models][contract]",
                   zpew::models::EwmaModel, zpew::models::RealisedVolModel,
                   zpew::models::GarchModel) {
    auto returns = sample_returns();
    TestType original;
    original.fit(std::span<const double>{returns}.first(kFitLength));
    const double before = original.forecast_variance();

    for (std::size_t i = kFitLength; i < returns.size(); ++i) {
        returns[i] = 10.0;
    }
    TestType perturbed;
    perturbed.fit(std::span<const double>{returns}.first(kFitLength));
    REQUIRE(perturbed.forecast_variance() == before);
}

TEMPLATE_TEST_CASE("refitting replaces all previous state", "[models][contract]",
                   zpew::models::EwmaModel, zpew::models::RealisedVolModel,
                   zpew::models::GarchModel) {
    const auto returns = sample_returns();
    const std::span<const double> all{returns};
    TestType fresh;
    fresh.fit(all.last(kFitLength));

    TestType reused;
    reused.fit(all.first(kFitLength));
    reused.update(0.5);
    reused.fit(all.last(kFitLength));
    REQUIRE(reused.forecast_variance() == fresh.forecast_variance());
}

TEMPLATE_TEST_CASE("forecasts are finite and positive on realistic data", "[models][contract]",
                   zpew::models::EwmaModel, zpew::models::RealisedVolModel,
                   zpew::models::GarchModel) {
    const auto returns = sample_returns();
    TestType model;
    model.fit(std::span<const double>{returns}.first(kFitLength));
    for (const double r : std::span<const double>{returns}.subspan(kFitLength)) {
        REQUIRE(std::isfinite(model.forecast_variance()));
        REQUIRE(model.forecast_variance() > 0.0);
        model.update(r);
    }
    REQUIRE_THROWS_AS(model.update(std::numeric_limits<double>::infinity()), zpew::InvalidArgument);
}

TEMPLATE_TEST_CASE("models are usable through the VolModel interface", "[models][contract]",
                   zpew::models::EwmaModel, zpew::models::RealisedVolModel,
                   zpew::models::GarchModel) {
    const auto returns = sample_returns();
    TestType concrete;
    zpew::models::VolModel& model = concrete;
    model.fit(returns);
    REQUIRE(model.forecast_variance() > 0.0);
    REQUIRE_FALSE(model.name().empty());
}
