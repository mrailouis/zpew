#include <zpew/data/bar.hpp>
#include <zpew/data/iso_date.hpp>
#include <zpew/data/trading_calendar.hpp>
#include <zpew/data/validation.hpp>
#include <zpew/error.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <span>
#include <string_view>

namespace zpew::data {

namespace {

[[noreturn]] void fail(std::size_t index, const Bar& bar, std::string_view message) {
    throw ValidationError{
        std::format("bar {} ({}): {}", index + 1, format_iso_date(bar.date), message)};
}

void require_positive_price(std::size_t index, const Bar& bar, std::string_view name,
                            double price) {
    if (!std::isfinite(price) || price <= 0.0) {
        fail(index, bar, std::format("{} must be finite and positive, got {}", name, price));
    }
}

void validate_bar(std::size_t index, const Bar& bar, TradingCalendar calendar) {
    require_positive_price(index, bar, "open", bar.open);
    require_positive_price(index, bar, "high", bar.high);
    require_positive_price(index, bar, "low", bar.low);
    require_positive_price(index, bar, "close", bar.close);
    if (!std::isfinite(bar.volume) || bar.volume < 0.0) {
        fail(index, bar, std::format("volume must be finite and non-negative, got {}", bar.volume));
    }
    if (bar.high < std::max(bar.open, bar.close)) {
        fail(index, bar,
             std::format("high {} is below max(open, close) {}", bar.high,
                         std::max(bar.open, bar.close)));
    }
    if (bar.low > std::min(bar.open, bar.close)) {
        fail(index, bar,
             std::format("low {} is above min(open, close) {}", bar.low,
                         std::min(bar.open, bar.close)));
    }
    if (!is_session(bar.date, calendar)) {
        fail(index, bar,
             std::format("date is not a session of the '{}' calendar", to_string(calendar)));
    }
}

}

void validate_bars(std::span<const Bar> bars, TradingCalendar calendar) {
    if (bars.size() < 2) {
        throw ValidationError{std::format("at least 2 bars are required, got {}", bars.size())};
    }
    for (std::size_t i = 0; i < bars.size(); ++i) {
        validate_bar(i, bars[i], calendar);
        if (i > 0 && bars[i].date <= bars[i - 1].date) {
            fail(i, bars[i],
                 std::format("date is not after the previous bar's {}",
                             format_iso_date(bars[i - 1].date)));
        }
    }
}

}
