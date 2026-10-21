#include "data/bar_builders.hpp"

#include <zpew/data/bar.hpp>
#include <zpew/data/snapshot_csv.hpp>
#include <zpew/error.hpp>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <string>
#include <string_view>
#include <vector>

using Catch::Matchers::ContainsSubstring;
using zpew::data::Bar;
using zpew::data::parse_snapshot_csv;
using zpew::test::day;

namespace {

std::vector<Bar> expected_bars() {
    return {Bar{.date = day(2024, 1, 4),
                .open = 100.25,
                .high = 101.5,
                .low = 99.75,
                .close = 101.0,
                .volume = 1200.0},
            Bar{.date = day(2024, 1, 5),
                .open = 101.0,
                .high = 102.0,
                .low = 100.5,
                .close = 100.75,
                .volume = 0.0}};
}

std::string with_line_ending(std::string_view ending, bool trailing) {
    std::string text = "date,open,high,low,close,volume";
    text += ending;
    text += "2024-01-04,100.25,101.5,99.75,101,1200";
    text += ending;
    text += "2024-01-05,101,102,100.5,100.75,0";
    if (trailing) {
        text += ending;
    }
    return text;
}

}

TEST_CASE("parsed values equal the correctly rounded literals for LF and CRLF files",
          "[data][csv]") {
    const std::string_view ending = GENERATE(as<std::string_view>{}, "\n", "\r\n");
    const bool trailing = GENERATE(true, false);
    CAPTURE(ending, trailing);
    REQUIRE(parse_snapshot_csv(with_line_ending(ending, trailing), "test.csv") == expected_bars());
}

TEST_CASE("a header-only snapshot parses to no bars", "[data][csv]") {
    REQUIRE(parse_snapshot_csv("date,open,high,low,close,volume\n", "test.csv").empty());
}

TEST_CASE("scientific notation and negative values parse; validation judges them later",
          "[data][csv]") {
    const auto bars = parse_snapshot_csv(
        "date,open,high,low,close,volume\n2024-01-04,1e2,1.5e2,-1,1E2,0\n", "test.csv");
    REQUIRE(bars.size() == 1);
    REQUIRE(bars[0].open == 100.0);
    REQUIRE(bars[0].high == 150.0);
    REQUIRE(bars[0].low == -1.0);
}

TEST_CASE("structural problems fail with the source name", "[data][csv]") {
    REQUIRE_THROWS_WITH(parse_snapshot_csv("", "spy.csv"),
                        ContainsSubstring("spy.csv: missing header"));
    REQUIRE_THROWS_WITH(parse_snapshot_csv("\xEF\xBB\xBF"
                                           "date,open,high,low,close,volume\n",
                                           "spy.csv"),
                        ContainsSubstring("byte-order mark"));
    REQUIRE_THROWS_WITH(parse_snapshot_csv("Date,Open,High,Low,Close,Volume\n", "spy.csv"),
                        ContainsSubstring("spy.csv:1: expected header"));
    REQUIRE_THROWS_WITH(parse_snapshot_csv("date,open,high,low,close\n", "spy.csv"),
                        ContainsSubstring("spy.csv:1:"));
}

TEST_CASE("malformed rows fail with the line number", "[data][csv]") {
    struct Case {
        std::string_view row;
        std::string_view message;
    };
    const auto c = GENERATE(
        Case{"", "spy.csv:2: blank line"},
        Case{"2024-01-04,1,2,1,1", "spy.csv:2: expected 6 comma-separated fields"},
        Case{"2024-01-04,1,2,1,1,1,1", "spy.csv:2: expected 6 comma-separated fields"},
        Case{"04/01/2024,1,2,1,1,1", "spy.csv:2: invalid date '04/01/2024'"},
        Case{"2024-01-04,abc,2,1,1,1", "spy.csv:2: invalid number 'abc' in column open"},
        Case{"2024-01-04,1,,1,1,1", "spy.csv:2: invalid number '' in column high"},
        Case{"2024-01-04,1,2, 1,1,1", "spy.csv:2: invalid number ' 1' in column low"},
        Case{"2024-01-04,1,2,1,1 ,1", "spy.csv:2: invalid number '1 ' in column close"},
        Case{"2024-01-04,1,2,1,1,+5", "spy.csv:2: invalid number '+5' in column volume"},
        Case{"2024-01-04,1,2,1,1,0x10", "spy.csv:2: invalid number '0x10' in column volume"},
        Case{"2024-01-04,\"1\",2,1,1,1", "spy.csv:2: invalid number '\"1\"' in column open"});
    CAPTURE(c.row);
    const std::string text =
        "date,open,high,low,close,volume\n" + std::string{c.row} + "\n2024-01-05,1,2,1,1,1\n";
    REQUIRE_THROWS_AS(parse_snapshot_csv(text, "spy.csv"), zpew::DataError);
    REQUIRE_THROWS_WITH(parse_snapshot_csv(text, "spy.csv"),
                        ContainsSubstring(std::string{c.message}));
}

TEST_CASE("an extra blank line at the end is malformed", "[data][csv]") {
    REQUIRE_THROWS_WITH(
        parse_snapshot_csv("date,open,high,low,close,volume\n2024-01-04,1,2,1,1,1\n\n", "spy.csv"),
        ContainsSubstring("spy.csv:3: blank line"));
}

TEST_CASE("a bare carriage return inside a row is malformed", "[data][csv]") {
    REQUIRE_THROWS_AS(
        parse_snapshot_csv("date,open,high,low,close,volume\n2024-01-04,1,2\r,1,1,1\n", "spy.csv"),
        zpew::DataError);
}
