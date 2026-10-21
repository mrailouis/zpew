#pragma once

#include <zpew/models/garch_params.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace zpew::test {

inline std::vector<double> simulate_garch(const models::GarchParams& params, std::size_t count,
                                          std::uint64_t seed) {
    std::mt19937_64 rng{seed};
    std::normal_distribution<double> shock{0.0, 1.0};
    constexpr std::size_t kBurnIn = 1000;
    double variance = params.long_run_variance();
    std::vector<double> returns;
    returns.reserve(count);
    for (std::size_t t = 0; t < count + kBurnIn; ++t) {
        const double r = std::sqrt(variance) * shock(rng);
        if (t >= kBurnIn) {
            returns.push_back(r);
        }
        variance = models::garch_next_variance(params, variance, r);
    }
    return returns;
}

}
