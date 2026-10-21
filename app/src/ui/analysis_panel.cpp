#include "ui/analysis_panel.hpp"

#include "analysis/analysis_settings.hpp"
#include "analysis/asset_analysis.hpp"

#include <zpew/evaluation/rolling_forecast.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <imgui.h>
#include <optional>
#include <string>

namespace zpew::app {

namespace {

constexpr int kMinimumWindow = 100;
constexpr int kMaximumWindow = 5000;
constexpr int kMaximumRefit = 250;
constexpr int kMinimumBaseline = 2;
constexpr int kMaximumBaseline = 252;
constexpr double kMinimumLambda = 0.5;
constexpr double kMaximumLambda = 0.999;
constexpr double kLambdaStep = 0.01;
constexpr std::array<double, 3> kConfidenceLevels{0.95, 0.975, 0.99};
constexpr int kComparisonColumns = 12;

bool edit_int(const char* label, const char* id, int& value, int minimum, int maximum) {
    ImGui::TextUnformatted(label);
    ImGui::SetNextItemWidth(-1.0F);
    const int before = value;
    ImGui::InputInt(id, &value);
    value = std::clamp(value, minimum, maximum);
    return value != before;
}

bool edit_mode(evaluation::WindowMode& mode) {
    bool changed = false;
    ImGui::TextUnformatted("estimation window mode");
    ImGui::SetNextItemWidth(-1.0F);
    if (ImGui::BeginCombo("##mode", std::string{evaluation::to_string(mode)}.c_str())) {
        for (const auto option :
             {evaluation::WindowMode::kSliding, evaluation::WindowMode::kExpanding}) {
            if (ImGui::Selectable(std::string{evaluation::to_string(option)}.c_str(),
                                  option == mode)) {
                changed = option != mode;
                mode = option;
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

bool edit_confidence(double& confidence) {
    bool changed = false;
    ImGui::TextUnformatted("VaR confidence");
    ImGui::SetNextItemWidth(-1.0F);
    if (ImGui::BeginCombo("##confidence", std::format("{:g}%", confidence * 100.0).c_str())) {
        for (const double level : kConfidenceLevels) {
            if (ImGui::Selectable(std::format("{:g}%", level * 100.0).c_str(),
                                  level == confidence)) {
                changed = level != confidence;
                confidence = level;
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

std::string percent(const std::optional<double>& value) {
    return value ? std::format("{:+.2f}%", *value) : std::string{"baseline"};
}

void cell(const std::string& text) {
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(text.c_str());
}

void draw_model_row(const ModelAnalysis& model) {
    ImGui::TableNextRow();
    cell(model.name);
    if (model.failure) {
        cell(*model.failure);
        for (int column = 2; column < kComparisonColumns; ++column) {
            cell("");
        }
        return;
    }
    cell(std::format("{} ({} at boundary)", model.refits, model.boundary_refits));
    const auto& losses = model.losses;
    cell(losses ? std::format("{:.4g}", losses->squared_return.mse) : "");
    cell(losses ? std::format("{:.4f}", losses->squared_return.qlike) : "");
    const auto& squared = model.squared_return_improvement;
    cell(percent(squared ? std::optional<double>{squared->mse_percent} : std::nullopt));
    cell(percent(squared ? std::optional<double>{squared->qlike_percent} : std::nullopt));
    const auto& parkinson = model.parkinson_improvement;
    cell(percent(parkinson ? std::optional<double>{parkinson->mse_percent} : std::nullopt));
    cell(percent(parkinson ? std::optional<double>{parkinson->qlike_percent} : std::nullopt));
    const auto& backtest = model.backtest;
    cell(backtest ? std::format("{} / {}", backtest->breaches, backtest->observations) : "");
    cell(backtest ? std::format("{:.2f}%", backtest->breach_rate * 100.0) : "");
    cell(backtest ? std::format("{:.2f}%", backtest->expected_rate * 100.0) : "");
    cell(backtest ? std::format("{:.3g}", backtest->kupiec.p_value) : "");
}

}

bool edit_analysis_settings(AnalysisSettings& settings) {
    bool changed = false;
    changed |= edit_int("estimation window (returns)", "##window", settings.window, kMinimumWindow,
                        kMaximumWindow);
    changed |= edit_int("refit every (days)", "##refit", settings.refit_every, 1, kMaximumRefit);
    changed |= edit_mode(settings.mode);
    changed |= edit_int("baseline realised window", "##baseline", settings.baseline_window,
                        kMinimumBaseline, kMaximumBaseline);
    ImGui::TextUnformatted("EWMA lambda");
    ImGui::SetNextItemWidth(-1.0F);
    const double lambda_before = settings.ewma_lambda;
    ImGui::InputDouble("##lambda", &settings.ewma_lambda, kLambdaStep, kLambdaStep, "%.3f");
    settings.ewma_lambda = std::clamp(settings.ewma_lambda, kMinimumLambda, kMaximumLambda);
    changed |= settings.ewma_lambda != lambda_before;
    changed |= ImGui::Checkbox("include GARCH(1,1)", &settings.include_garch);
    changed |= edit_confidence(settings.confidence);
    return changed;
}

void edit_var_model(std::size_t& var_model, const AssetAnalysis* analysis) {
    if (analysis == nullptr || analysis->models.empty()) {
        return;
    }
    var_model = std::min(var_model, analysis->models.size() - 1);
    ImGui::TextUnformatted("VaR band model");
    ImGui::SetNextItemWidth(-1.0F);
    if (ImGui::BeginCombo("##var_model", analysis->models[var_model].name.c_str())) {
        for (std::size_t i = 0; i < analysis->models.size(); ++i) {
            if (ImGui::Selectable(analysis->models[i].name.c_str(), i == var_model)) {
                var_model = i;
            }
        }
        ImGui::EndCombo();
    }
}

void draw_model_comparison(const AssetAnalysis& analysis) {
    ImGui::TextUnformatted(
        std::format("{}: window {} returns, refit every {} days, {} window, baseline {}, computed "
                    "in {:.1f} s",
                    analysis.symbol, analysis.settings.window, analysis.settings.refit_every,
                    evaluation::to_string(analysis.settings.mode), analysis.baseline,
                    analysis.seconds)
            .c_str());
    constexpr ImGuiTableFlags kFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                       ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX;
    if (!ImGui::BeginTable("##comparison", kComparisonColumns, kFlags)) {
        return;
    }
    for (const char* header :
         {"model", "refits", "MSE (r²)", "QLIKE (r²)", "MSE vs base (r²)", "QLIKE vs base (r²)",
          "MSE vs base (Parkinson)", "QLIKE vs base (Parkinson)", "VaR breaches", "breach rate",
          "expected", "Kupiec p"}) {
        ImGui::TableSetupColumn(header);
    }
    ImGui::TableHeadersRow();
    for (const ModelAnalysis& model : analysis.models) {
        draw_model_row(model);
    }
    ImGui::EndTable();
    ImGui::TextUnformatted(
        "Improvements are 100 x (1 - model loss / baseline loss); positive means better than the "
        "baseline. QLIKE excludes days with a zero proxy. Kupiec p below 0.05 rejects the "
        "expected breach rate.");
}

}
