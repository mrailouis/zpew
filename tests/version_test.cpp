#include <zpew/version.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string_view>

TEST_CASE("version matches the CMake project version", "[version]") {
    REQUIRE(zpew::version() == std::string_view{ZPEW_EXPECTED_VERSION});
}
