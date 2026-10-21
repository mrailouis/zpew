#include "chart/chart_view.hpp"

#include "chart/candlestick_plot.hpp"
#include "chart/chart_series.hpp"
#include "chart/chart_settings.hpp"

#include <zpew/data/iso_date.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <format>
#include <imgui.h>
#include <implot.h>
#include <string>

namespace zpew::app {

namespace {

constexpr float kVolumeBarFraction = 0.7F;
constexpr ImPlotAxisFlags kValueAxisFlags = ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_RangeFit;
constexpr ImVec4 kFastAverageColour{0.98F, 0.75F, 0.18F, 1.0F};
constexpr ImVec4 kSlowAverageColour{0.36F, 0.62F, 0.98F, 1.0F};
constexpr ImVec4 kVolumeColour{0.55F, 0.60F, 0.70F, 0.6F};
constexpr ImVec4 kVolatilityColour{0.82F, 0.45F, 0.95F, 1.0F};

ImPlotSpec line_spec(ImVec4 colour) {
    ImPlotSpec spec;
    spec.LineColor = colour;
    spec.LineWeight = 1.5F;
    return spec;
}

void setup_time_axis(const ChartSeries& series, bool fit_to_data) {
    ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
    if (fit_to_data && series.size() > 0) {
        ImPlot::SetupAxisLimits(ImAxis_X1, series.times.front(),
                                series.times.back() + series.seconds_per_bar, ImPlotCond_Always);
    }
}

std::string format_value(double value) {
    return std::isnan(value) ? std::string{"-"} : std::format("{:.2f}", value);
}

void draw_tooltip(const ChartSeries& series) {
    if (!ImPlot::IsPlotHovered()) {
        return;
    }
    const auto index = series.nearest_index(ImPlot::GetPlotMousePos().x);
    if (!index) {
        return;
    }
    const std::size_t i = *index;
    const std::string text = std::format(
        "{}\nopen   {:.4f}\nhigh   {:.4f}\nlow    {:.4f}\nclose  {:.4f}\nvolume {:.0f}\n"
        "vol    {}%",
        data::format_iso_date(series.dates[i]), series.open[i], series.high[i], series.low[i],
        series.close[i], series.volume[i], format_value(series.volatility_percent[i]));
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(text.c_str());
    ImGui::EndTooltip();
}

void draw_price_plot(const ChartSeries& series, const ChartSettings& settings, bool fit_to_data) {
    if (!ImPlot::BeginPlot("##price")) {
        return;
    }
    ImPlot::SetupAxes(nullptr, "price", ImPlotAxisFlags_NoTickLabels, kValueAxisFlags);
    setup_time_axis(series, fit_to_data);
    if (settings.log_price_scale) {
        ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
    }
    ImPlot::SetupLegend(ImPlotLocation_NorthWest);
    const int count = static_cast<int>(series.size());
    plot_candlesticks(series);
    const std::string fast_label = std::format("SMA {}", settings.fast_average);
    const std::string slow_label = std::format("SMA {}", settings.slow_average);
    ImPlot::PlotLine(fast_label.c_str(), series.times.data(), series.fast_average.data(), count,
                     line_spec(kFastAverageColour));
    ImPlot::PlotLine(slow_label.c_str(), series.times.data(), series.slow_average.data(), count,
                     line_spec(kSlowAverageColour));
    draw_tooltip(series);
    ImPlot::EndPlot();
}

void draw_volume_plot(const ChartSeries& series, bool fit_to_data) {
    if (!ImPlot::BeginPlot("##volume")) {
        return;
    }
    ImPlot::SetupAxes(nullptr, "volume", ImPlotAxisFlags_NoTickLabels, kValueAxisFlags);
    setup_time_axis(series, fit_to_data);
    ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, 0.0, HUGE_VAL);
    ImPlotSpec spec;
    spec.FillColor = kVolumeColour;
    spec.LineColor = kVolumeColour;
    spec.Flags = ImPlotItemFlags_NoLegend;
    ImPlot::PlotBars("volume", series.times.data(), series.volume.data(),
                     static_cast<int>(series.size()), series.seconds_per_bar * kVolumeBarFraction,
                     spec);
    draw_tooltip(series);
    ImPlot::EndPlot();
}

void draw_volatility_plot(const ChartSeries& series, const ChartSettings& settings,
                          bool fit_to_data) {
    if (!ImPlot::BeginPlot("##volatility")) {
        return;
    }
    ImPlot::SetupAxes("date", "realised vol %", 0, kValueAxisFlags);
    setup_time_axis(series, fit_to_data);
    ImPlot::SetupLegend(ImPlotLocation_NorthWest);
    const std::string label =
        std::format("{}-day realised volatility (annualised)", settings.volatility_window);
    ImPlot::PlotLine(label.c_str(), series.times.data(), series.volatility_percent.data(),
                     static_cast<int>(series.size()), line_spec(kVolatilityColour));
    draw_tooltip(series);
    ImPlot::EndPlot();
}

}

void draw_chart(const ChartSeries& series, const ChartSettings& settings, bool fit_to_data) {
    std::array<float, 3> row_ratios{0.60F, 0.15F, 0.25F};
    if (!ImPlot::BeginSubplots("##chart", 3, 1, ImVec2{-1.0F, -1.0F},
                               ImPlotSubplotFlags_LinkAllX | ImPlotSubplotFlags_NoTitle,
                               row_ratios.data())) {
        return;
    }
    draw_price_plot(series, settings, fit_to_data);
    draw_volume_plot(series, fit_to_data);
    draw_volatility_plot(series, settings, fit_to_data);
    ImPlot::EndSubplots();
}

}
