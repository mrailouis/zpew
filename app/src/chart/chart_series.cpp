#include "chart/chart_series.hpp"

#include "chart/chart_settings.hpp"

#include <zpew/data/bar.hpp>
#include <zpew/data/snapshot.hpp>
#include <zpew/data/trading_calendar.hpp>
#include <zpew/returns/log_returns.hpp>
#include <zpew/returns/moving_average.hpp>
#include <zpew/returns/realised_variance.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iterator>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace zpew::app {

namespace {

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kDaysPerYearForDailyCalendar = 365.0;
constexpr double kPercent = 100.0;

std::vector<double> aligned_to_bars(const std::vector<double>& values, std::size_t first_bar,
                                    std::size_t bar_count) {
    std::vector<double> aligned(bar_count, kNaN);
    for (std::size_t i = 0; i < values.size() && first_bar + i < bar_count; ++i) {
        aligned[first_bar + i] = values[i];
    }
    return aligned;
}

double seconds_since_epoch(std::chrono::sys_days date) {
    return static_cast<double>(
        std::chrono::duration_cast<std::chrono::seconds>(date.time_since_epoch()).count());
}

}

std::optional<std::size_t> ChartSeries::nearest_index(double time) const {
    if (times.empty()) {
        return std::nullopt;
    }
    const auto upper = std::ranges::lower_bound(times, time);
    if (upper == times.begin()) {
        return 0;
    }
    if (upper == times.end()) {
        return times.size() - 1;
    }
    const auto lower = std::prev(upper);
    const auto nearest = (time - *lower) <= (*upper - time) ? lower : upper;
    return static_cast<std::size_t>(std::distance(times.begin(), nearest));
}

ChartSeries build_chart_series(const data::Snapshot& snapshot, const ChartSettings& settings) {
    ChartSeries series;
    series.symbol = snapshot.asset.symbol;
    series.name = snapshot.asset.name;
    const std::size_t count = snapshot.bars.size();
    for (std::vector<double>* column :
         {&series.times, &series.open, &series.high, &series.low, &series.close, &series.volume}) {
        column->reserve(count);
    }
    series.dates.reserve(count);
    for (const data::Bar& bar : snapshot.bars) {
        series.dates.push_back(bar.date);
        series.times.push_back(seconds_since_epoch(bar.date));
        series.open.push_back(bar.open);
        series.high.push_back(bar.high);
        series.low.push_back(bar.low);
        series.close.push_back(bar.close);
        series.volume.push_back(bar.volume);
    }

    const auto fast = static_cast<std::size_t>(settings.fast_average);
    const auto slow = static_cast<std::size_t>(settings.slow_average);
    series.fast_average =
        aligned_to_bars(returns::simple_moving_average(series.close, fast), fast - 1, count);
    series.slow_average =
        aligned_to_bars(returns::simple_moving_average(series.close, slow), slow - 1, count);

    const auto window = static_cast<std::size_t>(settings.volatility_window);
    const double periods_per_year = snapshot.asset.calendar == data::TradingCalendar::kDaily
                                        ? kDaysPerYearForDailyCalendar
                                        : returns::kTradingDaysPerYear;
    std::vector<double> volatility = returns::rolling_realised_variance(
        returns::log_returns(std::span<const data::Bar>{snapshot.bars}), window);
    std::ranges::transform(volatility, volatility.begin(), [&](double variance) {
        return kPercent * returns::annualise_volatility(variance, periods_per_year);
    });
    series.volatility_percent = aligned_to_bars(volatility, window, count);
    return series;
}

}
