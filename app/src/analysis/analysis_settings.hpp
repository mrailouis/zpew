#pragma once

#include <zpew/evaluation/rolling_forecast.hpp>

namespace zpew::app {

struct AnalysisSettings {
    int window = 1000;
    int refit_every = 20;
    evaluation::WindowMode mode = evaluation::WindowMode::kSliding;
    int baseline_window = 21;
    double ewma_lambda = 0.94;
    bool include_garch = true;
    double confidence = 0.95;

    friend bool operator==(const AnalysisSettings&, const AnalysisSettings&) = default;
};

}
