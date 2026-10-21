#include "analysis_data.hpp"

#include "analysis_object.hpp"
#include "async_worker.hpp"
#include "js_error.hpp"
#include "request_settings.hpp"
#include "typed_arrays.hpp"

#include <zpew/data/snapshot.hpp>
#include <zpew/data/snapshot_data_source.hpp>
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
#include <exception>
#include <filesystem>
#include <format>
#include <memory>
#include <napi.h>
#include <string>
#include <utility>
#include <vector>

namespace zpew::native {

namespace {

constexpr double kPercent = 100.0;
constexpr double kDaysPerYearForDailyCalendar = 365.0;

std::vector<models::ModelSpec> model_specs(const AnalysisSettings& settings,
                                           const std::string& baseline) {
    std::vector<models::ModelSpec> specs{
        {.name = baseline, .parameters = models::RealisedSpec{.window = settings.baseline_window}},
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

void attach_var(ModelData& model, const evaluation::ModelEvaluation& evaluated,
                const evaluation::AssetEvaluation& asset, const data::Snapshot& snapshot,
                const std::vector<double>& times, double confidence) {
    const std::vector<double> var = var::value_at_risk_series(evaluated.forecasts, confidence);
    const std::size_t first_bar = snapshot.bars.size() - asset.dates.size();
    model.var_price.reserve(var.size());
    for (std::size_t i = 0; i < var.size(); ++i) {
        model.var_price.push_back(snapshot.bars[first_bar + i - 1].close * std::exp(-var[i]));
        if (asset.returns[i] < -var[i]) {
            model.breach_times.push_back(times[i]);
            model.breach_prices.push_back(snapshot.bars[first_bar + i].close);
        }
    }
    model.backtest = var::backtest(evaluated.forecasts, asset.returns, confidence);
}

void attach_improvements(ModelData& model,
                         const std::vector<evaluation::Improvement>& improvements) {
    for (const evaluation::Improvement& improvement : improvements) {
        if (improvement.model != model.name) {
            continue;
        }
        if (improvement.proxy == "squared_return") {
            model.squared_return_improvement = improvement;
        } else {
            model.parkinson_improvement = improvement;
        }
    }
}

class AnalyseWorker final : public Napi::AsyncWorker {
public:
    AnalyseWorker(Napi::Env env, std::string manifest_path, std::string symbol,
                  AnalysisSettings settings)
        : Napi::AsyncWorker{env}, deferred_{Napi::Promise::Deferred::New(env)},
          manifest_path_{std::move(manifest_path)}, symbol_{std::move(symbol)},
          settings_{settings} {}

    [[nodiscard]] Napi::Promise promise() const { return deferred_.Promise(); }

    void Execute() override {
        try {
            const data::SnapshotDataSource source{std::filesystem::path{manifest_path_}};
            result_ = build_analysis_data(source.load_snapshot(symbol_), settings_);
        } catch (const std::exception& error) {
            SetError(error.what());
        }
    }

    void OnOK() override { deferred_.Resolve(to_js_object(Env(), result_)); }

    void OnError(const Napi::Error& error) override { deferred_.Reject(error.Value()); }

private:
    Napi::Promise::Deferred deferred_;
    std::string manifest_path_;
    std::string symbol_;
    AnalysisSettings settings_;
    AnalysisData result_;
};

}

AnalysisData build_analysis_data(const data::Snapshot& snapshot, const AnalysisSettings& settings) {
    const auto start = std::chrono::steady_clock::now();
    AnalysisData analysis{.symbol = snapshot.asset.symbol,
                          .baseline = std::format("rv{}", settings.baseline_window),
                          .settings = settings};
    const evaluation::AssetEvaluation asset = evaluation::evaluate_asset(
        snapshot, model_specs(settings, analysis.baseline),
        {.window = settings.window, .refit_every = settings.refit_every, .mode = settings.mode});
    for (const auto date : asset.dates) {
        analysis.times.push_back(seconds_since_epoch(date));
    }
    const double periods_per_year = snapshot.asset.calendar == data::TradingCalendar::kDaily
                                        ? kDaysPerYearForDailyCalendar
                                        : returns::kTradingDaysPerYear;
    const std::vector<evaluation::Improvement> improvements =
        evaluation::improvements_over_baseline(asset, analysis.baseline);

    for (const evaluation::ModelEvaluation& evaluated : asset.models) {
        ModelData model{.name = evaluated.model,
                        .failure = evaluated.failure,
                        .refits = evaluated.refits,
                        .boundary_refits = evaluated.boundary_refits,
                        .losses = evaluated.losses};
        if (!evaluated.failure) {
            model.volatility_percent.reserve(evaluated.forecasts.size());
            for (const double variance : evaluated.forecasts) {
                model.volatility_percent.push_back(
                    kPercent * returns::annualise_volatility(variance, periods_per_year));
            }
            attach_var(model, evaluated, asset, snapshot, analysis.times, settings.confidence);
        }
        attach_improvements(model, improvements);
        analysis.models.push_back(std::move(model));
    }
    analysis.seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    return analysis;
}

Napi::Value analyse(const Napi::CallbackInfo& info) {
    const Napi::Env env = info.Env();
    std::string manifest_path = require_string(info, 0, "manifestPath");
    std::string symbol = require_string(info, 1, "symbol");
    const AnalysisSettings settings = parse_analysis_settings(env, info[2]);
    return queue_worker(std::make_unique<AnalyseWorker>(env, std::move(manifest_path),
                                                        std::move(symbol), settings));
}

}
