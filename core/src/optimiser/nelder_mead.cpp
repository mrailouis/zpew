#include <zpew/error.hpp>
#include <zpew/optimiser/nelder_mead.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <limits>
#include <span>
#include <utility>
#include <vector>

namespace zpew::optimiser {

namespace {

constexpr double kReflection = 1.0;
constexpr double kExpansion = 2.0;
constexpr double kContraction = 0.5;
constexpr double kShrink = 0.5;

using Point = std::vector<double>;

struct Vertex {
    Point x;
    double f;
};

class Simplex {
public:
    Simplex(const Objective& objective, std::span<const double> x0, double step)
        : objective_{&objective} {
        vertices_.push_back(make_vertex(Point(x0.begin(), x0.end())));
        for (std::size_t i = 0; i < x0.size(); ++i) {
            Point x(x0.begin(), x0.end());
            x[i] += step;
            vertices_.push_back(make_vertex(std::move(x)));
        }
        sort();
    }

    [[nodiscard]] Vertex make_vertex(Point x) const {
        const double f = (*objective_)(x);
        return {.x = std::move(x),
                .f = std::isfinite(f) ? f : std::numeric_limits<double>::infinity()};
    }

    void sort() { std::ranges::stable_sort(vertices_, {}, &Vertex::f); }

    [[nodiscard]] const Vertex& best() const { return vertices_.front(); }
    [[nodiscard]] const Vertex& worst() const { return vertices_.back(); }
    [[nodiscard]] const Vertex& second_worst() const { return vertices_[vertices_.size() - 2]; }
    void replace_worst(Vertex vertex) { vertices_.back() = std::move(vertex); }

    [[nodiscard]] bool converged(const NelderMeadOptions& options) const {
        double max_distance = 0.0;
        for (const Vertex& vertex : vertices_) {
            for (std::size_t i = 0; i < vertex.x.size(); ++i) {
                max_distance = std::max(max_distance, std::abs(vertex.x[i] - best().x[i]));
            }
        }
        const double spread = worst().f - best().f;
        const double scale = std::max(1.0, std::abs(best().f));
        return std::isfinite(best().f) && max_distance <= options.x_tolerance &&
               spread <= options.f_tolerance * scale;
    }

    [[nodiscard]] Point centroid_without_worst() const {
        Point centroid(best().x.size(), 0.0);
        for (std::size_t v = 0; v + 1 < vertices_.size(); ++v) {
            for (std::size_t i = 0; i < centroid.size(); ++i) {
                centroid[i] += vertices_[v].x[i];
            }
        }
        const auto count = static_cast<double>(vertices_.size() - 1);
        for (double& c : centroid) {
            c /= count;
        }
        return centroid;
    }

    void shrink() {
        const Point anchor = best().x;
        for (std::size_t v = 1; v < vertices_.size(); ++v) {
            Point x = vertices_[v].x;
            for (std::size_t i = 0; i < x.size(); ++i) {
                x[i] = anchor[i] + (kShrink * (x[i] - anchor[i]));
            }
            vertices_[v] = make_vertex(std::move(x));
        }
    }

private:
    const Objective* objective_;
    std::vector<Vertex> vertices_;
};

Point along(const Point& from, const Point& to, double coefficient) {
    Point result(from.size());
    for (std::size_t i = 0; i < from.size(); ++i) {
        result[i] = from[i] + (coefficient * (to[i] - from[i]));
    }
    return result;
}

void validate(std::span<const double> x0, const NelderMeadOptions& options) {
    if (x0.empty()) {
        throw InvalidArgument{"Nelder-Mead needs at least one parameter"};
    }
    if (!std::ranges::all_of(x0, [](double v) { return std::isfinite(v); })) {
        throw InvalidArgument{"Nelder-Mead starting point must be finite"};
    }
    const bool positive = options.initial_step > 0.0 && options.x_tolerance > 0.0 &&
                          options.f_tolerance > 0.0 && options.max_iterations > 0;
    if (!positive) {
        throw InvalidArgument{std::format(
            "Nelder-Mead options must be positive: step {}, x tolerance {}, f tolerance {}, "
            "max iterations {}",
            options.initial_step, options.x_tolerance, options.f_tolerance,
            options.max_iterations)};
    }
}

void step(Simplex& simplex) {
    const Point centroid = simplex.centroid_without_worst();
    const Vertex reflected = simplex.make_vertex(along(centroid, simplex.worst().x, -kReflection));

    if (reflected.f < simplex.best().f) {
        const Vertex expanded = simplex.make_vertex(along(centroid, reflected.x, kExpansion));
        simplex.replace_worst(expanded.f < reflected.f ? expanded : reflected);
        return;
    }
    if (reflected.f < simplex.second_worst().f) {
        simplex.replace_worst(reflected);
        return;
    }
    if (reflected.f < simplex.worst().f) {
        Vertex outside = simplex.make_vertex(along(centroid, reflected.x, kContraction));
        if (outside.f <= reflected.f) {
            simplex.replace_worst(std::move(outside));
            return;
        }
    } else {
        Vertex inside = simplex.make_vertex(along(centroid, simplex.worst().x, kContraction));
        if (inside.f < simplex.worst().f) {
            simplex.replace_worst(std::move(inside));
            return;
        }
    }
    simplex.shrink();
}

}

NelderMeadResult nelder_mead(const Objective& objective, std::span<const double> x0,
                             const NelderMeadOptions& options) {
    validate(x0, options);
    Simplex simplex{objective, x0, options.initial_step};
    int iterations = 0;
    bool converged = simplex.converged(options);
    while (!converged && iterations < options.max_iterations) {
        step(simplex);
        simplex.sort();
        ++iterations;
        converged = simplex.converged(options);
    }
    return {.x = simplex.best().x,
            .f = simplex.best().f,
            .iterations = iterations,
            .converged = converged};
}

}
