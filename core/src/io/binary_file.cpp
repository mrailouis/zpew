#include <zpew/error.hpp>
#include <zpew/io/binary_file.hpp>

#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>

namespace zpew::io {

std::string read_binary_file(const std::filesystem::path& path) {
    if (!std::filesystem::is_regular_file(path)) {
        throw DataError{"not a regular file: " + path.string()};
    }
    std::ifstream stream{path, std::ios::binary};
    if (!stream) {
        throw DataError{"cannot open file: " + path.string()};
    }
    std::string bytes{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
    if (stream.bad()) {
        throw DataError{"failed reading file: " + path.string()};
    }
    return bytes;
}

}
