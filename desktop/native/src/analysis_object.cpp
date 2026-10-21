#include "analysis_object.hpp"

#include "analysis_data.hpp"
#include "typed_arrays.hpp"

#include <zpew/evaluation/improvement.hpp>
#include <zpew/evaluation/losses.hpp>
#include <zpew/evaluation/rolling_forecast.hpp>
#include <zpew/var/backtest.hpp>

#include <cstddef>
#include <cstdint>
#include <napi.h>
#include <optional>
#include <string>

namespace zpew::native {

namespace {

Napi::Object loss_object(Napi::Env env, const evaluation::LossSummary& losses) {
    Napi::Object object = Napi::Object::New(env);
    object.Set("observations", static_cast<double>(losses.observations));
    object.Set("mse", losses.mse);
    object.Set("qlike", losses.qlike);
    object.Set("qlikeExcluded", static_cast<double>(losses.qlike_excluded));
    return object;
}

Napi::Value improvement_value(Napi::Env env,
                              const std::optional<evaluation::Improvement>& improvement) {
    if (!improvement) {
        return env.Null();
    }
    Napi::Object object = Napi::Object::New(env);
    object.Set("msePercent", improvement->mse_percent);
    object.Set("qlikePercent", improvement->qlike_percent);
    return object;
}

Napi::Value backtest_value(Napi::Env env, const std::optional<var::BacktestResult>& backtest) {
    if (!backtest) {
        return env.Null();
    }
    Napi::Object object = Napi::Object::New(env);
    object.Set("confidence", backtest->confidence);
    object.Set("observations", static_cast<double>(backtest->observations));
    object.Set("breaches", static_cast<double>(backtest->breaches));
    object.Set("breachRate", backtest->breach_rate);
    object.Set("expectedRate", backtest->expected_rate);
    object.Set("kupiecLikelihoodRatio", backtest->kupiec.likelihood_ratio);
    object.Set("kupiecPValue", backtest->kupiec.p_value);
    return object;
}

Napi::Object model_object(Napi::Env env, const ModelData& model) {
    Napi::Object object = Napi::Object::New(env);
    object.Set("name", model.name);
    object.Set("failure",
               model.failure ? Napi::Value{Napi::String::New(env, *model.failure)} : env.Null());
    object.Set("refits", static_cast<double>(model.refits));
    object.Set("boundaryRefits", static_cast<double>(model.boundary_refits));
    object.Set("volatilityPercent", to_float64_array(env, model.volatility_percent));
    object.Set("varPrice", to_float64_array(env, model.var_price));
    object.Set("breachTimes", to_float64_array(env, model.breach_times));
    object.Set("breachPrices", to_float64_array(env, model.breach_prices));
    if (model.losses) {
        Napi::Object losses = Napi::Object::New(env);
        losses.Set("squaredReturn", loss_object(env, model.losses->squared_return));
        losses.Set("parkinson", loss_object(env, model.losses->parkinson));
        object.Set("losses", losses);
    } else {
        object.Set("losses", env.Null());
    }
    Napi::Object improvements = Napi::Object::New(env);
    improvements.Set("squaredReturn", improvement_value(env, model.squared_return_improvement));
    improvements.Set("parkinson", improvement_value(env, model.parkinson_improvement));
    object.Set("improvements", improvements);
    object.Set("backtest", backtest_value(env, model.backtest));
    return object;
}

}

Napi::Object to_js_object(Napi::Env env, const AnalysisData& analysis) {
    Napi::Object object = Napi::Object::New(env);
    object.Set("symbol", analysis.symbol);
    object.Set("baseline", analysis.baseline);
    object.Set("window", static_cast<double>(analysis.settings.window));
    object.Set("refitEvery", static_cast<double>(analysis.settings.refit_every));
    object.Set("mode", std::string{evaluation::to_string(analysis.settings.mode)});
    object.Set("confidence", analysis.settings.confidence);
    object.Set("seconds", analysis.seconds);
    object.Set("times", to_float64_array(env, analysis.times));
    Napi::Array models = Napi::Array::New(env, analysis.models.size());
    for (std::size_t i = 0; i < analysis.models.size(); ++i) {
        models.Set(static_cast<std::uint32_t>(i), model_object(env, analysis.models[i]));
    }
    object.Set("models", models);
    return object;
}

}
