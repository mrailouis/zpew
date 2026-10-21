#include "js_error.hpp"

#include <cstddef>
#include <format>
#include <napi.h>
#include <string>
#include <string_view>

namespace zpew::native {

void throw_js_error(Napi::Env env, std::string_view message) {
    throw Napi::Error::New(env, std::string{message});
}

std::string require_string(const Napi::CallbackInfo& info, std::size_t index,
                           std::string_view name) {
    if (info.Length() <= index || !info[index].IsString()) {
        throw_js_error(info.Env(), std::format("argument '{}' must be a string", name));
    }
    return info[index].As<Napi::String>().Utf8Value();
}

}
