#pragma once

#include <zpew/models/vol_model.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace zpew::evaluation {

enum class WindowMode : std::uint8_t {
    kSliding,
    kExpanding,
};

[[nodiscard]] std::optional<WindowMode> parse_window_mode(std::string_view name) noexcept;
[[nodiscard]] std::string_view to_string(WindowMode mode) noexcept;

inline constexpr std::size_t kDefaultEstimationWindow = 1000;
inline constexpr std::size_t kDefaultRefitInterval = 20;

struct RollingOptions {
    std::size_t window = kDefaultEstimationWindow;
    std::size_t refit_every = kDefaultRefitInterval;
    WindowMode mode = WindowMode::kSliding;
};

struct ForecastSeries {
    std::size_t first_index{};
    std::vector<double> forecasts;
    std::size_t refits{};
    std::size_t boundary_refits{};
};

[[nodiscard]] ForecastSeries rolling_forecasts(models::VolModel& model,
                                               std::span<const double> returns,
                                               const RollingOptions& options);

}
