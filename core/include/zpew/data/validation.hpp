#pragma once

#include <zpew/data/bar.hpp>
#include <zpew/data/trading_calendar.hpp>

#include <span>

namespace zpew::data {

void validate_bars(std::span<const Bar> bars, TradingCalendar calendar);

}
