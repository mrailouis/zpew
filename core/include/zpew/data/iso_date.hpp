#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace zpew::data {

[[nodiscard]] std::optional<std::chrono::sys_days> parse_iso_date(std::string_view text);
[[nodiscard]] std::string format_iso_date(std::chrono::sys_days date);

}
