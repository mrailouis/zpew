#include <zpew/data/iso_date.hpp>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace zpew::data {

namespace {

constexpr std::size_t kIsoDateLength = 10;
constexpr std::size_t kFirstSeparator = 4;
constexpr std::size_t kSecondSeparator = 7;

bool is_ascii_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}

std::optional<int> parse_digits(std::string_view digits) {
    if (digits.empty() || !std::ranges::all_of(digits, is_ascii_digit)) {
        return std::nullopt;
    }
    int value = 0;
    const char* const last = std::to_address(digits.end());
    const auto [end, error] = std::from_chars(std::to_address(digits.begin()), last, value);
    if (error != std::errc{} || end != last) {
        return std::nullopt;
    }
    return value;
}

}

std::optional<std::chrono::sys_days> parse_iso_date(std::string_view text) {
    if (text.size() != kIsoDateLength || text[kFirstSeparator] != '-' ||
        text[kSecondSeparator] != '-') {
        return std::nullopt;
    }
    const auto year = parse_digits(text.substr(0, kFirstSeparator));
    const auto month = parse_digits(text.substr(kFirstSeparator + 1, 2));
    const auto day = parse_digits(text.substr(kSecondSeparator + 1, 2));
    if (!year || !month || !day) {
        return std::nullopt;
    }
    const std::chrono::year_month_day date{std::chrono::year{*year},
                                           std::chrono::month{static_cast<unsigned>(*month)},
                                           std::chrono::day{static_cast<unsigned>(*day)}};
    if (!date.ok()) {
        return std::nullopt;
    }
    return std::chrono::sys_days{date};
}

std::string format_iso_date(std::chrono::sys_days date) {
    const std::chrono::year_month_day ymd{date};
    return std::format("{:04}-{:02}-{:02}", static_cast<int>(ymd.year()),
                       static_cast<unsigned>(ymd.month()), static_cast<unsigned>(ymd.day()));
}

}
