#include <zpew/data/bar.hpp>
#include <zpew/error.hpp>
#include <zpew/returns/log_returns.hpp>

#include <cmath>
#include <cstddef>
#include <format>
#include <span>
#include <vector>

namespace zpew::returns {

namespace {

void require_valid_close(double close, std::size_t index) {
    if (!std::isfinite(close) || close <= 0.0) {
        throw InvalidArgument{
            std::format("close {} at index {} must be finite and positive", close, index)};
    }
}

}

std::vector<double> log_returns(std::span<const double> closes) {
    for (std::size_t i = 0; i < closes.size(); ++i) {
        require_valid_close(closes[i], i);
    }
    std::vector<double> returns;
    if (closes.size() < 2) {
        return returns;
    }
    returns.reserve(closes.size() - 1);
    for (std::size_t i = 1; i < closes.size(); ++i) {
        returns.push_back(std::log(closes[i] / closes[i - 1]));
    }
    return returns;
}

std::vector<double> log_returns(std::span<const data::Bar> bars) {
    std::vector<double> closes;
    closes.reserve(bars.size());
    for (const data::Bar& bar : bars) {
        closes.push_back(bar.close);
    }
    return log_returns(std::span<const double>{closes});
}

}
