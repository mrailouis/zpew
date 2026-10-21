#include <zpew/error.hpp>

#include <catch2/catch_test_macros.hpp>

#include <stdexcept>
#include <string>

TEST_CASE("every zpew error is catchable as zpew::Error and std::runtime_error", "[error]") {
    REQUIRE_THROWS_AS(throw zpew::DataError{"d"}, zpew::Error);
    REQUIRE_THROWS_AS(throw zpew::ValidationError{"v"}, zpew::Error);
    REQUIRE_THROWS_AS(throw zpew::InvalidArgument{"i"}, zpew::Error);
    REQUIRE_THROWS_AS(throw zpew::ConvergenceError{"c"}, zpew::Error);
    REQUIRE_THROWS_AS(throw zpew::Error{"e"}, std::runtime_error);
}

TEST_CASE("errors keep their message", "[error]") {
    const zpew::DataError error{"snapshot.csv:3: bad date"};
    REQUIRE(std::string{error.what()} == "snapshot.csv:3: bad date");
}
