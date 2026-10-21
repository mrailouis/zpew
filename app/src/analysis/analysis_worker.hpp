#pragma once

#include "analysis/analysis_settings.hpp"
#include "analysis/asset_analysis.hpp"

#include <zpew/data/snapshot.hpp>

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <thread>
#include <variant>

namespace zpew::app {

struct AnalysisOutcome {
    std::uint64_t request_id{};
    std::variant<AssetAnalysis, std::string> value;
};

class AnalysisWorker {
public:
    AnalysisWorker();
    ~AnalysisWorker();

    AnalysisWorker(const AnalysisWorker&) = delete;
    AnalysisWorker& operator=(const AnalysisWorker&) = delete;
    AnalysisWorker(AnalysisWorker&&) = delete;
    AnalysisWorker& operator=(AnalysisWorker&&) = delete;

    std::uint64_t request(data::Snapshot snapshot, AnalysisSettings settings);
    [[nodiscard]] std::optional<AnalysisOutcome> take_outcome();
    [[nodiscard]] bool busy() const;

private:
    struct Request {
        std::uint64_t id{};
        data::Snapshot snapshot;
        AnalysisSettings settings;
    };

    void run(const std::stop_token& stop);

    mutable std::mutex mutex_;
    std::condition_variable_any wake_;
    std::optional<Request> pending_;
    std::optional<AnalysisOutcome> outcome_;
    std::uint64_t next_id_ = 0;
    bool running_ = false;
    std::jthread thread_;
};

}
