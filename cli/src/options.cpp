#include "options.hpp"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <format>
#include <initializer_list>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>

namespace zpew::cli {

namespace {

template <typename T>
T parse(std::string_view name, const std::string& text) {
    T value{};
    const char* const last = std::to_address(text.end());
    const auto [end, error] = std::from_chars(std::to_address(text.begin()), last, value);
    if (text.empty() || error != std::errc{} || end != last) {
        throw UsageError{std::format("--{} expects a number, got '{}'", name, text)};
    }
    return value;
}

}

Options::Options(std::span<const std::string_view> args,
                 std::initializer_list<std::string_view> allowed) {
    for (std::size_t i = 0; i < args.size(); i += 2) {
        const std::string_view flag = args[i];
        if (!flag.starts_with("--") || i + 1 >= args.size()) {
            throw UsageError{std::format("expected --option value, got '{}'", flag)};
        }
        const std::string_view name = flag.substr(2);
        if (std::ranges::find(allowed, name) == allowed.end()) {
            throw UsageError{std::format("unknown option '{}'", flag)};
        }
        if (!values_.emplace(std::string{name}, std::string{args[i + 1]}).second) {
            throw UsageError{std::format("option '{}' given twice", flag)};
        }
    }
}

std::optional<std::string> Options::get(std::string_view name) const {
    const auto it = values_.find(name);
    return it == values_.end() ? std::nullopt : std::optional<std::string>{it->second};
}

std::string Options::require(std::string_view name) const {
    auto value = get(name);
    if (!value) {
        throw UsageError{std::format("missing required option --{}", name)};
    }
    return *value;
}

double Options::number(std::string_view name, double fallback) const {
    const auto value = get(name);
    return value ? parse<double>(name, *value) : fallback;
}

std::size_t Options::count(std::string_view name, std::size_t fallback) const {
    const auto value = get(name);
    return value ? parse<std::size_t>(name, *value) : fallback;
}

}
