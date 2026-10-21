#pragma once

#include "chart/chart_settings.hpp"

#include <zpew/data/snapshot.hpp>

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace zpew::app {

struct ChartSeries {
    std::string symbol;
    std::string name;
    std::vector<std::chrono::sys_days> dates;
    std::vector<double> times;
    std::vector<double> open;
    std::vector<double> high;
    std::vector<double> low;
    std::vector<double> close;
    std::vector<double> volume;
    std::vector<double> fast_average;
    std::vector<double> slow_average;
    std::vector<double> volatility_percent;
    double seconds_per_bar = 86400.0;

    [[nodiscard]] std::size_t size() const noexcept { return times.size(); }
    [[nodiscard]] std::optional<std::size_t> nearest_index(double time) const;
};

[[nodiscard]] ChartSeries build_chart_series(const data::Snapshot& snapshot,
                                             const ChartSettings& settings);

}
