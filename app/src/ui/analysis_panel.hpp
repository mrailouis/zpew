#pragma once

#include "analysis/analysis_settings.hpp"
#include "analysis/asset_analysis.hpp"

#include <cstddef>

namespace zpew::app {

[[nodiscard]] bool edit_analysis_settings(AnalysisSettings& settings);
void edit_var_model(std::size_t& var_model, const AssetAnalysis* analysis);
void draw_model_comparison(const AssetAnalysis& analysis);

}
