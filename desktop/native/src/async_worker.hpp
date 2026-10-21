#pragma once

#include <memory>
#include <napi.h>

namespace zpew::native {

template <typename Worker>
[[nodiscard]] Napi::Promise queue_worker(std::unique_ptr<Worker> worker) {
    const Napi::Promise promise = worker->promise();
    worker->Queue();
    static_cast<void>(worker.release());
    return promise;
}

}
