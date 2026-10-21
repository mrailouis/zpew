#pragma once

#include "options.hpp"

#include <ostream>

namespace zpew::cli {

[[nodiscard]] int run_backtest(const Options& options, std::ostream& out, std::ostream& err);

}
