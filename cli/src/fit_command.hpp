#pragma once

#include "options.hpp"

#include <ostream>

namespace zpew::cli {

[[nodiscard]] int run_fit(const Options& options, std::ostream& out);

}
