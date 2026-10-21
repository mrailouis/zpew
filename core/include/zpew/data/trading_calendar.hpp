#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string_view>

namespace zpew::data {

enum class TradingCalendar : std::uint8_t {
    kWeekdays,
    kDaily,
};

[[nodiscard]] std::optional<TradingCalendar> parse_trading_calendar(std::string_view name) noexcept;
[[nodiscard]] std::string_view to_string(TradingCalendar calendar) noexcept;
[[nodiscard]] bool is_session(std::chrono::sys_days date, TradingCalendar calendar) noexcept;
[[nodiscard]] int sessions_between(std::chrono::sys_days earlier, std::chrono::sys_days later,
                                   TradingCalendar calendar) noexcept;
[[nodiscard]] int max_missing_sessions(TradingCalendar calendar) noexcept;

}
