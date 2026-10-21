#include "results_writer.hpp"

#include <zpew/data/iso_date.hpp>
#include <zpew/error.hpp>
#include <zpew/evaluation/asset_evaluation.hpp>
#include <zpew/evaluation/improvement.hpp>
#include <zpew/evaluation/losses.hpp>
#include <zpew/evaluation/rolling_forecast.hpp>

#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <span>
#include <string>
#include <string_view>

namespace zpew::cli {

namespace {

std::ofstream open_output(const std::filesystem::path& file) {
    std::filesystem::create_directories(file.parent_path());
    std::ofstream stream{file, std::ios::binary | std::ios::trunc};
    if (!stream) {
        throw DataError{std::format("cannot write {}", file.generic_string())};
    }
    return stream;
}

std::string csv_field(std::string_view text) {
    if (text.find_first_of(",\"\n") == std::string_view::npos) {
        return std::string{text};
    }
    std::string quoted = "\"";
    for (const char c : text) {
        quoted += c == '"' ? std::string{"\"\""} : std::string{c};
    }
    return quoted + "\"";
}

void write_loss_row(std::ofstream& out, const evaluation::AssetEvaluation& asset,
                    const evaluation::ModelEvaluation& model, std::string_view proxy,
                    const evaluation::LossSummary& losses) {
    out << std::format("{},{},{},{},{},{},{},{},{},{},{}\n", asset.symbol,
                       evaluation::to_string(asset.mode), csv_field(model.model), proxy,
                       losses.observations, losses.mse, losses.qlike, losses.qlike_excluded,
                       losses.qlike_all_days, model.refits, model.boundary_refits);
}

}

void write_forecasts(const std::filesystem::path& directory,
                     const evaluation::AssetEvaluation& asset) {
    std::ofstream out = open_output(directory / std::string{evaluation::to_string(asset.mode)} /
                                    (asset.symbol + ".csv"));
    out << "date,return,squared_return,parkinson";
    for (const auto& model : asset.models) {
        out << ',' << csv_field(model.model);
    }
    out << '\n';
    for (std::size_t i = 0; i < asset.dates.size(); ++i) {
        out << std::format("{},{},{},{}", data::format_iso_date(asset.dates[i]), asset.returns[i],
                           asset.squared_return_proxy[i], asset.parkinson_proxy[i]);
        for (const auto& model : asset.models) {
            out << ',';
            if (!model.failure) {
                out << std::format("{}", model.forecasts[i]);
            }
        }
        out << '\n';
    }
}

void write_losses(const std::filesystem::path& file,
                  std::span<const evaluation::AssetEvaluation> assets) {
    std::ofstream out = open_output(file);
    out << "symbol,window_mode,model,proxy,observations,mse,qlike,qlike_excluded,"
           "qlike_all_days,refits,boundary_refits\n";
    for (const auto& asset : assets) {
        for (const auto& model : asset.models) {
            if (!model.losses) {
                continue;
            }
            write_loss_row(out, asset, model, "squared_return", model.losses->squared_return);
            write_loss_row(out, asset, model, "parkinson", model.losses->parkinson);
        }
    }
}

void write_improvements(const std::filesystem::path& file,
                        std::span<const evaluation::Improvement> improvements) {
    std::ofstream out = open_output(file);
    out << "symbol,window_mode,model,proxy,mse_vs_baseline_pct,qlike_vs_baseline_pct\n";
    for (const auto& item : improvements) {
        out << std::format("{},{},{},{},{},{}\n", item.symbol, evaluation::to_string(item.mode),
                           csv_field(item.model), item.proxy, item.mse_percent, item.qlike_percent);
    }
}

void write_pooled(const std::filesystem::path& file,
                  std::span<const evaluation::PooledImprovement> pooled) {
    std::ofstream out = open_output(file);
    out << "window_mode,model,proxy,metric,assets,mean_improvement_pct,"
           "median_improvement_pct,assets_beating_baseline\n";
    for (const auto& row : pooled) {
        out << std::format("{},{},{},{},{},{},{},{}\n", evaluation::to_string(row.mode),
                           csv_field(row.model), row.proxy, row.metric, row.assets,
                           row.mean_percent, row.median_percent, row.wins);
    }
}

void write_failures(const std::filesystem::path& file, std::span<const AssetFailure> failures) {
    std::ofstream out = open_output(file);
    out << "symbol,window_mode,model,reason\n";
    for (const auto& failure : failures) {
        out << std::format("{},{},{},{}\n", failure.symbol, failure.mode, csv_field(failure.model),
                           csv_field(failure.reason));
    }
}

}
