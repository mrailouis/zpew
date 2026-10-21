#pragma once

#include "analysis/analysis_settings.hpp"
#include "analysis/analysis_worker.hpp"
#include "analysis/asset_analysis.hpp"
#include "chart/chart_series.hpp"
#include "chart/chart_settings.hpp"
#include "ui/symbol_picker.hpp"

#include <zpew/data/snapshot.hpp>
#include <zpew/data/snapshot_data_source.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace zpew::app {

class TerminalApp {
public:
    explicit TerminalApp(std::optional<std::filesystem::path> manifest_path);

    void draw();

private:
    void select(const std::string& symbol);
    void rebuild_chart();
    void request_analysis();
    void poll_analysis();
    void draw_sidebar();
    void draw_chart_settings();
    void draw_analysis_settings();
    void draw_main_area();
    void draw_analysis_status();

    std::unique_ptr<data::SnapshotDataSource> source_;
    std::string load_error_;
    std::string selected_;
    std::optional<data::Snapshot> snapshot_;
    std::optional<ChartSeries> series_;
    std::string series_error_;
    ChartSettings settings_;
    SymbolPicker picker_;
    bool fit_to_data_ = true;

    AnalysisWorker worker_;
    AnalysisSettings analysis_settings_;
    std::optional<AssetAnalysis> analysis_;
    std::string analysis_error_;
    std::uint64_t latest_request_ = 0;
    std::size_t var_model_ = 2;
};

}
