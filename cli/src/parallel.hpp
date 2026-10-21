#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <exception>
#include <mutex>
#include <thread>
#include <vector>

namespace zpew::cli {

template <typename Task>
void run_parallel(std::size_t count, std::size_t threads, const Task& task) {
    std::atomic<std::size_t> next{0};
    std::exception_ptr failure;
    std::mutex failure_mutex;
    const auto worker = [&] {
        for (std::size_t i = next++; i < count; i = next++) {
            try {
                task(i);
            } catch (...) {
                const std::scoped_lock lock{failure_mutex};
                if (!failure) {
                    failure = std::current_exception();
                }
            }
        }
    };
    {
        std::vector<std::jthread> workers;
        const std::size_t worker_count =
            std::clamp<std::size_t>(threads, 1, std::max<std::size_t>(count, 1));
        workers.reserve(worker_count);
        for (std::size_t i = 0; i < worker_count; ++i) {
            workers.emplace_back(worker);
        }
    }
    if (failure) {
        std::rethrow_exception(failure);
    }
}

[[nodiscard]] inline std::size_t default_thread_count() {
    const unsigned available = std::thread::hardware_concurrency();
    return available > 1 ? available - 1 : 1;
}

}
