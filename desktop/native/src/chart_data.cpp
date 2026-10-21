#include "chart_data.hpp"

#include "async_worker.hpp"
#include "js_error.hpp"
#include "request_settings.hpp"
#include "typed_arrays.hpp"

#include <zpew/data/bar.hpp>
#include <zpew/data/gap.hpp>
#include <zpew/data/snapshot.hpp>
#include <zpew/data/snapshot_data_source.hpp>
#include <zpew/data/trading_calendar.hpp>
#include <zpew/error.hpp>
#include <zpew/returns/log_returns.hpp>
#include <zpew/returns/moving_average.hpp>
#include <zpew/returns/realised_variance.hpp>

#include <cstddef>
#include <exception>
#include <filesystem>
#include <limits>
#include <memory>
#include <napi.h>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace zpew::native {

namespace {

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kPercent = 100.0;
constexpr double kDaysPerYearForDailyCalendar = 365.0;

std::vector<double> aligned_to_bars(const std::vector<double>& values, std::size_t first_bar,
                                    std::size_t bar_count) {
    std::vector<double> aligned(bar_count, kNaN);
    for (std::size_t i = 0; i < values.size() && first_bar + i < bar_count; ++i) {
        aligned[first_bar + i] = values[i];
    }
    return aligned;
}

class LoadChartWorker final : public Napi::AsyncWorker {
public:
    LoadChartWorker(Napi::Env env, std::string manifest_path, std::string symbol,
                    ChartSettings settings)
        : Napi::AsyncWorker{env}, deferred_{Napi::Promise::Deferred::New(env)},
          manifest_path_{std::move(manifest_path)}, symbol_{std::move(symbol)},
          settings_{settings} {}

    [[nodiscard]] Napi::Promise promise() const { return deferred_.Promise(); }

    void Execute() override {
        try {
            const data::SnapshotDataSource source{std::filesystem::path{manifest_path_}};
            result_ = build_chart_data(source.load_snapshot(symbol_), settings_);
        } catch (const std::exception& error) {
            SetError(error.what());
        }
    }

    void OnOK() override {
        const Napi::Env env = Env();
        Napi::Object result = Napi::Object::New(env);
        result.Set("symbol", result_.symbol);
        result.Set("name", result_.name);
        result.Set("calendar", result_.calendar);
        result.Set("times", to_float64_array(env, result_.times));
        result.Set("open", to_float64_array(env, result_.open));
        result.Set("high", to_float64_array(env, result_.high));
        result.Set("low", to_float64_array(env, result_.low));
        result.Set("close", to_float64_array(env, result_.close));
        result.Set("volume", to_float64_array(env, result_.volume));
        result.Set("fastAverage", to_float64_array(env, result_.fast_average));
        result.Set("slowAverage", to_float64_array(env, result_.slow_average));
        result.Set("volatilityPercent", to_float64_array(env, result_.volatility_percent));
        Napi::Array gaps = Napi::Array::New(env, result_.gaps.size());
        for (std::size_t i = 0; i < result_.gaps.size(); ++i) {
            Napi::Object gap = Napi::Object::New(env);
            gap.Set("lastBefore", result_.gaps[i].last_before);
            gap.Set("firstAfter", result_.gaps[i].first_after);
            gap.Set("missingSessions", result_.gaps[i].missing_sessions);
            gaps.Set(static_cast<std::uint32_t>(i), gap);
        }
        result.Set("gaps", gaps);
        deferred_.Resolve(result);
    }

    void OnError(const Napi::Error& error) override { deferred_.Reject(error.Value()); }

private:
    Napi::Promise::Deferred deferred_;
    std::string manifest_path_;
    std::string symbol_;
    ChartSettings settings_;
    ChartData result_;
};

}

ChartData build_chart_data(const data::Snapshot& snapshot, const ChartSettings& settings) {
    ChartData chart{.symbol = snapshot.asset.symbol,
                    .name = snapshot.asset.name,
                    .calendar = std::string{data::to_string(snapshot.asset.calendar)}};
    const std::size_t count = snapshot.bars.size();
    for (std::vector<double>* column :
         {&chart.times, &chart.open, &chart.high, &chart.low, &chart.close, &chart.volume}) {
        column->reserve(count);
    }
    for (const data::Bar& bar : snapshot.bars) {
        chart.times.push_back(seconds_since_epoch(bar.date));
        chart.open.push_back(bar.open);
        chart.high.push_back(bar.high);
        chart.low.push_back(bar.low);
        chart.close.push_back(bar.close);
        chart.volume.push_back(bar.volume);
    }
    chart.fast_average =
        aligned_to_bars(returns::simple_moving_average(chart.close, settings.fast_average),
                        settings.fast_average - 1, count);
    chart.slow_average =
        aligned_to_bars(returns::simple_moving_average(chart.close, settings.slow_average),
                        settings.slow_average - 1, count);

    const double periods_per_year = snapshot.asset.calendar == data::TradingCalendar::kDaily
                                        ? kDaysPerYearForDailyCalendar
                                        : returns::kTradingDaysPerYear;
    std::vector<double> volatility = returns::rolling_realised_variance(
        returns::log_returns(std::span<const data::Bar>{snapshot.bars}),
        settings.volatility_window);
    for (double& value : volatility) {
        value = kPercent * returns::annualise_volatility(value, periods_per_year);
    }
    chart.volatility_percent = aligned_to_bars(volatility, settings.volatility_window, count);

    for (const data::Gap& gap : snapshot.gaps) {
        chart.gaps.push_back({.last_before = seconds_since_epoch(gap.last_before),
                              .first_after = seconds_since_epoch(gap.first_after),
                              .missing_sessions = gap.missing_sessions});
    }
    return chart;
}

Napi::Value load_chart(const Napi::CallbackInfo& info) {
    const Napi::Env env = info.Env();
    std::string manifest_path = require_string(info, 0, "manifestPath");
    std::string symbol = require_string(info, 1, "symbol");
    const ChartSettings settings = parse_chart_settings(env, info[2]);
    return queue_worker(std::make_unique<LoadChartWorker>(env, std::move(manifest_path),
                                                          std::move(symbol), settings));
}

}
