#include "ui/terminal_app.hpp"

#include "chart/chart_series.hpp"
#include "chart/chart_settings.hpp"
#include "chart/chart_view.hpp"

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

namespace zpew::app {

namespace {

constexpr float kSidebarWidth = 340.0F;
constexpr int kMinimumWindow = 1;
constexpr int kMaximumWindow = 1000;
constexpr ImVec4 kErrorColour{0.94F, 0.33F, 0.31F, 1.0F};

void error_text(const std::string& message) {
    ImGui::PushStyleColor(ImGuiCol_Text, kErrorColour);
    ImGui::PushTextWrapPos(0.0F);
    ImGui::TextUnformatted(message.c_str());
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
}

bool edit_window(const char* label, int& value) {
    ImGui::SetNextItemWidth(-1.0F);
    const bool changed = ImGui::InputInt(label, &value);
    value = std::clamp(value, kMinimumWindow, kMaximumWindow);
    return changed;
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
    series_.reset();
    series_error_.clear();
    fit_to_data_ = true;
    try {
        series_ = build_chart_series(source_->load_snapshot(symbol), settings_);
    } catch (const Error& error) {
        series_error_ = error.what();
    }
}

void TerminalApp::draw_settings() {
    ImGui::SeparatorText("chart");
    const ChartSettings before = settings_;
    ImGui::TextUnformatted("fast moving average");
    edit_window("##fast", settings_.fast_average);
    ImGui::TextUnformatted("slow moving average");
    edit_window("##slow", settings_.slow_average);
    ImGui::TextUnformatted("volatility window (days)");
    edit_window("##volatility", settings_.volatility_window);
    ImGui::Checkbox("log price scale", &settings_.log_price_scale);
    const bool only_scale_changed =
        settings_.log_price_scale != before.log_price_scale &&
        ChartSettings{.fast_average = settings_.fast_average,
                      .slow_average = settings_.slow_average,
                      .volatility_window = settings_.volatility_window,
                      .log_price_scale = before.log_price_scale} == before;
    if (settings_ != before && !only_scale_changed && !selected_.empty()) {
        const bool keep_view = fit_to_data_;
        select(selected_);
        fit_to_data_ = keep_view;
    }
}

void TerminalApp::draw_sidebar() {
    ImGui::BeginChild("##sidebar", ImVec2{kSidebarWidth, 0.0F}, ImGuiChildFlags_Borders);
    ImGui::TextUnformatted(std::format("zpew {}", version()).c_str());
    if (source_) {
        draw_settings();
        ImGui::SeparatorText("symbols");
        if (const auto chosen = picker_.draw(source_->manifest(), selected_)) {
            select(*chosen);
        }
    }
    ImGui::EndChild();
}

void TerminalApp::draw_main_area() {
    ImGui::BeginChild("##main");
    if (!load_error_.empty()) {
        error_text(load_error_);
    } else if (!series_error_.empty()) {
        error_text(series_error_);
    } else if (series_) {
        ImGui::TextUnformatted(std::format("{}  {}", series_->symbol, series_->name).c_str());
        draw_chart(*series_, settings_, fit_to_data_);
        fit_to_data_ = false;
    }
    ImGui::EndChild();
}

void TerminalApp::draw() {
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
