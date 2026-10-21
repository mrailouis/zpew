#pragma once

#include <zpew/models/vol_model.hpp>
#include <zpew/returns/rolling_mean.hpp>

#include <cstddef>
#include <optional>
#include <span>
#include <string>

namespace zpew::models {

inline constexpr std::size_t kDefaultRealisedWindow = 21;

class RealisedVolModel final : public VolModel {
public:
    explicit RealisedVolModel(std::size_t window = kDefaultRealisedWindow);

    void fit(std::span<const double> returns) override;
    [[nodiscard]] double forecast_variance() const override;
    void update(double new_return) override;
    [[nodiscard]] std::string name() const override;

    [[nodiscard]] std::size_t window() const noexcept;

private:
    std::size_t window_;
    std::optional<returns::RollingMean> squares_;
};

}
