#pragma once

#include <string>
#include <string_view>

namespace zpew::cli {

[[nodiscard]] std::string json_string(std::string_view text);

}
