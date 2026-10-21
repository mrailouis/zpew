#pragma once

#include "analysis_data.hpp"

#include <napi.h>

namespace zpew::native {

[[nodiscard]] Napi::Object to_js_object(Napi::Env env, const AnalysisData& analysis);

}
