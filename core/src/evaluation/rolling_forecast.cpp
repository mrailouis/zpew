#include <zpew/error.hpp>
#include <zpew/evaluation/rolling_forecast.hpp>
#include <zpew/models/vol_model.hpp>

#include <cstddef>
#include <format>
#include <optional>
#include <span>
#include <string_view>

namespace zpew::evaluation {

namespace {

constexpr std::string_view kSlidingName = "sliding";
constexpr std::string_view kExpandingName = "expanding";

void validate(std::span<const double> returns, const RollingOptions& options) {
    if (options.window == 0 || options.refit_every == 0) {
        throw InvalidArgument{"estimation window and refit interval must be at least 1"};
    }
    if (returns.size() <= options.window) {
        throw InvalidArgument{std::format("rolling evaluation needs more than {} returns, got {}",
                                          options.window, returns.size())};
    }
}

template <typename Exception>
[[noreturn]] void rethrow_with_index(const Exception& error, std::size_t t) {
    throw Exception{std::format("at return {}: {}", t, error.what())};
}

}

std::optional<WindowMode> parse_window_mode(std::string_view name) noexcept {
    if (name == kSlidingName) {
        return WindowMode::kSliding;
    }
    if (name == kExpandingName) {
        return WindowMode::kExpanding;
    }
    return std::nullopt;
}

std::string_view to_string(WindowMode mode) noexcept {
    return mode == WindowMode::kSliding ? kSlidingName : kExpandingName;
}

ForecastSeries rolling_forecasts(models::VolModel& model, std::span<const double> returns,
                                 const RollingOptions& options) {
    validate(returns, options);
    ForecastSeries series{.first_index = options.window, .forecasts = {}, .refits = 0};
    series.forecasts.reserve(returns.size() - options.window);
    for (std::size_t t = options.window; t < returns.size(); ++t) {
        try {
            if ((t - options.window) % options.refit_every == 0) {
                const std::size_t start =
                    options.mode == WindowMode::kSliding ? t - options.window : 0;
                model.fit(returns.subspan(start, t - start));
                ++series.refits;
                if (model.fit_at_boundary()) {
                    ++series.boundary_refits;
                }
            } else {
                model.update(returns[t - 1]);
            }
            series.forecasts.push_back(model.forecast_variance());
        } catch (const ConvergenceError& error) {
            rethrow_with_index(error, t);
        } catch (const InvalidArgument& error) {
            rethrow_with_index(error, t);
        } catch (const Error& error) {
            rethrow_with_index(error, t);
        }
    }
    return series;
}

}
