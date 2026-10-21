#include <zpew/error.hpp>
#include <zpew/var/normal_quantile.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <format>

namespace zpew::var {

namespace {

using Coefficients = std::array<double, 8>;

constexpr double kCentralSplit = 0.425;
constexpr double kTailSplit = 5.0;
constexpr double kCentralOffset = 0.180625;
constexpr double kTailOffset = 1.6;

constexpr Coefficients kCentralNumerator{
    3.387132872796366608,  133.14166789178437745, 1971.5909503065514427, 13731.693765509461125,
    45921.953931549871457, 67265.770927008700853, 33430.575583588128105, 2509.0809287301226727};
constexpr Coefficients kCentralDenominator{1.0,
                                           42.313330701600911252,
                                           687.1870074920579083,
                                           5394.1960214247511077,
                                           21213.794301586595867,
                                           39307.89580009271061,
                                           28729.085735721942674,
                                           5226.495278852545925};
constexpr Coefficients kIntermediateNumerator{1.42343711074968357734,   4.6303378461565452959,
                                              5.7694972214606914055,    3.64784832476320460504,
                                              1.27045825245236838258,   0.24178072517745061177,
                                              0.0227238449892691845833, 7.7454501427834140764e-4};
constexpr Coefficients kIntermediateDenominator{1.0,
                                                2.05319162663775882187,
                                                1.6763848301838038494,
                                                0.68976733498510000455,
                                                0.14810397642748007459,
                                                0.0151986665636164571966,
                                                5.475938084995344946e-4,
                                                1.05075007164441684324e-9};
constexpr Coefficients kTailNumerator{6.6579046435011037772,     5.4637849111641143699,
                                      1.7848265399172913358,     0.29656057182850489123,
                                      0.026532189526576123093,   0.0012426609473880784386,
                                      2.71155556874348757815e-5, 2.01033439929228813265e-7};
constexpr Coefficients kTailDenominator{1.0,
                                        0.59983220655588793769,
                                        0.13692988092273580531,
                                        0.0148753612908506148525,
                                        7.868691311456132591e-4,
                                        1.8463183175100546818e-5,
                                        1.4215117583164458887e-7,
                                        2.04426310338993978564e-15};

double polynomial(const Coefficients& coefficients, double x) noexcept {
    double value = 0.0;
    for (std::size_t i = coefficients.size(); i-- > 0;) {
        value = (value * x) + coefficients[i];
    }
    return value;
}

double rational(const Coefficients& numerator, const Coefficients& denominator, double x) noexcept {
    return polynomial(numerator, x) / polynomial(denominator, x);
}

}

double normal_quantile(double probability) {
    const bool in_open_unit_interval = probability > 0.0 && probability < 1.0;
    if (!in_open_unit_interval) {
        throw InvalidArgument{
            std::format("normal quantile needs a probability in (0, 1), got {}", probability)};
    }
    const double q = probability - 0.5;
    if (std::abs(q) <= kCentralSplit) {
        const double r = kCentralOffset - (q * q);
        return q * rational(kCentralNumerator, kCentralDenominator, r);
    }
    const double tail = q < 0.0 ? probability : 1.0 - probability;
    double r = std::sqrt(-std::log(tail));
    double value = 0.0;
    if (r <= kTailSplit) {
        r -= kTailOffset;
        value = rational(kIntermediateNumerator, kIntermediateDenominator, r);
    } else {
        r -= kTailSplit;
        value = rational(kTailNumerator, kTailDenominator, r);
    }
    return q < 0.0 ? -value : value;
}

}
