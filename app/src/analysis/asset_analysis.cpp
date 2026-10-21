#include "analysis/asset_analysis.hpp"

#include "analysis/analysis_settings.hpp"

#include <zpew/data/snapshot.hpp>
#include <zpew/data/trading_calendar.hpp>
#include <zpew/evaluation/asset_evaluation.hpp>
#include <zpew/evaluation/improvement.hpp>
#include <zpew/models/ewma_model.hpp>
#include <zpew/models/model_spec.hpp>
#include <zpew/returns/realised_variance.hpp>
#include <zpew/var/backtest.hpp>

#include <chrono>
#include <cmath>
#include <cstddef>
#include <format>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace zpew::app {

namespace {

constexpr double kPercent = 100.0;
constexpr double kDaysPerYearForDailyCalendar = 365.0;

std::vector<models::ModelSpec> model_specs(const AnalysisSettings& settings,
                                           std::string& baseline) {
    baseline = std::format("rv{}", settings.baseline_window);
    std::vector<models::ModelSpec> specs{
        {.name = baseline,
         .parameters =
             models::RealisedSpec{.window = static_cast<std::size_t>(settings.baseline_window)}},
        {.name = "ewma",
         .parameters = models::EwmaSpec{.lambda = settings.ewma_lambda,
                                        .seed_window = models::kDefaultEwmaSeedWindow}},
    };
    if (settings.include_garch) {
        specs.push_back(
            {.name = "garch", .parameters = models::GarchSpec{.variance_targeting = false}});
    }
    return specs;
}

double seconds_since_epoch(std::chrono::sys_days date) {
    return static_cast<double>(
        std::chrono::duration_cast<std::chrono::seconds>(date.time_since_epoch()).count());
}

void attach_var(ModelAnalysis& model, const evaluation::ModelEvaluation& evaluated,
                const evaluation::AssetEvaluation& asset, const data::Snapshot& snapshot,
                const std::vector<double>& times, double confidence) {
    const std::vector<double> var = var::value_at_risk_series(evaluated.forecasts, confidence);
    const std::size_t first_bar = snapshot.bars.size() - asset.dates.size();
    model.var_price.reserve(var.size());
    for (std::size_t i = 0; i < var.size(); ++i) {
        const double previous_close = snapshot.bars[first_bar + i - 1].close;
        model.var_price.push_back(previous_close * std::exp(-var[i]));
        if (asset.returns[i] < -var[i]) {
            model.breach_times.push_back(times[i]);
            model.breach_prices.push_back(snapshot.bars[first_bar + i].close);
        }
    }
    model.backtest = var::backtest(evaluated.forecasts, asset.returns, confidence);
}

}

AssetAnalysis analyse_asset(const data::Snapshot& snapshot, const AnalysisSettings& settings) {
    const auto start = std::chrono::steady_clock::now();
    AssetAnalysis analysis{.symbol = snapshot.asset.symbol, .settings = settings};
    const std::vector<models::ModelSpec> specs = model_specs(settings, analysis.baseline);
    const evaluation::AssetEvaluation asset =
        evaluation::evaluate_asset(snapshot, specs,
                                   {.window = static_cast<std::size_t>(settings.window),
                                    .refit_every = static_cast<std::size_t>(settings.refit_every),
                                    .mode = settings.mode});

    for (const auto date : asset.dates) {
        analysis.times.push_back(seconds_since_epoch(date));
    }
    const double periods_per_year = snapshot.asset.calendar == data::TradingCalendar::kDaily
                                        ? kDaysPerYearForDailyCalendar
                                        : returns::kTradingDaysPerYear;
    const std::vector<evaluation::Improvement> improvements =
        evaluation::improvements_over_baseline(asset, analysis.baseline);

    for (const evaluation::ModelEvaluation& evaluated : asset.models) {
        ModelAnalysis model{.name = evaluated.model,
                            .failure = evaluated.failure,
                            .losses = evaluated.losses,
                            .refits = evaluated.refits,
                            .boundary_refits = evaluated.boundary_refits};
        if (!evaluated.failure) {
            for (const double variance : evaluated.forecasts) {
                model.volatility_percent.push_back(
                    kPercent * returns::annualise_volatility(variance, periods_per_year));
            }
            attach_var(model, evaluated, asset, snapshot, analysis.times, settings.confidence);
        }
        for (const evaluation::Improvement& improvement : improvements) {
            if (improvement.model != evaluated.model) {
                continue;
            }
            if (improvement.proxy == "squared_return") {
                model.squared_return_improvement = improvement;
            } else {
                model.parkinson_improvement = improvement;
            }
        }
        analysis.models.push_back(std::move(model));
    }
    analysis.seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    return analysis;
}

}
