#pragma once

#include <filesystem>
#include <ostream>

namespace zpew::cli {

[[nodiscard]] int run_check_data(const std::filesystem::path& manifest_path, std::ostream& out,
                                 std::ostream& err);

}
