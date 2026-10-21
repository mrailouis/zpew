#include <zpew/data/bar.hpp>
#include <zpew/data/snapshot.hpp>
#include <zpew/error.hpp>
#include <zpew/evaluation/asset_evaluation.hpp>
#include <zpew/evaluation/losses.hpp>
#include <zpew/evaluation/proxies.hpp>
#include <zpew/evaluation/rolling_forecast.hpp>
#include <zpew/models/model_spec.hpp>
#include <zpew/models/vol_model.hpp>
#include <zpew/returns/log_returns.hpp>

#include <cstddef>
#include <format>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace zpew::evaluation {

namespace {

std::vector<double> tail(const std::vector<double>& values, std::size_t first) {
    return {values.begin() + static_cast<std::ptrdiff_t>(first), values.end()};
}

ModelEvaluation evaluate_model(const models::ModelSpec& spec, std::span<const double> returns,
                               const AssetEvaluation& asset, const RollingOptions& options) {
    ModelEvaluation result{.model = spec.name};
    if (models::minimum_fit_length(spec) > options.window) {
        result.failure =
            std::format("needs a window of at least {} returns", models::minimum_fit_length(spec));
        return result;
    }
    try {
        const std::unique_ptr<models::VolModel> model = models::make_model(spec);
        ForecastSeries series = rolling_forecasts(*model, returns, options);
        result.refits = series.refits;
        result.boundary_refits = series.boundary_refits;
        result.losses = ModelLosses{
            .squared_return = summarise_losses(series.forecasts, asset.squared_return_proxy),
            .parkinson = summarise_losses(series.forecasts, asset.parkinson_proxy)};
        result.forecasts = std::move(series.forecasts);
    } catch (const Error& error) {
        result.failure = error.what();
    }
    return result;
}

}

AssetEvaluation evaluate_asset(const data::Snapshot& snapshot,
                               std::span<const models::ModelSpec> specs,
                               const RollingOptions& options) {
    const std::vector<double> returns =
        returns::log_returns(std::span<const data::Bar>{snapshot.bars});
    if (returns.size() <= options.window) {
        throw InvalidArgument{std::format("{} has {} returns; the estimation window needs more "
                                          "than {}",
                                          snapshot.asset.symbol, returns.size(), options.window)};
    }
    const std::size_t first = options.window;

    AssetEvaluation asset{.symbol = snapshot.asset.symbol, .mode = options.mode};
    for (std::size_t t = first; t < returns.size(); ++t) {
        asset.dates.push_back(snapshot.bars[t + 1].date);
    }
    asset.returns = tail(returns, first);
    asset.squared_return_proxy = tail(squared_return_proxy(returns), first);
    asset.parkinson_proxy = tail(parkinson_proxy(std::span<const data::Bar>{snapshot.bars}), first);

    for (const models::ModelSpec& spec : specs) {
        asset.models.push_back(evaluate_model(spec, returns, asset, options));
    }
    return asset;
}

}
