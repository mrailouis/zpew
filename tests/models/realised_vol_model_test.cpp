#include <zpew/error.hpp>
#include <zpew/models/realised_vol_model.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <limits>
#include <vector>

using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::WithinRel;
using zpew::models::RealisedVolModel;

namespace {

constexpr double kFewRoundings = 8.0 * std::numeric_limits<double>::epsilon();

}

TEST_CASE("realised volatility forecasts the mean square of the last window", "[models][rv]") {
    RealisedVolModel model{2};
    const std::vector<double> returns{0.5, 0.01, -0.02};
    model.fit(returns);
    REQUIRE_THAT(model.forecast_variance(), WithinRel(2.5e-4, kFewRoundings));
    model.update(0.03);
    REQUIRE_THAT(model.forecast_variance(), WithinRel(6.5e-4, kFewRoundings));
    REQUIRE(model.name() == "RealisedVol(window=2)");
}

TEST_CASE("realised volatility rejects bad windows and inputs", "[models][rv]") {
    REQUIRE_THROWS_AS(RealisedVolModel{0}, zpew::InvalidArgument);
    RealisedVolModel model{3};
    const std::vector<double> short_input{0.01, 0.02};
    REQUIRE_THROWS_WITH(model.fit(short_input), ContainsSubstring("at least 3 returns"));
    const std::vector<double> early_nan{std::numeric_limits<double>::quiet_NaN(), 0.01, 0.02, 0.03};
    REQUIRE_THROWS_AS(model.fit(early_nan), zpew::InvalidArgument);
}

TEST_CASE("a window of zero returns cannot produce a forecast", "[models][rv]") {
    RealisedVolModel model{2};
    const std::vector<double> zeros{0.01, 0.0, 0.0};
    model.fit(zeros);
    REQUIRE_THROWS_WITH(model.forecast_variance(), ContainsSubstring("is zero"));
}
