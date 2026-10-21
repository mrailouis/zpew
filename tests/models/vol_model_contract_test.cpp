#include <zpew/error.hpp>
#include <zpew/models/ewma_model.hpp>
#include <zpew/models/realised_vol_model.hpp>
#include <zpew/models/vol_model.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstddef>
#include <limits>
#include <random>
#include <span>
#include <vector>

namespace {

std::vector<double> simulated_returns(std::size_t count) {
    std::mt19937_64 rng{20261021U};
    std::normal_distribution<double> noise{0.0, 0.01};
    std::vector<double> returns(count);
    for (double& r : returns) {
        r = noise(rng);
    }
    return returns;
}

}

TEMPLATE_TEST_CASE("every model forecasts only after fit", "[models][contract]",
                   zpew::models::EwmaModel, zpew::models::RealisedVolModel) {
    TestType model;
    REQUIRE_THROWS_AS(model.forecast_variance(), zpew::Error);
    REQUIRE_THROWS_AS(model.update(0.01), zpew::Error);
}

TEMPLATE_TEST_CASE("fitting a prefix then updating equals fitting everything exactly",
                   "[models][contract]", zpew::models::EwmaModel, zpew::models::RealisedVolModel) {
    const auto returns = simulated_returns(300);
    const std::span<const double> all{returns};

    TestType whole;
    whole.fit(all);

    TestType incremental;
    incremental.fit(all.first(100));
    for (const double r : all.subspan(100)) {
        incremental.update(r);
    }
    REQUIRE(incremental.forecast_variance() == whole.forecast_variance());
}

TEMPLATE_TEST_CASE("a forecast never depends on returns after the fit sample", "[models][contract]",
                   zpew::models::EwmaModel, zpew::models::RealisedVolModel) {
    auto returns = simulated_returns(300);
    TestType original;
    original.fit(std::span<const double>{returns}.first(200));
    const double before = original.forecast_variance();

    for (std::size_t i = 200; i < returns.size(); ++i) {
        returns[i] = 10.0;
    }
    TestType perturbed;
    perturbed.fit(std::span<const double>{returns}.first(200));
    REQUIRE(perturbed.forecast_variance() == before);
}

TEMPLATE_TEST_CASE("refitting replaces all previous state", "[models][contract]",
                   zpew::models::EwmaModel, zpew::models::RealisedVolModel) {
    const auto returns = simulated_returns(300);
    const std::span<const double> all{returns};
    TestType fresh;
    fresh.fit(all.last(150));

    TestType reused;
    reused.fit(all.first(150));
    reused.update(0.5);
    reused.fit(all.last(150));
    REQUIRE(reused.forecast_variance() == fresh.forecast_variance());
}

TEMPLATE_TEST_CASE("forecasts are finite and positive on realistic data", "[models][contract]",
                   zpew::models::EwmaModel, zpew::models::RealisedVolModel) {
    const auto returns = simulated_returns(500);
    TestType model;
    model.fit(std::span<const double>{returns}.first(50));
    for (const double r : std::span<const double>{returns}.subspan(50)) {
        REQUIRE(std::isfinite(model.forecast_variance()));
        REQUIRE(model.forecast_variance() > 0.0);
        model.update(r);
    }
    REQUIRE_THROWS_AS(model.update(std::numeric_limits<double>::infinity()), zpew::InvalidArgument);
}

TEMPLATE_TEST_CASE("models are usable through the VolModel interface", "[models][contract]",
                   zpew::models::EwmaModel, zpew::models::RealisedVolModel) {
    const auto returns = simulated_returns(100);
    TestType concrete;
    zpew::models::VolModel& model = concrete;
    model.fit(returns);
    REQUIRE(model.forecast_variance() > 0.0);
    REQUIRE_FALSE(model.name().empty());
}
