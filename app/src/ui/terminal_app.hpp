#pragma once

#include "chart/chart_series.hpp"
#include "chart/chart_settings.hpp"
#include "ui/symbol_picker.hpp"

#include <zpew/data/snapshot_data_source.hpp>

#include <filesystem>
#include <map>
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
    void draw_sidebar();
    void draw_settings();
    void draw_main_area();

    std::unique_ptr<data::SnapshotDataSource> source_;
    std::string load_error_;
    std::string selected_;
    std::optional<ChartSeries> series_;
    std::string series_error_;
    ChartSettings settings_;
    SymbolPicker picker_;
    bool fit_to_data_ = true;
};

}
