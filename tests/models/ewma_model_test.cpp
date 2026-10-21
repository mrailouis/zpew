#include <zpew/error.hpp>
#include <zpew/models/ewma_model.hpp>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <limits>
#include <vector>

using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::WithinRel;
using zpew::models::EwmaModel;

namespace {

constexpr double kFewRoundings = 8.0 * std::numeric_limits<double>::epsilon();

}

TEST_CASE("EWMA matches a hand-calculated sequence", "[models][ewma]") {
    EwmaModel model{0.5, 2};
    const std::vector<double> returns{0.1, 0.3, 0.2};
    model.fit(returns);
    REQUIRE_THAT(model.forecast_variance(), WithinRel(0.045, kFewRoundings));
    model.update(0.0);
    REQUIRE_THAT(model.forecast_variance(), WithinRel(0.0225, kFewRoundings));
    model.update(-0.4);
    REQUIRE_THAT(model.forecast_variance(), WithinRel(0.09125, kFewRoundings));
}

TEST_CASE("EWMA seeds with the mean square of the seed window", "[models][ewma]") {
    EwmaModel model{0.94, 3};
    const std::vector<double> seed_only{0.01, -0.02, 0.03};
    model.fit(seed_only);
    REQUIRE_THAT(model.forecast_variance(), WithinRel(14e-4 / 3.0, kFewRoundings));
}

TEST_CASE("EWMA uses the RiskMetrics defaults", "[models][ewma]") {
    const EwmaModel model;
    REQUIRE(model.lambda() == 0.94);
    REQUIRE(model.seed_window() == 21);
    REQUIRE(model.name() == "EWMA(lambda=0.94, seed=21)");
}

TEST_CASE("EWMA lambda must lie strictly between zero and one", "[models][ewma]") {
    for (const double bad : {0.0, 1.0, -0.1, 1.5, std::numeric_limits<double>::quiet_NaN(),
                             std::numeric_limits<double>::infinity()}) {
        CAPTURE(bad);
        REQUIRE_THROWS_AS(EwmaModel{bad}, zpew::InvalidArgument);
    }
    REQUIRE_THROWS_AS(EwmaModel(0.94, 0), zpew::InvalidArgument);
}

TEST_CASE("EWMA accepts lambda arbitrarily close to either edge", "[models][ewma]") {
    REQUIRE_NOTHROW(EwmaModel{std::numeric_limits<double>::min()});
    REQUIRE_NOTHROW(EwmaModel{1.0 - std::numeric_limits<double>::epsilon()});
}

TEST_CASE("EWMA lambda near the edges behaves as the limits suggest", "[models][ewma]") {
    const std::vector<double> returns{0.01, 0.02, 0.05};
    EwmaModel almost_memoryless{1e-12, 1};
    almost_memoryless.fit(returns);
    REQUIRE_THAT(almost_memoryless.forecast_variance(), WithinRel(0.0025, 1e-9));

    EwmaModel almost_constant{1.0 - 1e-12, 1};
    almost_constant.fit(returns);
    REQUIRE_THAT(almost_constant.forecast_variance(), WithinRel(1e-4, 1e-9));
}

TEST_CASE("EWMA fit rejects short, all-zero and non-finite input", "[models][ewma]") {
    EwmaModel model{0.94, 3};
    const std::vector<double> short_input{0.01, 0.02};
    REQUIRE_THROWS_WITH(model.fit(short_input), ContainsSubstring("at least 3 returns"));
    const std::vector<double> zeros{0.0, 0.0, 0.0, 0.01};
    REQUIRE_THROWS_WITH(model.fit(zeros), ContainsSubstring("only zero returns"));
    const std::vector<double> nan_late{0.01, 0.02, 0.03, std::numeric_limits<double>::quiet_NaN()};
    REQUIRE_THROWS_AS(model.fit(nan_late), zpew::InvalidArgument);
}

TEST_CASE("EWMA stays positive through long runs of zero returns until it underflows",
          "[models][ewma]") {
    EwmaModel model{0.5, 1};
    const std::vector<double> seed{1.0};
    model.fit(seed);
    for (int i = 0; i < 1000; ++i) {
        model.update(0.0);
    }
    REQUIRE(model.forecast_variance() > 0.0);
    REQUIRE_THROWS_AS(
        [&] {
            for (int i = 0; i < 100; ++i) {
                model.update(0.0);
            }
        }(),
        zpew::Error);
}
