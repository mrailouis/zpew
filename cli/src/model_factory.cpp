#include "model_factory.hpp"

#include "options.hpp"

#include <zpew/models/ewma_model.hpp>
#include <zpew/models/garch_model.hpp>
#include <zpew/models/realised_vol_model.hpp>

#include <cstddef>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace zpew::cli {

namespace {

constexpr std::size_t kDefaultGarchFitWindow = 1000;

bool flag(const Options& options, std::string_view name) {
    const auto value = options.get(name);
    if (!value || *value == "off") {
        return false;
    }
    if (*value == "on") {
        return true;
    }
    throw UsageError{std::format("--{} expects on or off, got '{}'", name, *value)};
}

}

ModelChoice make_model(const Options& options) {
    const std::string model = options.require("model");
    if (model == "ewma") {
        auto ewma = std::make_unique<models::EwmaModel>(
            options.number("lambda", models::kRiskMetricsDailyLambda),
            options.count("seed", models::kDefaultEwmaSeedWindow));
        const std::size_t seed = ewma->seed_window();
        return {.model = std::move(ewma), .fit_length = seed};
    }
    if (model == "realised") {
        auto realised = std::make_unique<models::RealisedVolModel>(
            options.count("window", models::kDefaultRealisedWindow));
        const std::size_t window = realised->window();
        return {.model = std::move(realised), .fit_length = window};
    }
    if (model == "garch") {
        return {.model = std::make_unique<models::GarchModel>(
                    models::GarchOptions{.variance_targeting = flag(options, "targeting")}),
                .fit_length = options.count("fit-window", kDefaultGarchFitWindow)};
    }
    throw UsageError{std::format("unknown model '{}'; expected ewma, realised or garch", model)};
}

}
