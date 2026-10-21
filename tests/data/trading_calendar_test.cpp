#include "data/bar_builders.hpp"

#include <zpew/data/trading_calendar.hpp>

#include <catch2/catch_test_macros.hpp>

using zpew::data::is_session;
using zpew::data::parse_trading_calendar;
using zpew::data::sessions_between;
using zpew::data::TradingCalendar;
using zpew::test::day;

TEST_CASE("calendar names parse and print", "[data][calendar]") {
    REQUIRE(parse_trading_calendar("weekdays") == TradingCalendar::kWeekdays);
    REQUIRE(parse_trading_calendar("daily") == TradingCalendar::kDaily);
    REQUIRE_FALSE(parse_trading_calendar("Weekdays").has_value());
    REQUIRE_FALSE(parse_trading_calendar("").has_value());
    REQUIRE(to_string(TradingCalendar::kWeekdays) == "weekdays");
    REQUIRE(to_string(TradingCalendar::kDaily) == "daily");
}

TEST_CASE("weekend days are sessions only on the daily calendar", "[data][calendar]") {
    const auto friday = day(2024, 1, 5);
    const auto saturday = day(2024, 1, 6);
    const auto sunday = day(2024, 1, 7);
    REQUIRE(is_session(friday, TradingCalendar::kWeekdays));
    REQUIRE_FALSE(is_session(saturday, TradingCalendar::kWeekdays));
    REQUIRE_FALSE(is_session(sunday, TradingCalendar::kWeekdays));
    REQUIRE(is_session(saturday, TradingCalendar::kDaily));
    REQUIRE(is_session(sunday, TradingCalendar::kDaily));
}

TEST_CASE("sessions_between counts sessions strictly between two dates", "[data][calendar]") {
    const auto monday = day(2024, 1, 8);
    REQUIRE(sessions_between(day(2024, 1, 5), monday, TradingCalendar::kWeekdays) == 0);
    REQUIRE(sessions_between(day(2024, 1, 5), monday, TradingCalendar::kDaily) == 2);
    REQUIRE(sessions_between(day(2024, 1, 5), day(2024, 1, 10), TradingCalendar::kWeekdays) == 2);
    REQUIRE(sessions_between(monday, day(2024, 1, 9), TradingCalendar::kWeekdays) == 0);
    REQUIRE(sessions_between(monday, monday, TradingCalendar::kDaily) == 0);
    REQUIRE(sessions_between(day(2023, 12, 29), day(2024, 1, 2), TradingCalendar::kWeekdays) == 1);
}
