// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ResolverService.hpp"

#include <algorithm>
#include <chrono>
#include <thread>
#include <utility>

namespace erbsland::network::impl {

ResolverService::ResolverService() {
    _minimumWorkerCount = std::min(cMaximumWorkerCount, std::max(4U, std::thread::hardware_concurrency()));
    for (auto index = 0U; index < _minimumWorkerCount; ++index) {
        addWorker();
    }
}

auto ResolverService::service() -> ResolverService & {
    static auto *instance = new ResolverService{}; // Process lifetime avoids waiting for blocked resolver calls.
    return *instance;
}

void ResolverService::submit(std::function<void()> work) {
    service().submitWork(std::move(work));
}

void ResolverService::submitWork(std::function<void()> work) {
    {
        const auto lock = std::scoped_lock{_mutex};
        _queue.push_back(std::move(work));
        if (_idleWorkerCount == 0U) {
            addWorker();
        }
    }
    _condition.notify_one();
}

void ResolverService::addWorker() {
    if (_workerCount >= cMaximumWorkerCount) {
        return;
    }
    ++_workerCount;
    ++_idleWorkerCount;
    std::thread{[this]() -> void { run(); }}.detach();
}

void ResolverService::run() {
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
        work();
        {
            const auto lock = std::scoped_lock{_mutex};
            ++_idleWorkerCount;
        }
        _condition.notify_one();
    }
}

}
