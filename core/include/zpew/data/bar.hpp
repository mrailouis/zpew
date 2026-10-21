#pragma once

#include <chrono>

namespace zpew::data {

struct Bar {
    std::chrono::sys_days date;
    double open{};
    double high{};
    double low{};
    double close{};
    double volume{};

    friend bool operator==(const Bar&, const Bar&) = default;
};

}
