#include "request_settings.hpp"

#include "js_error.hpp"

#include <zpew/evaluation/rolling_forecast.hpp>

#include <cmath>
#include <cstddef>
#include <format>
#include <napi.h>
#include <string>
#include <string_view>

namespace zpew::native {

namespace {

constexpr std::size_t kMaximumWindow = 20000;

Napi::Object require_object(Napi::Env env, const Napi::Value& value, std::string_view name) {
    if (!value.IsObject()) {
        throw_js_error(env, std::format("'{}' must be an object", name));
    }
    return value.As<Napi::Object>();
}

double require_number(Napi::Env env, const Napi::Object& object, std::string_view key) {
    const Napi::Value value = object.Get(std::string{key});
    if (!value.IsNumber() || !std::isfinite(value.As<Napi::Number>().DoubleValue())) {
        throw_js_error(env, std::format("setting '{}' must be a finite number", key));
    }
    return value.As<Napi::Number>().DoubleValue();
}

std::size_t require_count(Napi::Env env, const Napi::Object& object, std::string_view key,
                          std::size_t minimum) {
    const double value = require_number(env, object, key);
    const bool whole = std::floor(value) == value;
    if (!whole || value < static_cast<double>(minimum) ||
        value > static_cast<double>(kMaximumWindow)) {
        throw_js_error(env, std::format("setting '{}' must be a whole number from {} to {}", key,
                                        minimum, kMaximumWindow));
    }
    return static_cast<std::size_t>(value);
}

bool require_bool(Napi::Env env, const Napi::Object& object, std::string_view key) {
    const Napi::Value value = object.Get(std::string{key});
    if (!value.IsBoolean()) {
        throw_js_error(env, std::format("setting '{}' must be true or false", key));
    }
    return value.As<Napi::Boolean>().Value();
}

double require_open_interval(Napi::Env env, const Napi::Object& object, std::string_view key,
                             double lower, double upper) {
    const double value = require_number(env, object, key);
    const bool inside = value > lower && value < upper;
    if (!inside) {
        throw_js_error(env, std::format("setting '{}' must be between {} and {} (exclusive)", key,
                                        lower, upper));
    }
    return value;
}

evaluation::WindowMode require_mode(Napi::Env env, const Napi::Object& object) {
    const Napi::Value value = object.Get("mode");
    if (!value.IsString()) {
        throw_js_error(env, "setting 'mode' must be 'sliding' or 'expanding'");
    }
    const auto mode = evaluation::parse_window_mode(value.As<Napi::String>().Utf8Value());
    if (!mode) {
        throw_js_error(env, "setting 'mode' must be 'sliding' or 'expanding'");
    }
    return *mode;
}

}

ChartSettings parse_chart_settings(Napi::Env env, const Napi::Value& value) {
    const Napi::Object object = require_object(env, value, "settings");
    return {.fast_average = require_count(env, object, "fastAverage", 1),
            .slow_average = require_count(env, object, "slowAverage", 1),
            .volatility_window = require_count(env, object, "volatilityWindow", 1)};
}

AnalysisSettings parse_analysis_settings(Napi::Env env, const Napi::Value& value) {
    const Napi::Object object = require_object(env, value, "settings");
    return {.window = require_count(env, object, "window", 2),
            .refit_every = require_count(env, object, "refitEvery", 1),
            .mode = require_mode(env, object),
            .baseline_window = require_count(env, object, "baselineWindow", 1),
            .ewma_lambda = require_open_interval(env, object, "ewmaLambda", 0.0, 1.0),
            .include_garch = require_bool(env, object, "includeGarch"),
            .confidence = require_open_interval(env, object, "confidence", 0.5, 1.0)};
}

}
