#include <zpew/data/trading_calendar.hpp>

#include <chrono>
#include <optional>
#include <string_view>

namespace zpew::data {

namespace {

constexpr std::string_view kWeekdaysName = "weekdays";
constexpr std::string_view kDailyName = "daily";
constexpr int kMaxMissingWeekdays = 2;
constexpr int kMaxMissingDays = 0;

}

std::optional<TradingCalendar> parse_trading_calendar(std::string_view name) noexcept {
    if (name == kWeekdaysName) {
        return TradingCalendar::kWeekdays;
    }
    if (name == kDailyName) {
        return TradingCalendar::kDaily;
    }
    return std::nullopt;
}

std::string_view to_string(TradingCalendar calendar) noexcept {
    switch (calendar) {
    case TradingCalendar::kWeekdays:
        return kWeekdaysName;
    case TradingCalendar::kDaily:
        return kDailyName;
    }
    return {};
}

bool is_session(std::chrono::sys_days date, TradingCalendar calendar) noexcept {
    if (calendar == TradingCalendar::kDaily) {
        return true;
    }
    const std::chrono::weekday weekday{date};
    return weekday != std::chrono::Saturday && weekday != std::chrono::Sunday;
}

int sessions_between(std::chrono::sys_days earlier, std::chrono::sys_days later,
                     TradingCalendar calendar) noexcept {
    int sessions = 0;
    for (auto date = earlier + std::chrono::days{1}; date < later; date += std::chrono::days{1}) {
        if (is_session(date, calendar)) {
            ++sessions;
        }
    }
    return sessions;
}

int max_missing_sessions(TradingCalendar calendar) noexcept {
    return calendar == TradingCalendar::kDaily ? kMaxMissingDays : kMaxMissingWeekdays;
}

}
