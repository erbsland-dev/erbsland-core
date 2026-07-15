// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CoWorkerService.hpp"

#include <algorithm>
#include <chrono>
#include <exception>
#include <thread>
#include <utility>

namespace erbsland::util::impl {

CoWorkerService::CoWorkerService() {
    _minimumWorkerCount = std::min(cMaximumWorkerCount, std::max(4U, std::thread::hardware_concurrency()));
    for (auto i = 0U; i < _minimumWorkerCount; ++i) {
        addWorker();
    }
}

auto CoWorkerService::service() -> CoWorkerService & {
    static auto *instance = new CoWorkerService{}; // Intentionally process-lifetime; workers are never joined.
    return *instance;
}

void CoWorkerService::submit(std::function<void()> work) {
    service().submitWork(std::move(work));
}

void CoWorkerService::submitWork(std::function<void()> work) {
    {
        const auto lock = std::scoped_lock{_mutex};
        _queue.push_back(std::move(work));
        if (_idleWorkerCount == 0U) {
            addWorker();
        }
    }
    _condition.notify_one();
}

void CoWorkerService::addWorker() {
    if (_workerCount >= cMaximumWorkerCount) {
        return;
    }
    ++_workerCount;
    ++_idleWorkerCount;
    std::thread{[this]() -> void { run(); }}.detach();
}

void CoWorkerService::run() {
    while (true) {
        auto work = std::function<void()>{};
        {
            auto lock = std::unique_lock{_mutex};
            while (_queue.empty()) {
                constexpr auto cIdleTimeout = std::chrono::seconds{30};
                if (_condition.wait_for(lock, cIdleTimeout) == std::cv_status::timeout &&
                    _workerCount > _minimumWorkerCount) {
                    --_workerCount;
                    --_idleWorkerCount;
                    return;
                }
            }
            --_idleWorkerCount;
            work = std::move(_queue.front());
            _queue.pop_front();
            if (!_queue.empty() && _idleWorkerCount == 0U) {
                addWorker();
            }
        }
        try {
            work();
        } catch (...) {}
        {
            const auto lock = std::scoped_lock{_mutex};
            ++_idleWorkerCount;
        }
        _condition.notify_one();
    }
}

}
