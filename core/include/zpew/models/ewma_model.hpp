#pragma once

#include <zpew/models/vol_model.hpp>

#include <cstddef>
#include <optional>
#include <span>
#include <string>

namespace zpew::models {

inline constexpr double kRiskMetricsDailyLambda = 0.94;
inline constexpr std::size_t kDefaultEwmaSeedWindow = 21;

class EwmaModel final : public VolModel {
public:
    explicit EwmaModel(double lambda = kRiskMetricsDailyLambda,
                       std::size_t seed_window = kDefaultEwmaSeedWindow);

    void fit(std::span<const double> returns) override;
    [[nodiscard]] double forecast_variance() const override;
    void update(double new_return) override;
    [[nodiscard]] std::string name() const override;

    [[nodiscard]] double lambda() const noexcept;
    [[nodiscard]] std::size_t seed_window() const noexcept;

private:
    [[nodiscard]] double next_variance(double variance, double r) const noexcept;

    double lambda_;
    std::size_t seed_window_;
    std::optional<double> variance_;
};

}
