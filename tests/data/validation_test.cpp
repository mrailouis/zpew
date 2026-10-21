#include "data/bar_builders.hpp"

#include <zpew/data/bar.hpp>
#include <zpew/data/trading_calendar.hpp>
#include <zpew/data/validation.hpp>
#include <zpew/error.hpp>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <limits>
#include <string>
#include <vector>

using Catch::Matchers::ContainsSubstring;
using zpew::data::Bar;
using zpew::data::TradingCalendar;
using zpew::data::validate_bars;
using zpew::test::bar_on;
using zpew::test::day;

namespace {

std::vector<Bar> two_good_bars() {
    return {bar_on(day(2024, 1, 4)), bar_on(day(2024, 1, 5))};
}

void require_invalid(const std::vector<Bar>& bars, const std::string& message,
                     TradingCalendar calendar = TradingCalendar::kWeekdays) {
    REQUIRE_THROWS_AS(validate_bars(bars, calendar), zpew::ValidationError);
    REQUIRE_THROWS_WITH(validate_bars(bars, calendar), ContainsSubstring(message));
}

}

TEST_CASE("well-formed bars validate", "[data][validation]") {
    REQUIRE_NOTHROW(validate_bars(two_good_bars(), TradingCalendar::kWeekdays));
}

TEST_CASE("fewer than two bars are rejected", "[data][validation]") {
    require_invalid({}, "at least 2 bars");
    require_invalid({bar_on(day(2024, 1, 4))}, "at least 2 bars");
}

TEST_CASE("dates must be strictly increasing", "[data][validation]") {
    require_invalid({bar_on(day(2024, 1, 4)), bar_on(day(2024, 1, 4))},
                    "bar 2 (2024-01-04): date is not after the previous bar's 2024-01-04");
    require_invalid({bar_on(day(2024, 1, 5)), bar_on(day(2024, 1, 4))}, "bar 2 (2024-01-04)");
}

TEST_CASE("prices must be finite and positive", "[data][validation]") {
    constexpr double kNan = std::numeric_limits<double>::quiet_NaN();
    constexpr double kInf = std::numeric_limits<double>::infinity();
    for (const double bad : {0.0, -1.0, kNan, kInf, -kInf}) {
        auto bars = two_good_bars();
        bars[1].close = bad;
        CAPTURE(bad);
        require_invalid(bars, "bar 2 (2024-01-05): close must be finite and positive");
    }
    auto bars = two_good_bars();
    bars[0].open = 0.0;
    require_invalid(bars, "bar 1 (2024-01-04): open must be finite and positive");
}

TEST_CASE("volume must be finite and non-negative; zero is allowed", "[data][validation]") {
    auto bars = two_good_bars();
    bars[0].volume = 0.0;
    REQUIRE_NOTHROW(validate_bars(bars, TradingCalendar::kWeekdays));
    bars[0].volume = -1.0;
    require_invalid(bars, "volume must be finite and non-negative");
    bars[0].volume = std::numeric_limits<double>::quiet_NaN();
    require_invalid(bars, "volume must be finite and non-negative");
}

TEST_CASE("high and low must bracket open and close", "[data][validation]") {
    auto bars = two_good_bars();
    bars[1].high = 100.5;
    require_invalid(bars, "high 100.5 is below max(open, close) 101");

    bars = two_good_bars();
    bars[1].low = 100.5;
    require_invalid(bars, "low 100.5 is above min(open, close) 100");

    bars = two_good_bars();
    bars[1] = Bar{.date = day(2024, 1, 5),
                  .open = 100.0,
                  .high = 100.0,
                  .low = 100.0,
                  .close = 100.0,
                  .volume = 0.0};
    REQUIRE_NOTHROW(validate_bars(bars, TradingCalendar::kWeekdays));
}

TEST_CASE("weekend bars are rejected only on the weekdays calendar", "[data][validation]") {
    const std::vector<Bar> bars{bar_on(day(2024, 1, 5)), bar_on(day(2024, 1, 6))};
    require_invalid(bars, "bar 2 (2024-01-06): date is not a session of the 'weekdays' calendar");
    REQUIRE_NOTHROW(validate_bars(bars, TradingCalendar::kDaily));
}
