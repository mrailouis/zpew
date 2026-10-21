#include <zpew/error.hpp>
#include <zpew/optimiser/nelder_mead.hpp>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <cstddef>
#include <limits>
#include <span>
#include <vector>

using Catch::Matchers::WithinAbs;
using zpew::optimiser::nelder_mead;

namespace {

double rosenbrock(std::span<const double> x) {
    const double a = 1.0 - x[0];
    const double b = x[1] - (x[0] * x[0]);
    return (a * a) + (100.0 * b * b);
}

constexpr double kLocationTolerance = 1e-6;

}

TEST_CASE("Nelder-Mead finds the Rosenbrock minimum from the classic start", "[optimiser]") {
    const std::vector<double> start{-1.2, 1.0};
    const auto result = nelder_mead(rosenbrock, start);
    REQUIRE(result.converged);
    REQUIRE_THAT(result.x[0], WithinAbs(1.0, kLocationTolerance));
    REQUIRE_THAT(result.x[1], WithinAbs(1.0, kLocationTolerance));
    REQUIRE(result.f < 1e-12);
}

TEST_CASE("Nelder-Mead finds shifted quadratic minima in several dimensions", "[optimiser]") {
    const std::size_t dimensions = GENERATE(std::size_t{1}, std::size_t{3}, std::size_t{5});
    CAPTURE(dimensions);
    std::vector<double> centre(dimensions);
    for (std::size_t i = 0; i < dimensions; ++i) {
        centre[i] = static_cast<double>(i) - 1.5;
    }
    const auto quadratic = [&](std::span<const double> x) {
        double sum = 0.0;
        for (std::size_t i = 0; i < x.size(); ++i) {
            const double d = x[i] - centre[i];
            sum += static_cast<double>(i + 1) * d * d;
        }
        return 7.0 + sum;
    };
    const std::vector<double> start(dimensions, 2.0);
    const auto result = nelder_mead(quadratic, start);
    REQUIRE(result.converged);
    REQUIRE_THAT(result.f, WithinAbs(7.0, 1e-10));
    for (std::size_t i = 0; i < dimensions; ++i) {
        REQUIRE_THAT(result.x[i], WithinAbs(centre[i], kLocationTolerance));
    }
}

TEST_CASE("Nelder-Mead treats non-finite objective values as infinitely bad", "[optimiser]") {
    const auto log_barrier = [](std::span<const double> x) {
        return x[0] <= 0.0 ? std::numeric_limits<double>::quiet_NaN() : x[0] - std::log(x[0]);
    };
    const std::vector<double> start{0.05};
    const auto result = nelder_mead(log_barrier, start, {.initial_step = 0.5});
    REQUIRE(result.converged);
    REQUIRE_THAT(result.x[0], WithinAbs(1.0, kLocationTolerance));
}

TEST_CASE("Nelder-Mead reports non-convergence at the iteration cap", "[optimiser]") {
    const std::vector<double> start{-1.2, 1.0};
    const auto result = nelder_mead(rosenbrock, start, {.max_iterations = 10});
    REQUIRE_FALSE(result.converged);
    REQUIRE(result.iterations == 10);
}

TEST_CASE("Nelder-Mead is deterministic", "[optimiser]") {
    const std::vector<double> start{-1.2, 1.0};
    const auto first = nelder_mead(rosenbrock, start);
    const auto second = nelder_mead(rosenbrock, start);
    REQUIRE(first.x == second.x);
    REQUIRE(first.iterations == second.iterations);
}

TEST_CASE("Nelder-Mead rejects invalid input", "[optimiser]") {
    const std::vector<double> start{1.0};
    REQUIRE_THROWS_AS(nelder_mead(rosenbrock, std::span<const double>{}), zpew::InvalidArgument);
    const std::vector<double> nan_start{std::numeric_limits<double>::quiet_NaN()};
    REQUIRE_THROWS_AS(nelder_mead(rosenbrock, nan_start), zpew::InvalidArgument);
    REQUIRE_THROWS_AS(nelder_mead(rosenbrock, start, {.initial_step = 0.0}), zpew::InvalidArgument);
    REQUIRE_THROWS_AS(nelder_mead(rosenbrock, start, {.max_iterations = 0}), zpew::InvalidArgument);
}
