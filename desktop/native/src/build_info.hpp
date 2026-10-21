#pragma once

#include <string_view>

namespace zpew::native {

struct BuildInfo {
    std::string_view commit;
    bool dirty;
    std::string_view compiler;
};

[[nodiscard]] BuildInfo build_info() noexcept;

}
