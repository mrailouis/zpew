#include <zpew/error.hpp>

#include <catch2/catch_test_macros.hpp>

#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>

TEST_CASE("every zpew error derives from zpew::Error and std::runtime_error", "[error]") {
    STATIC_REQUIRE(std::is_base_of_v<std::runtime_error, zpew::Error>);
    STATIC_REQUIRE(std::is_base_of_v<zpew::Error, zpew::DataError>);
    STATIC_REQUIRE(std::is_base_of_v<zpew::Error, zpew::ValidationError>);
    STATIC_REQUIRE(std::is_base_of_v<zpew::Error, zpew::InvalidArgument>);
    STATIC_REQUIRE(std::is_base_of_v<zpew::Error, zpew::ConvergenceError>);
}

TEST_CASE("a thrown error is caught as zpew::Error with its message", "[error]") {
    const std::exception_ptr thrown =
        std::make_exception_ptr(zpew::DataError{"snapshot.csv:3: bad date"});
    std::string caught_message;
    try {
        std::rethrow_exception(thrown);
    } catch (const zpew::Error& error) {
        caught_message = error.what();
    }
    REQUIRE(caught_message == "snapshot.csv:3: bad date");
}
