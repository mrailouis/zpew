#include "ui/terminal_app.hpp"

#include "analysis/asset_analysis.hpp"
#include "chart/chart_series.hpp"
#include "chart/chart_settings.hpp"
#include "chart/chart_view.hpp"
#include "ui/analysis_panel.hpp"

#include <zpew/data/snapshot_data_source.hpp>
#include <zpew/error.hpp>
#include <zpew/version.hpp>

#include <algorithm>
#include <filesystem>
#include <format>
#include <imgui.h>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace zpew::app {

namespace {

constexpr float kSidebarWidth = 340.0F;
constexpr int kMinimumWindow = 1;
constexpr int kMaximumWindow = 1000;
constexpr ImVec4 kErrorColour{0.94F, 0.33F, 0.31F, 1.0F};
constexpr ImVec4 kMutedColour{0.60F, 0.62F, 0.68F, 1.0F};

void coloured_text(const std::string& message, ImVec4 colour) {
    ImGui::PushStyleColor(ImGuiCol_Text, colour);
    ImGui::PushTextWrapPos(0.0F);
    ImGui::TextUnformatted(message.c_str());
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
}

bool edit_window(const char* label, int& value) {
    ImGui::SetNextItemWidth(-1.0F);
    const int before = value;
    ImGui::InputInt(label, &value);
    value = std::clamp(value, kMinimumWindow, kMaximumWindow);
    return value != before;
}

}

TerminalApp::TerminalApp(std::optional<std::filesystem::path> manifest_path) {
    if (!manifest_path) {
        load_error_ = "No data/manifest.json found. Fetch data as described in README.md, "
                      "or start zpew-app with --manifest <path>.";
        return;
    }
    try {
        source_ = std::make_unique<data::SnapshotDataSource>(*manifest_path);
        if (!source_->manifest().assets().empty()) {
            select(source_->manifest().assets().front().symbol);
        }
    } catch (const Error& error) {
        load_error_ = error.what();
    }
}

void TerminalApp::select(const std::string& symbol) {
    selected_ = symbol;
    snapshot_.reset();
    series_.reset();
    series_error_.clear();
    analysis_.reset();
    analysis_error_.clear();
    fit_to_data_ = true;
    try {
        snapshot_ = source_->load_snapshot(symbol);
    } catch (const Error& error) {
        series_error_ = error.what();
        return;
    }
    rebuild_chart();
    request_analysis();
}

void TerminalApp::rebuild_chart() {
    if (!snapshot_) {
        return;
    }
    try {
        series_ = build_chart_series(*snapshot_, settings_);
        series_error_.clear();
    } catch (const Error& error) {
        series_.reset();
        series_error_ = error.what();
    }
}

void TerminalApp::request_analysis() {
    if (snapshot_) {
        latest_request_ = worker_.request(*snapshot_, analysis_settings_);
    }
}

void TerminalApp::poll_analysis() {
    auto outcome = worker_.take_outcome();
    if (!outcome || outcome->request_id != latest_request_) {
        return;
    }
    if (auto* analysis = std::get_if<AssetAnalysis>(&outcome->value)) {
        analysis_ = std::move(*analysis);
        analysis_error_.clear();
    } else {
        analysis_.reset();
        analysis_error_ = std::get<std::string>(outcome->value);
    }
}

void TerminalApp::draw_chart_settings() {
    ImGui::SeparatorText("chart");
    const ChartSettings before = settings_;
    ImGui::TextUnformatted("fast moving average");
    edit_window("##fast", settings_.fast_average);
    ImGui::TextUnformatted("slow moving average");
    edit_window("##slow", settings_.slow_average);
    ImGui::TextUnformatted("realised volatility window (days)");
    edit_window("##volatility", settings_.volatility_window);
    ImGui::Checkbox("log price scale", &settings_.log_price_scale);
    ChartSettings without_scale = settings_;
    without_scale.log_price_scale = before.log_price_scale;
    if (without_scale != before) {
        rebuild_chart();
    }
}

void TerminalApp::draw_analysis_settings() {
    ImGui::SeparatorText("forecast evaluation");
    if (edit_analysis_settings(analysis_settings_)) {
        request_analysis();
    }
    edit_var_model(var_model_, analysis_ ? &*analysis_ : nullptr);
}

void TerminalApp::draw_sidebar() {
    ImGui::BeginChild("##sidebar", ImVec2{kSidebarWidth, 0.0F}, ImGuiChildFlags_Borders);
    ImGui::TextUnformatted(std::format("zpew {}", version()).c_str());
    if (source_) {
        draw_chart_settings();
        draw_analysis_settings();
        ImGui::SeparatorText("symbols");
        if (const auto chosen = picker_.draw(source_->manifest(), selected_)) {
            select(*chosen);
        }
    }
    ImGui::EndChild();
}

void TerminalApp::draw_analysis_status() {
    if (worker_.busy()) {
        coloured_text(analysis_ ? "updating forecasts..." : "evaluating forecasts...",
                      kMutedColour);
    } else if (!analysis_error_.empty()) {
        coloured_text("forecast evaluation unavailable: " + analysis_error_, kErrorColour);
    }
}

void TerminalApp::draw_main_area() {
    ImGui::BeginChild("##main");
    if (!load_error_.empty()) {
        coloured_text(load_error_, kErrorColour);
    } else if (!series_error_.empty()) {
        coloured_text(series_error_, kErrorColour);
    } else if (series_) {
        const ChartSeries& series = *series_;
        ImGui::TextUnformatted(std::format("{}  {}", series.symbol, series.name).c_str());
        draw_analysis_status();
        if (ImGui::BeginTabBar("##views")) {
            if (ImGui::BeginTabItem("chart")) {
                const ChartOverlay overlay{.analysis = analysis_ ? &*analysis_ : nullptr,
                                           .var_model = var_model_};
                draw_chart(series, settings_, fit_to_data_, overlay);
                fit_to_data_ = false;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("model comparison")) {
                if (analysis_) {
                    draw_model_comparison(*analysis_);
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::EndChild();
}

void TerminalApp::draw() {
    poll_analysis();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoSavedSettings |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (ImGui::Begin("zpew", nullptr, kFlags)) {
        draw_sidebar();
        ImGui::SameLine();
        draw_main_area();
    }
    ImGui::End();
}

}
