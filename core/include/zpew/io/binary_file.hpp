#pragma once

#include <filesystem>
#include <string>

namespace zpew::io {

[[nodiscard]] std::string read_binary_file(const std::filesystem::path& path);

}
