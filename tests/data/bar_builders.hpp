#pragma once

#include <zpew/data/bar.hpp>

#include <chrono>

namespace zpew::test {

inline std::chrono::sys_days day(int year, unsigned month, unsigned day_of_month) {
    return std::chrono::sys_days{std::chrono::year{year} / std::chrono::month{month} /
                                 std::chrono::day{day_of_month}};
}

inline data::Bar bar_on(std::chrono::sys_days date) {
    return data::Bar{
        .date = date, .open = 100.0, .high = 102.0, .low = 99.0, .close = 101.0, .volume = 1000.0};
}

}
