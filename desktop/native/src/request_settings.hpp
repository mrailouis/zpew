#pragma once

#include <zpew/evaluation/rolling_forecast.hpp>

#include <cstddef>
#include <napi.h>
#include <string>

namespace zpew::native {

struct ChartSettings {
    std::size_t fast_average{};
    std::size_t slow_average{};
    std::size_t volatility_window{};
};

struct AnalysisSettings {
    std::size_t window{};
    std::size_t refit_every{};
    evaluation::WindowMode mode{evaluation::WindowMode::kSliding};
    std::size_t baseline_window{};
    double ewma_lambda{};
    bool include_garch{};
    double confidence{};
};

[[nodiscard]] ChartSettings parse_chart_settings(Napi::Env env, const Napi::Value& value);
[[nodiscard]] AnalysisSettings parse_analysis_settings(Napi::Env env, const Napi::Value& value);

}
