#include <zpew/version.hpp>

#include <string_view>

namespace zpew {

std::string_view version() noexcept {
    return ZPEW_VERSION;
}

}
