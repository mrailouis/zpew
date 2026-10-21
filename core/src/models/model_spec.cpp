#include <zpew/models/ewma_model.hpp>
#include <zpew/models/garch_model.hpp>
#include <zpew/models/model_spec.hpp>
#include <zpew/models/realised_vol_model.hpp>
#include <zpew/models/vol_model.hpp>

#include <cstddef>
#include <memory>
#include <type_traits>
#include <variant>

namespace zpew::models {

std::unique_ptr<VolModel> make_model(const ModelSpec& spec) {
    return std::visit(
        [](const auto& p) -> std::unique_ptr<VolModel> {
            using Parameters = std::decay_t<decltype(p)>;
            if constexpr (std::is_same_v<Parameters, RealisedSpec>) {
                return std::make_unique<RealisedVolModel>(p.window);
            } else if constexpr (std::is_same_v<Parameters, EwmaSpec>) {
                return std::make_unique<EwmaModel>(p.lambda, p.seed_window);
            } else {
                return std::make_unique<GarchModel>(
                    GarchOptions{.variance_targeting = p.variance_targeting});
            }
        },
        spec.parameters);
}

std::size_t minimum_fit_length(const ModelSpec& spec) {
    return std::visit(
        [](const auto& p) -> std::size_t {
            using Parameters = std::decay_t<decltype(p)>;
            if constexpr (std::is_same_v<Parameters, RealisedSpec>) {
                return p.window;
            } else if constexpr (std::is_same_v<Parameters, EwmaSpec>) {
                return p.seed_window;
            } else {
                return kGarchMinimumFitLength;
            }
        },
        spec.parameters);
}

}
