#include <zpew/error.hpp>
#include <zpew/evaluation/asset_evaluation.hpp>
#include <zpew/evaluation/improvement.hpp>
#include <zpew/evaluation/losses.hpp>
#include <zpew/evaluation/rolling_forecast.hpp>

#include <algorithm>
#include <cstddef>
#include <format>
#include <map>
#include <numeric>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace zpew::evaluation {

namespace {

constexpr std::string_view kSquaredReturn = "squared_return";
constexpr std::string_view kParkinson = "parkinson";

const ModelEvaluation* find_model(const AssetEvaluation& asset, std::string_view name) {
    const auto it = std::ranges::find(asset.models, name, &ModelEvaluation::model);
    return it == asset.models.end() ? nullptr : &*it;
}

Improvement compare(const AssetEvaluation& asset, const ModelEvaluation& model,
                    const LossSummary& model_losses, const LossSummary& baseline_losses,
                    std::string_view proxy) {
    return {.symbol = asset.symbol,
            .mode = asset.mode,
            .model = model.model,
            .proxy = std::string{proxy},
            .mse_percent = relative_improvement_percent(model_losses.mse, baseline_losses.mse),
            .qlike_percent =
                relative_improvement_percent(model_losses.qlike, baseline_losses.qlike)};
}

}

std::vector<Improvement> improvements_over_baseline(const AssetEvaluation& asset,
                                                    std::string_view baseline) {
    const ModelEvaluation* base = find_model(asset, baseline);
    if (base == nullptr) {
        throw InvalidArgument{std::format("baseline model '{}' was not evaluated", baseline)};
    }
    std::vector<Improvement> result;
    if (!base->losses) {
        return result;
    }
    const ModelLosses& base_losses = *base->losses;
    for (const ModelEvaluation& model : asset.models) {
        if (model.model == baseline || !model.losses) {
            continue;
        }
        const ModelLosses& losses = *model.losses;
        result.push_back(compare(asset, model, losses.squared_return, base_losses.squared_return,
                                 kSquaredReturn));
        result.push_back(
            compare(asset, model, losses.parkinson, base_losses.parkinson, kParkinson));
    }
    return result;
}

double median(std::vector<double> values) {
    if (values.empty()) {
        throw InvalidArgument{"median of an empty set"};
    }
    const std::size_t middle = values.size() / 2;
    std::ranges::nth_element(values, values.begin() + static_cast<std::ptrdiff_t>(middle));
    const double upper = values[middle];
    if (values.size() % 2 == 1) {
        return upper;
    }
    const double lower =
        *std::max_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(middle));
    return (lower + upper) / 2.0;
}

std::vector<PooledImprovement> pool_improvements(std::span<const Improvement> improvements) {
    using Key = std::tuple<WindowMode, std::string, std::string, std::string>;
    std::map<Key, std::vector<double>> groups;
    for (const Improvement& item : improvements) {
        groups[{item.mode, item.model, item.proxy, "mse"}].push_back(item.mse_percent);
        groups[{item.mode, item.model, item.proxy, "qlike"}].push_back(item.qlike_percent);
    }
    std::vector<PooledImprovement> pooled;
    for (const auto& [key, values] : groups) {
        const auto& [mode, model, proxy, metric] = key;
        const double sum = std::accumulate(values.begin(), values.end(), 0.0);
        pooled.push_back({.mode = mode,
                          .model = model,
                          .proxy = proxy,
                          .metric = metric,
                          .assets = values.size(),
                          .mean_percent = sum / static_cast<double>(values.size()),
                          .median_percent = median(values),
                          .wins = static_cast<std::size_t>(
                              std::ranges::count_if(values, [](double v) { return v > 0.0; }))});
    }
    return pooled;
}

}
