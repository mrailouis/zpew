#pragma once

#include <span>
#include <string>

namespace zpew::models {

class VolModel {
public:
    virtual ~VolModel() = default;

    virtual void fit(std::span<const double> returns) = 0;
    [[nodiscard]] virtual double forecast_variance() const = 0;
    virtual void update(double new_return) = 0;
    [[nodiscard]] virtual std::string name() const = 0;
    [[nodiscard]] virtual bool fit_at_boundary() const noexcept { return false; }

protected:
    VolModel() = default;
    VolModel(const VolModel&) = default;
    VolModel& operator=(const VolModel&) = default;
    VolModel(VolModel&&) = default;
    VolModel& operator=(VolModel&&) = default;
};

}
