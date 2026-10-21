#pragma once

#include "options.hpp"

#include <zpew/models/vol_model.hpp>

#include <cstddef>
#include <memory>

namespace zpew::cli {

struct ModelChoice {
    std::unique_ptr<models::VolModel> model;
    std::size_t fit_length;
};

[[nodiscard]] ModelChoice make_model(const Options& options);

}
