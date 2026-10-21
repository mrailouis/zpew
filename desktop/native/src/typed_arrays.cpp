#include "typed_arrays.hpp"

#include <algorithm>
#include <chrono>
#include <napi.h>
#include <span>

namespace zpew::native {

Napi::Float64Array to_float64_array(Napi::Env env, std::span<const double> values) {
    Napi::Float64Array array = Napi::Float64Array::New(env, values.size());
    std::ranges::copy(values, array.Data());
    return array;
}

double seconds_since_epoch(std::chrono::sys_days date) noexcept {
    return static_cast<double>(
        std::chrono::duration_cast<std::chrono::seconds>(date.time_since_epoch()).count());
}

}
