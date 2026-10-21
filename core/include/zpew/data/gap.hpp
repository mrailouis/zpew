#pragma once

#include <zpew/data/bar.hpp>
#include <zpew/data/trading_calendar.hpp>

#include <chrono>
#include <span>
#include <vector>

namespace zpew::data {

struct Gap {
    std::chrono::sys_days last_before;
    std::chrono::sys_days first_after;
    int missing_sessions{};

    friend bool operator==(const Gap&, const Gap&) = default;
};

[[nodiscard]] std::vector<Gap> find_gaps(std::span<const Bar> bars, TradingCalendar calendar);

}
