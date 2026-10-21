#include <zpew/error.hpp>
#include <zpew/var/normal_quantile.hpp>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <limits>

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using zpew::var::normal_quantile;

namespace {

constexpr double kAs241RelativeAccuracy = 1e-15;

}

TEST_CASE("normal quantiles match scipy across the central, intermediate and tail regions",
          "[var][quantile]") {
    struct Reference {
        double probability;
        double quantile;
    };
    const auto ref =
        GENERATE(Reference{0.95, 1.644853626951472}, Reference{0.99, 2.3263478740408408},
                 Reference{0.975, 1.959963984540054}, Reference{0.001, -3.090232306167813},
                 Reference{1e-10, -6.361340902404056}, Reference{0.999999, 4.753424308817087},
                 Reference{0.02425, -1.972961051311885});
    CAPTURE(ref.probability);
    REQUIRE_THAT(normal_quantile(ref.probability), WithinRel(ref.quantile, kAs241RelativeAccuracy));
}

TEST_CASE("the normal quantile is zero at the median and antisymmetric", "[var][quantile]") {
    REQUIRE(normal_quantile(0.5) == 0.0);
    for (const double p : {0.6, 0.9, 0.99, 0.999999}) {
        REQUIRE_THAT(normal_quantile(1.0 - p),
                     WithinAbs(-normal_quantile(p), 4.0 * std::numeric_limits<double>::epsilon()));
    }
}

TEST_CASE("the normal quantile rejects probabilities outside (0, 1)", "[var][quantile]") {
    for (const double bad : {0.0, 1.0, -0.1, 1.1, std::numeric_limits<double>::quiet_NaN()}) {
        REQUIRE_THROWS_AS(normal_quantile(bad), zpew::InvalidArgument);
    }
}
