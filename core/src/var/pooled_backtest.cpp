#include <zpew/evaluation/improvement.hpp>
#include <zpew/evaluation/rolling_forecast.hpp>
#include <zpew/var/pooled_backtest.hpp>

#include <cstddef>
#include <map>
#include <numeric>
#include <span>
#include <string>
#include <tuple>
#include <vector>

namespace zpew::var {

std::vector<PooledBacktest> pool_backtests(std::span<const AssetBacktest> backtests) {
    using Key = std::tuple<evaluation::WindowMode, std::string, double>;
    std::map<Key, std::vector<const AssetBacktest*>> groups;
    for (const AssetBacktest& item : backtests) {
        groups[{item.mode, item.model, item.result.confidence}].push_back(&item);
    }
    std::vector<PooledBacktest> pooled;
    for (const auto& [key, items] : groups) {
        const auto& [mode, model, confidence] = key;
        PooledBacktest row{.mode = mode, .model = model, .confidence = confidence};
        std::vector<double> rates;
        for (const AssetBacktest* item : items) {
            row.observations += item->result.observations;
            row.breaches += item->result.breaches;
            rates.push_back(item->result.breach_rate);
            if (item->result.kupiec.p_value < kKupiecSignificance) {
                ++row.kupiec_rejections;
            }
        }
        row.assets = items.size();
        row.pooled_breach_rate =
            static_cast<double>(row.breaches) / static_cast<double>(row.observations);
        row.mean_breach_rate =
            std::accumulate(rates.begin(), rates.end(), 0.0) / static_cast<double>(rates.size());
        row.median_breach_rate = evaluation::median(rates);
        pooled.push_back(row);
    }
    return pooled;
}

}
