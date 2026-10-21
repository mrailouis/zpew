#pragma once

#include <chrono>
#include <napi.h>
#include <span>

namespace zpew::native {

[[nodiscard]] Napi::Float64Array to_float64_array(Napi::Env env, std::span<const double> values);
[[nodiscard]] double seconds_since_epoch(std::chrono::sys_days date) noexcept;

}
