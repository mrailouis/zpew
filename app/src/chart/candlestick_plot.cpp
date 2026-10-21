#include "chart/candlestick_plot.hpp"

#include "chart/chart_series.hpp"

#include <algorithm>
#include <cstddef>
#include <imgui.h>
#include <implot.h>
#include <iterator>

namespace zpew::app {

namespace {

constexpr float kBodyWidthFraction = 0.35F;
constexpr ImU32 kRisingColour = IM_COL32(38, 166, 154, 255);
constexpr ImU32 kFallingColour = IM_COL32(239, 83, 80, 255);

struct VisibleRange {
    std::size_t first;
    std::size_t last;
};

VisibleRange visible_range(const ChartSeries& series) {
    const ImPlotRect limits = ImPlot::GetPlotLimits();
    const auto begin =
        std::ranges::lower_bound(series.times, limits.X.Min - series.seconds_per_bar);
    const auto end = std::ranges::upper_bound(series.times, limits.X.Max + series.seconds_per_bar);
    return {.first = static_cast<std::size_t>(std::distance(series.times.begin(), begin)),
            .last = static_cast<std::size_t>(std::distance(series.times.begin(), end))};
}

}

void plot_candlesticks(const ChartSeries& series) {
    ImPlotSpec invisible;
    invisible.LineColor = ImVec4{0.0F, 0.0F, 0.0F, 0.0F};
    invisible.Flags = ImPlotItemFlags_NoLegend;
    const int count = static_cast<int>(series.size());
    ImPlot::PlotLine("##high", series.times.data(), series.high.data(), count, invisible);
    ImPlot::PlotLine("##low", series.times.data(), series.low.data(), count, invisible);

    ImDrawList* draw_list = ImPlot::GetPlotDrawList();
    const auto [first, last] = visible_range(series);
    const float half_body = std::max(1.0F, (ImPlot::PlotToPixels(series.seconds_per_bar, 0.0).x -
                                            ImPlot::PlotToPixels(0.0, 0.0).x) *
                                               kBodyWidthFraction);

    ImPlot::PushPlotClipRect();
    for (std::size_t i = first; i < last; ++i) {
        const ImU32 colour = series.close[i] >= series.open[i] ? kRisingColour : kFallingColour;
        const ImVec2 high = ImPlot::PlotToPixels(series.times[i], series.high[i]);
        const ImVec2 low = ImPlot::PlotToPixels(series.times[i], series.low[i]);
        const ImVec2 open = ImPlot::PlotToPixels(series.times[i], series.open[i]);
        const ImVec2 close = ImPlot::PlotToPixels(series.times[i], series.close[i]);
        draw_list->AddLine(high, low, colour);
        const float top = std::min(open.y, close.y);
        const float bottom = std::max({open.y, close.y, top + 1.0F});
        draw_list->AddRectFilled(ImVec2{open.x - half_body, top},
                                 ImVec2{open.x + half_body, bottom}, colour);
    }
    ImPlot::PopPlotClipRect();
}

}
