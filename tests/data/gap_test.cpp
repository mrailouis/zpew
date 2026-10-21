#include "data/bar_builders.hpp"

#include <zpew/data/bar.hpp>
#include <zpew/data/gap.hpp>
#include <zpew/data/trading_calendar.hpp>
#include <zpew/error.hpp>

#include <catch2/catch_test_macros.hpp>

#include <vector>

using zpew::data::Bar;
using zpew::data::find_gaps;
using zpew::data::Gap;
using zpew::data::TradingCalendar;
using zpew::test::bar_on;
using zpew::test::day;

TEST_CASE("a weekend is not a gap on the weekdays calendar", "[data][gap]") {
    const std::vector<Bar> bars{bar_on(day(2024, 1, 5)), bar_on(day(2024, 1, 8))};
    REQUIRE(find_gaps(bars, TradingCalendar::kWeekdays).empty());
}

TEST_CASE("up to two missing weekdays are tolerated as holidays", "[data][gap]") {
    const std::vector<Bar> one{bar_on(day(2024, 1, 8)), bar_on(day(2024, 1, 10))};
    const std::vector<Bar> two{bar_on(day(2024, 1, 8)), bar_on(day(2024, 1, 11))};
    REQUIRE(find_gaps(one, TradingCalendar::kWeekdays).empty());
    REQUIRE(find_gaps(two, TradingCalendar::kWeekdays).empty());
}

TEST_CASE("three or more missing weekdays are reported with their extent", "[data][gap]") {
    const std::vector<Bar> bars{bar_on(day(2024, 1, 8)), bar_on(day(2024, 1, 12)),
                                bar_on(day(2024, 1, 15)), bar_on(day(2024, 1, 24))};
    REQUIRE(find_gaps(bars, TradingCalendar::kWeekdays) ==
            std::vector<Gap>{Gap{.last_before = day(2024, 1, 8),
                                 .first_after = day(2024, 1, 12),
                                 .missing_sessions = 3},
                             Gap{.last_before = day(2024, 1, 15),
                                 .first_after = day(2024, 1, 24),
                                 .missing_sessions = 6}});
}

TEST_CASE("any missing day is a gap on the daily calendar", "[data][gap]") {
    const std::vector<Bar> contiguous{bar_on(day(2024, 1, 6)), bar_on(day(2024, 1, 7))};
    const std::vector<Bar> missing{bar_on(day(2024, 1, 6)), bar_on(day(2024, 1, 8))};
    REQUIRE(find_gaps(contiguous, TradingCalendar::kDaily).empty());
    REQUIRE(find_gaps(missing, TradingCalendar::kDaily) ==
            std::vector<Gap>{Gap{.last_before = day(2024, 1, 6),
                                 .first_after = day(2024, 1, 8),
                                 .missing_sessions = 1}});
}

TEST_CASE("find_gaps refuses unsorted input", "[data][gap]") {
    const std::vector<Bar> bars{bar_on(day(2024, 1, 8)), bar_on(day(2024, 1, 5))};
    REQUIRE_THROWS_AS(find_gaps(bars, TradingCalendar::kWeekdays), zpew::InvalidArgument);
}

TEST_CASE("empty and single-bar inputs have no gaps", "[data][gap]") {
    REQUIRE(find_gaps({}, TradingCalendar::kDaily).empty());
    const std::vector<Bar> one{bar_on(day(2024, 1, 8))};
    REQUIRE(find_gaps(one, TradingCalendar::kDaily).empty());
}
