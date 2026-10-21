#include "asset_list.hpp"
#include "build_info.hpp"
#include "js_error.hpp"

#include <zpew/data/manifest.hpp>
#include <zpew/version.hpp>

#include <filesystem>
#include <napi.h>
#include <string>

namespace zpew::native {

namespace {

Napi::Value version(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), std::string{zpew::version()});
}

Napi::Value build(const Napi::CallbackInfo& info) {
    const BuildInfo build = build_info();
    Napi::Object result = Napi::Object::New(info.Env());
    result.Set("commit", std::string{build.commit});
    result.Set("dirty", build.dirty);
    return result;
}

Napi::Value find_manifest(const Napi::CallbackInfo& info) {
    const std::string start = require_string(info, 0, "start");
    const auto found = data::find_manifest(std::filesystem::path{start});
    if (!found) {
        return info.Env().Null();
    }
    return Napi::String::New(info.Env(), found->string());
}

}

Napi::Object initialise(Napi::Env env, Napi::Object exports) {
    exports.Set("version", Napi::Function::New(env, version));
    exports.Set("buildInfo", Napi::Function::New(env, build));
    exports.Set("findManifest", Napi::Function::New(env, find_manifest));
    exports.Set("listAssets", Napi::Function::New(env, list_assets));
    return exports;
}

}

Napi::Object zpew_native_initialise(Napi::Env env, Napi::Object exports) {
    return zpew::native::initialise(env, exports);
}

NODE_API_MODULE(zpew_native, zpew_native_initialise)
