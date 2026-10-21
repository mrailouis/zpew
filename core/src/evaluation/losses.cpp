#include <zpew/error.hpp>
#include <zpew/evaluation/losses.hpp>

#include <cmath>
#include <cstddef>
#include <format>
#include <limits>
#include <span>

namespace zpew::evaluation {

namespace {

constexpr double kPercent = 100.0;

void validate(std::span<const double> forecasts, std::span<const double> proxies) {
    if (forecasts.size() != proxies.size()) {
        throw InvalidArgument{
            std::format("{} forecasts but {} proxy values", forecasts.size(), proxies.size())};
    }
    if (forecasts.empty()) {
        throw InvalidArgument{"no forecasts to evaluate"};
    }
    for (std::size_t i = 0; i < forecasts.size(); ++i) {
        const bool forecast_ok = std::isfinite(forecasts[i]) && forecasts[i] > 0.0;
        const bool proxy_ok = std::isfinite(proxies[i]) && proxies[i] >= 0.0;
        if (!forecast_ok || !proxy_ok) {
            throw InvalidArgument{std::format("invalid pair at {}: forecast {}, proxy {}", i,
                                              forecasts[i], proxies[i])};
        }
    }
}

}

LossSummary summarise_losses(std::span<const double> forecasts, std::span<const double> proxies) {
    validate(forecasts, proxies);
    double squared_error = 0.0;
    double qlike = 0.0;
    double qlike_all_days = 0.0;
    std::size_t qlike_count = 0;
    for (std::size_t i = 0; i < forecasts.size(); ++i) {
        const double f = forecasts[i];
        const double h = proxies[i];
        const double error = f - h;
        squared_error += error * error;
        qlike_all_days += std::log(f) + (h / f);
        if (h > 0.0) {
            const double ratio = h / f;
            qlike += ratio - std::log(ratio) - 1.0;
            ++qlike_count;
        }
    }
    const auto n = static_cast<double>(forecasts.size());
    return LossSummary{
        .observations = forecasts.size(),
        .mse = squared_error / n,
        .qlike = qlike_count > 0 ? qlike / static_cast<double>(qlike_count)
                                 : std::numeric_limits<double>::quiet_NaN(),
        .qlike_excluded = forecasts.size() - qlike_count,
        .qlike_all_days = qlike_all_days / n,
    };
}

double relative_improvement_percent(double model_loss, double baseline_loss) {
    if (!(baseline_loss > 0.0) || !std::isfinite(baseline_loss) || !std::isfinite(model_loss)) {
        throw InvalidArgument{
            std::format("cannot compare loss {} with baseline {}", model_loss, baseline_loss)};
    }
    return kPercent * (1.0 - (model_loss / baseline_loss));
}

}
