#pragma once

#include "request_settings.hpp"

#include <zpew/data/snapshot.hpp>

#include <napi.h>
#include <string>
#include <vector>

namespace zpew::native {

struct ChartGap {
    double last_before{};
    double first_after{};
    int missing_sessions{};
};

struct ChartData {
    std::string symbol;
    std::string name;
    std::string calendar;
    std::vector<double> times;
    std::vector<double> open;
    std::vector<double> high;
    std::vector<double> low;
    std::vector<double> close;
    std::vector<double> volume;
    std::vector<double> fast_average;
    std::vector<double> slow_average;
    std::vector<double> volatility_percent;
    std::vector<ChartGap> gaps;
};

[[nodiscard]] ChartData build_chart_data(const data::Snapshot& snapshot,
                                         const ChartSettings& settings);

[[nodiscard]] Napi::Value load_chart(const Napi::CallbackInfo& info);

}
