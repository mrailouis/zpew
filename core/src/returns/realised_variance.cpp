#include <zpew/error.hpp>
#include <zpew/returns/realised_variance.hpp>
#include <zpew/returns/rolling_mean_of_squares.hpp>

#include <cmath>
#include <cstddef>
#include <format>
#include <span>
#include <vector>

namespace zpew::returns {

std::vector<double> rolling_realised_variance(std::span<const double> returns, std::size_t window) {
    RollingMeanOfSquares rolling{window};
    std::vector<double> variances;
    if (returns.size() >= window) {
        variances.reserve(returns.size() - window + 1);
    }
    for (const double r : returns) {
        rolling.push(r);
        if (rolling.full()) {
            variances.push_back(rolling.mean());
        }
    }
    return variances;
}

double annualise_volatility(double daily_variance, double periods_per_year) {
    if (!std::isfinite(daily_variance) || daily_variance < 0.0) {
        throw InvalidArgument{
            std::format("variance {} must be finite and non-negative", daily_variance)};
    }
    if (!std::isfinite(periods_per_year) || periods_per_year <= 0.0) {
        throw InvalidArgument{
            std::format("periods per year {} must be finite and positive", periods_per_year)};
    }
    return std::sqrt(daily_variance * periods_per_year);
}

}
