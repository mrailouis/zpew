#pragma once

#include "analysis/asset_analysis.hpp"
#include "chart/chart_series.hpp"
#include "chart/chart_settings.hpp"

#include <cstddef>

namespace zpew::app {

struct ChartOverlay {
    const AssetAnalysis* analysis = nullptr;
    std::size_t var_model = 0;
};

void draw_chart(const ChartSeries& series, const ChartSettings& settings, bool fit_to_data,
                const ChartOverlay& overlay);

}
