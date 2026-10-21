#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace zpew::cli {

[[nodiscard]] std::filesystem::path
resolve_manifest(const std::optional<std::string>& explicit_path);

}
