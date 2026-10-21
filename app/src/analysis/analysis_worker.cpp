#include "analysis/analysis_worker.hpp"

#include "analysis/analysis_settings.hpp"
#include "analysis/asset_analysis.hpp"

#include <zpew/data/snapshot.hpp>
#include <zpew/error.hpp>

#include <cstdint>
#include <exception>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <utility>

namespace zpew::app {

AnalysisWorker::AnalysisWorker() : thread_{[this](const std::stop_token& stop) { run(stop); }} {}

AnalysisWorker::~AnalysisWorker() {
    thread_.request_stop();
    wake_.notify_all();
}

std::uint64_t AnalysisWorker::request(data::Snapshot snapshot, AnalysisSettings settings) {
    std::uint64_t id = 0;
    {
        const std::scoped_lock lock{mutex_};
        id = ++next_id_;
        pending_ = Request{.id = id, .snapshot = std::move(snapshot), .settings = settings};
    }
    wake_.notify_one();
    return id;
}

std::optional<AnalysisOutcome> AnalysisWorker::take_outcome() {
    const std::scoped_lock lock{mutex_};
    return std::exchange(outcome_, std::nullopt);
}

bool AnalysisWorker::busy() const {
    const std::scoped_lock lock{mutex_};
    return running_ || pending_.has_value();
}

void AnalysisWorker::run(const std::stop_token& stop) {
    while (true) {
        std::optional<Request> taken;
        {
            std::unique_lock lock{mutex_};
            if (!wake_.wait(lock, stop, [this] { return pending_.has_value(); })) {
                return;
            }
            taken = std::exchange(pending_, std::nullopt);
            running_ = true;
        }
        if (!taken) {
            continue;
        }
        const Request& request = *taken;
        AnalysisOutcome outcome{.request_id = request.id, .value = std::string{}};
        try {
            outcome.value = analyse_asset(request.snapshot, request.settings);
        } catch (const Error& error) {
            outcome.value = std::string{error.what()};
        } catch (const std::exception& error) {
            outcome.value = std::string{"unexpected error: "} + error.what();
        }
        const std::scoped_lock lock{mutex_};
        outcome_ = std::move(outcome);
        running_ = false;
    }
}

}
