#pragma once

#include <functional>
#include <span>
#include <vector>

namespace zpew::optimiser {

struct NelderMeadOptions {
    double initial_step = 0.1;
    double x_tolerance = 1e-8;
    double f_tolerance = 1e-12;
    int max_iterations = 5000;
};

struct NelderMeadResult {
    std::vector<double> x;
    double f{};
    int iterations{};
    bool converged{};
};

using Objective = std::function<double(std::span<const double>)>;

[[nodiscard]] NelderMeadResult nelder_mead(const Objective& objective, std::span<const double> x0,
                                           const NelderMeadOptions& options = {});

}
