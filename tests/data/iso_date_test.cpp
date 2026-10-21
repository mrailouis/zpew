#include "data/bar_builders.hpp"

#include <zpew/data/iso_date.hpp>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <string_view>

using zpew::data::format_iso_date;
using zpew::data::parse_iso_date;
using zpew::test::day;

TEST_CASE("parse_iso_date accepts valid calendar dates", "[data][date]") {
    REQUIRE(parse_iso_date("2024-01-05") == day(2024, 1, 5));
    REQUIRE(parse_iso_date("2024-02-29") == day(2024, 2, 29));
    REQUIRE(parse_iso_date("2000-02-29") == day(2000, 2, 29));
    REQUIRE(parse_iso_date("1999-12-31") == day(1999, 12, 31));
}

TEST_CASE("parse_iso_date rejects malformed and impossible dates", "[data][date]") {
    const std::string_view text = GENERATE(
        as<std::string_view>{}, "", "2024-1-05", "2024-01-5", "2024/01/05", " 2024-01-05",
        "2024-01-05 ", "+024-01-05", "-024-01-05", "2024-00-10", "2024-13-01", "2024-01-00",
        "2024-01-32", "2023-02-29", "1900-02-29", "20240105xx", "2024-0a-05", "2024--1-05");
    CAPTURE(text);
    REQUIRE_FALSE(parse_iso_date(text).has_value());
}

TEST_CASE("format_iso_date pads fields and round-trips", "[data][date]") {
    REQUIRE(format_iso_date(day(2024, 1, 5)) == "2024-01-05");
    REQUIRE(format_iso_date(day(987, 3, 9)) == "0987-03-09");
    const auto date = day(2020, 2, 29);
    REQUIRE(parse_iso_date(format_iso_date(date)) == date);
}
