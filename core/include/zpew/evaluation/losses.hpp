#pragma once

#include <cstddef>
#include <span>

namespace zpew::evaluation {

struct LossSummary {
    std::size_t observations{};
    double mse{};
    double qlike{};
    std::size_t qlike_excluded{};
    double qlike_all_days{};
};

[[nodiscard]] LossSummary summarise_losses(std::span<const double> forecasts,
                                           std::span<const double> proxies);

[[nodiscard]] double relative_improvement_percent(double model_loss, double baseline_loss);

}
