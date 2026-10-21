#include "manifest_path.hpp"

#include <zpew/data/manifest.hpp>
#include <zpew/error.hpp>

#include <filesystem>
#include <optional>
#include <string>

namespace zpew::cli {

std::filesystem::path resolve_manifest(const std::optional<std::string>& explicit_path) {
    if (explicit_path) {
        return std::filesystem::path{*explicit_path};
    }
    if (auto found = data::find_manifest(std::filesystem::current_path())) {
        return *found;
    }
    throw DataError{"no data/manifest.json found in this directory or its parents"};
}

}
