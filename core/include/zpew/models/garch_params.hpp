#pragma once

#include <array>
#include <span>

namespace zpew::models {

struct GarchParams {
    double omega{};
    double alpha{};
    double beta{};

    [[nodiscard]] double persistence() const noexcept { return alpha + beta; }
    [[nodiscard]] double long_run_variance() const noexcept {
        return omega / (1.0 - persistence());
    }

    friend bool operator==(const GarchParams&, const GarchParams&) = default;
};

using GarchTheta = std::array<double, 3>;

[[nodiscard]] GarchParams params_from_theta(std::span<const double, 3> theta) noexcept;
[[nodiscard]] GarchTheta theta_from_params(const GarchParams& params);

[[nodiscard]] double mean_square(std::span<const double> returns);

[[nodiscard]] double garch_log_likelihood(std::span<const double> returns,
                                          const GarchParams& params, double backcast);

[[nodiscard]] double garch_next_variance(const GarchParams& params, double variance,
                                         double r) noexcept;

}
