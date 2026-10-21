#pragma once

#include <napi.h>
#include <string_view>

namespace zpew::native {

[[noreturn]] void throw_js_error(Napi::Env env, std::string_view message);

[[nodiscard]] std::string require_string(const Napi::CallbackInfo& info, std::size_t index,
                                         std::string_view name);

}
