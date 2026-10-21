#include <zpew/data/bar.hpp>
#include <zpew/evaluation/proxies.hpp>

#include <cmath>
#include <numbers>
#include <span>
#include <vector>

namespace zpew::evaluation {

std::vector<double> squared_return_proxy(std::span<const double> returns) {
    std::vector<double> proxy;
    proxy.reserve(returns.size());
    for (const double r : returns) {
        proxy.push_back(r * r);
    }
    return proxy;
}

std::vector<double> parkinson_proxy(std::span<const data::Bar> bars) {
    const double denominator = 4.0 * std::numbers::ln2;
    std::vector<double> proxy;
    if (bars.size() < 2) {
        return proxy;
    }
    proxy.reserve(bars.size() - 1);
    for (const data::Bar& bar : bars.subspan(1)) {
        const double range = std::log(bar.high / bar.low);
        proxy.push_back((range * range) / denominator);
    }
    return proxy;
}

}
