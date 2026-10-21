#pragma once

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <map>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace zpew::cli {

class UsageError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Options {
public:
    Options(std::span<const std::string_view> args,
            std::initializer_list<std::string_view> allowed);

    [[nodiscard]] std::optional<std::string> get(std::string_view name) const;
    [[nodiscard]] std::string require(std::string_view name) const;
    [[nodiscard]] double number(std::string_view name, double fallback) const;
    [[nodiscard]] std::size_t count(std::string_view name, std::size_t fallback) const;

private:
    std::map<std::string, std::string, std::less<>> values_;
};

}
