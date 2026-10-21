#pragma once

#include <zpew/models/vol_model.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <variant>

namespace zpew::models {

struct RealisedSpec {
    std::size_t window{};
};

struct EwmaSpec {
    double lambda{};
    std::size_t seed_window{};
};

struct GarchSpec {
    bool variance_targeting{};
};

struct ModelSpec {
    std::string name;
    std::variant<RealisedSpec, EwmaSpec, GarchSpec> parameters;
};

[[nodiscard]] std::unique_ptr<VolModel> make_model(const ModelSpec& spec);
[[nodiscard]] std::size_t minimum_fit_length(const ModelSpec& spec);

}
