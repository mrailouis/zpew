#pragma once

#include <napi.h>

namespace zpew::native {

[[nodiscard]] Napi::Value list_assets(const Napi::CallbackInfo& info);

}
