#pragma once

#include "chart/chart_series.hpp"
#include "chart/chart_settings.hpp"

namespace zpew::app {

void draw_chart(const ChartSeries& series, const ChartSettings& settings, bool fit_to_data);

}
