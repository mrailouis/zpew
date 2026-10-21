#pragma once

#include <zpew/evaluation/rolling_forecast.hpp>
#include <zpew/models/model_spec.hpp>

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace zpew::cli {

struct EvalConfig {
    std::filesystem::path path;
    std::string sha256;
    std::string name;
    std::filesystem::path manifest;
    std::filesystem::path output;
    std::vector<std::string> assets;
    std::size_t threads{};
    std::string baseline;
    std::size_t window{};
    std::size_t refit_every{};
    std::vector<evaluation::WindowMode> window_modes;
    std::vector<models::ModelSpec> models;
    std::vector<double> confidence_levels;

    [[nodiscard]] std::filesystem::path run_directory() const { return output / name; }
};

[[nodiscard]] EvalConfig load_eval_config(const std::filesystem::path& path);

}
