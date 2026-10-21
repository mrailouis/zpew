#include <zpew/error.hpp>
#include <zpew/io/binary_file.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <system_error>

namespace {

class TemporaryDirectory {
public:
    explicit TemporaryDirectory(const std::string& name)
        : path_{std::filesystem::temp_directory_path() / name} {
        std::filesystem::remove_all(path_);
        std::filesystem::create_directories(path_);
    }
    ~TemporaryDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }
    TemporaryDirectory(const TemporaryDirectory&) = delete;
    TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;
    TemporaryDirectory(TemporaryDirectory&&) = delete;
    TemporaryDirectory& operator=(TemporaryDirectory&&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

private:
    std::filesystem::path path_;
};

void write_bytes(const std::filesystem::path& path, const std::string& bytes) {
    std::ofstream stream{path, std::ios::binary};
    stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

}

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
