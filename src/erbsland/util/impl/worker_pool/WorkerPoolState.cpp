// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WorkerPoolState.hpp"

#include "../../../err/LogicFailure.hpp"
#include "../../../err/RuntimeError.hpp"
#include "../../../event/EventLoop.hpp"
#include "../../../text/Literals.hpp"
#include "../../WorkerCancelledError.hpp"
#include "../../WorkerPoolClosedError.hpp"
#include "../../WorkerPoolFullError.hpp"

#include <algorithm>
#include <exception>
#include <utility>

namespace erbsland::util::impl {

using namespace text::literals;
thread_local WorkerPoolState *WorkerPoolState::_currentPool = nullptr;

WorkerPoolState::WorkerPoolState(const WorkerPoolOptions &options) :
    _workerCount{resolveWorkerCount(options.workerCount())}, _maximumPending{options.maximumPendingTasks()} {
}

WorkerPoolState::~WorkerPoolState() {
    shutdown();
}

auto WorkerPoolState::resolveWorkerCount(const unsigned int count) noexcept -> unsigned int {
    if (count == 0) {
        return std::max(1U, std::thread::hardware_concurrency());
    }
    return count;
}

void WorkerPoolState::start() {
    _workers.reserve(_workerCount);
    try {
        for (auto index = 0U; index < _workerCount; ++index) {
            const auto lock = std::scoped_lock{_mutex};
            _workers.emplace_back([this]() -> void { run(); });
            ++_liveWorkers;
        }
    } catch (...) {
        quit();
        join();
        throw;
    }
}

void WorkerPoolState::verifyExternalThread() const {
    if (_currentPool == this) {
        throw err::LogicFailure{"A worker cannot wait for or join its own pool."_el};
    }
}

void WorkerPoolState::quit() noexcept {
    {
        const auto lock = std::scoped_lock{_mutex};
        _closed = true;
    }
    _condition.notify_all();
    kick();
}

void WorkerPoolState::stop() noexcept {
    const auto cancellation = std::make_exception_ptr(WorkerCancelledError{"Worker operation cancelled."_el});
    {
        const auto lock = std::scoped_lock{_mutex};
        _closed = true;
        _stopSource.requestStop();
        for (const auto &task : _tasks) {
            if (!task->error) {
                task->error = cancellation;
                task->cancelled = true;
            }
        }
    }
    _condition.notify_all();
    kick();
}

void WorkerPoolState::wait() {
    verifyExternalThread();
    auto lock = std::unique_lock{_mutex};
    _condition.wait(lock, [this]() -> bool { return _working == 0; });
}

void WorkerPoolState::join() {
    verifyExternalThread();
    const auto lock = std::scoped_lock{_joinMutex};
    for (auto &worker : _workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void WorkerPoolState::shutdown() noexcept {
    if (_currentPool == this) {
        std::terminate();
    }
    discardEvents();
    quit();
    join();
}

auto WorkerPoolState::isAccepting() const noexcept -> bool {
    const auto lock = std::scoped_lock{_mutex};
    return !_closed;
}

void WorkerPoolState::enqueue(WorkerTaskPtr task) {
    if (task->eventDelivery) {
        const auto owner = _owner.lock();
        if (owner == nullptr || owner->isQuitRequested()) {
            discardEvents();
            throw WorkerPoolClosedError{"The worker pool's owner loop has quit."_el};
        }
    }
    {
        const auto lock = std::scoped_lock{_mutex};
        if (_closed || (task->eventDelivery && _eventsDiscarded)) {
            throw WorkerPoolClosedError{"The worker pool is closed."_el};
        }
        if (_maximumPending != 0 && _pending >= _maximumPending) {
            throw WorkerPoolFullError{"The worker pool's pending-operation limit was reached."_el};
        }
        _tasks.insert(task);
        _queue.push_back(std::move(task));
        ++_working;
        ++_pending;
        _hadWork = true;
        _idleNotified = false;
    }
    _condition.notify_all();
}

void WorkerPoolState::run() {
    _currentPool = this;
    while (true) {
        auto task = WorkerTaskPtr{};
        auto index = std::size_t{0};
        auto execute = false;
        {
            auto lock = std::unique_lock{_mutex};
            _condition.wait(lock, [this]() -> bool { return !_queue.empty() || (_closed && _working == 0); });
            if (_queue.empty()) {
                --_liveWorkers;
                break;
            }
            task = std::move(_queue.front());
            _queue.pop_front();
            if (task->finished) {
                continue;
            }
            if (!task->error && task->next < task->itemCount()) {
                index = task->next++;
                ++task->running;
                execute = true;
                if (task->next < task->itemCount()) {
                    _queue.push_back(task);
                }
            } else if (task->running != 0) {
                continue;
            }
        }
        auto error = std::exception_ptr{};
        if (execute) {
            try {
                task->execute(index, _stopSource.stopToken());
            } catch (const err::RuntimeError &) {
                error = std::current_exception();
            }
        }
        auto finished = false;
        {
            const auto lock = std::scoped_lock{_mutex};
            if (execute) {
                --task->running;
            }
            if (error && !task->error) {
                task->error = error;
            }
            if (task->running == 0 && (task->error || task->next == task->itemCount())) {
                task->finished = true;
                _tasks.erase(task);
                finished = true;
            }
        }
        if (finished) {
            finish(std::move(task));
        }
    }
    kick();
    _currentPool = nullptr;
}

void WorkerPoolState::finish(WorkerTaskPtr task) {
    task->releaseWork();
    const auto eventDelivery = task->eventDelivery;
    auto delivery = WorkerDelivery{};
    if (eventDelivery) {
        delivery = WorkerDelivery{task->publish(task->error), std::move(task->failed), task->error, task->cancelled};
    } else {
        // Keep admission synchronized with future readiness: a consumer may immediately submit again.
        const auto lock = std::scoped_lock{_mutex};
        task->publish(task->error);
        --_pending;
    }
    task.reset();
    {
        const auto lock = std::scoped_lock{_mutex};
        if (eventDelivery) {
            if (!_eventsDiscarded) {
                _deliveries.push_back(std::move(delivery));
            } else {
                --_pending;
            }
        }
        --_working;
    }
    _condition.notify_all();
    kick();
}

}
