#pragma once

#include "options.hpp"

#include <ostream>

namespace zpew::cli {

[[nodiscard]] int run_forecast(const Options& options, std::ostream& out);

}
