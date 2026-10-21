#pragma once

#include <string_view>

namespace zpew::cli {

struct BuildInfo {
    std::string_view commit;
    bool dirty;
    std::string_view compiler;
};

[[nodiscard]] BuildInfo build_info() noexcept;

}
