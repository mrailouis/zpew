#pragma once

#include <cstddef>

namespace zpew::app {

struct ChartSettings {
    int fast_average = 50;
    int slow_average = 200;
    int volatility_window = 21;
    bool log_price_scale = false;

    friend bool operator==(const ChartSettings&, const ChartSettings&) = default;
};

}
