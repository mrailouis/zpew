#pragma once

#include <zpew/models/garch_params.hpp>
#include <zpew/models/vol_model.hpp>

#include <cstddef>
#include <optional>
#include <span>
#include <string>

namespace zpew::models {

inline constexpr std::size_t kGarchMinimumFitLength = 500;
inline constexpr double kGarchBoundaryPersistence = 1.0 - 1e-6;

struct GarchOptions {
    bool variance_targeting = false;
    std::size_t minimum_fit_length = kGarchMinimumFitLength;
};

struct GarchFit {
    GarchParams params;
    double log_likelihood{};
    double backcast{};
    std::size_t observations{};
    int starts{};
    int converged_starts{};
    int iterations{};
    bool at_boundary{};
};

class GarchModel final : public VolModel {
public:
    explicit GarchModel(GarchOptions options = {});

    void fit(std::span<const double> returns) override;
    [[nodiscard]] double forecast_variance() const override;
    void update(double new_return) override;
    [[nodiscard]] std::string name() const override;
    [[nodiscard]] bool fit_at_boundary() const noexcept override;

    [[nodiscard]] const GarchFit& fitted() const;

private:
    GarchOptions options_;
    std::optional<GarchFit> fit_;
    std::optional<double> variance_;
};

}
