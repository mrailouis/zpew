#include "support/temporary_directory.hpp"

#include <zpew/error.hpp>
#include <zpew/io/binary_file.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

using zpew::test::TemporaryDirectory;
using zpew::test::write_bytes;

TEST_CASE("read_binary_file returns the exact bytes, including CR and NUL", "[io]") {
    const TemporaryDirectory directory{"zpew_binary_file_test_exact"};
    const std::string bytes{"a,b\r\nc\0d\n", 9};
    write_bytes(directory.path() / "data.csv", bytes);
    REQUIRE(zpew::io::read_binary_file(directory.path() / "data.csv") == bytes);
}

TEST_CASE("read_binary_file reads an empty file as empty", "[io]") {
    const TemporaryDirectory directory{"zpew_binary_file_test_empty"};
    write_bytes(directory.path() / "empty.csv", "");
    REQUIRE(zpew::io::read_binary_file(directory.path() / "empty.csv").empty());
}

TEST_CASE("read_binary_file fails loudly on missing files and directories", "[io]") {
    const TemporaryDirectory directory{"zpew_binary_file_test_missing"};
    REQUIRE_THROWS_AS(zpew::io::read_binary_file(directory.path() / "missing.csv"),
                      zpew::DataError);
    REQUIRE_THROWS_AS(zpew::io::read_binary_file(directory.path()), zpew::DataError);
}
