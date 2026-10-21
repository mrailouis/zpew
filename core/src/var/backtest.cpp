#include <zpew/error.hpp>
#include <zpew/var/backtest.hpp>
#include <zpew/var/normal_quantile.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <span>
#include <vector>

namespace zpew::var {

namespace {

double x_log_y(double x, double y) {
    return x == 0.0 ? 0.0 : x * std::log(y);
}

void require_confidence(double confidence) {
    const bool valid = confidence > 0.5 && confidence < 1.0;
    if (!valid) {
        throw InvalidArgument{
            std::format("VaR confidence must be in (0.5, 1), got {}", confidence)};
    }
}

}

double value_at_risk(double variance, double confidence) {
    require_confidence(confidence);
    if (!std::isfinite(variance) || !(variance > 0.0)) {
        throw InvalidArgument{
            std::format("VaR needs a positive finite variance, got {}", variance)};
    }
    return normal_quantile(confidence) * std::sqrt(variance);
}

std::vector<double> value_at_risk_series(std::span<const double> variances, double confidence) {
    std::vector<double> series;
    series.reserve(variances.size());
    for (const double variance : variances) {
        series.push_back(value_at_risk(variance, confidence));
    }
    return series;
}

KupiecTest kupiec_test(std::size_t observations, std::size_t breaches, double expected_rate) {
    if (observations == 0 || breaches > observations) {
        throw InvalidArgument{std::format("Kupiec test needs 0 <= breaches <= observations > 0, "
                                          "got {} breaches in {}",
                                          breaches, observations)};
    }
    const bool valid_rate = expected_rate > 0.0 && expected_rate < 1.0;
    if (!valid_rate) {
        throw InvalidArgument{
            std::format("expected breach rate must be in (0, 1), got {}", expected_rate)};
    }
    const auto n = static_cast<double>(observations);
    const auto x = static_cast<double>(breaches);
    const double observed = x / n;
    const double restricted = x_log_y(n - x, 1.0 - expected_rate) + x_log_y(x, expected_rate);
    const double unrestricted = x_log_y(n - x, 1.0 - observed) + x_log_y(x, observed);
    const double likelihood_ratio = std::max(0.0, -2.0 * (restricted - unrestricted));
    return {.likelihood_ratio = likelihood_ratio,
            .p_value = std::erfc(std::sqrt(likelihood_ratio / 2.0))};
}

BacktestResult backtest(std::span<const double> variances, std::span<const double> returns,
                        double confidence) {
    if (variances.size() != returns.size()) {
        throw InvalidArgument{
            std::format("{} variance forecasts but {} returns", variances.size(), returns.size())};
    }
    const std::vector<double> var = value_at_risk_series(variances, confidence);
    std::size_t breaches = 0;
    for (std::size_t i = 0; i < returns.size(); ++i) {
        if (!std::isfinite(returns[i])) {
            throw InvalidArgument{std::format("return {} at {} must be finite", returns[i], i)};
        }
        if (returns[i] < -var[i]) {
            ++breaches;
        }
    }
    const double expected_rate = 1.0 - confidence;
    return {.confidence = confidence,
            .observations = returns.size(),
            .breaches = breaches,
            .breach_rate = returns.empty() ? 0.0
                                           : static_cast<double>(breaches) /
                                                 static_cast<double>(returns.size()),
            .expected_rate = expected_rate,
            .kupiec = kupiec_test(returns.size(), breaches, expected_rate)};
}

}
