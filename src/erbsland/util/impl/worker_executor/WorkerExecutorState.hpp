#pragma once

#include "WorkerAdmissionError.hpp"
#include "WorkerExecutorState_fwd.hpp"
#include "WorkerOperation.hpp"

#include "../../../text/Literals.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <system_error>
#include <thread>
#include <utility>

namespace erbsland::util::impl {

/// Shared scheduling state retained independently of the executor facade.
/// @tested{WorkerExecutorTest}
class WorkerExecutorState final : public std::enable_shared_from_this<WorkerExecutorState> {
public:
    /// One queued callback with retained admission.
    struct Work final {
        std::shared_ptr<WorkerOperation> operation;
        std::function<void()> callback;
    };

    /// Start a worker while holding the state mutex.
    void addWorker() {
        using namespace text::literals;
        if (workerCount >= maximumWorkers) {
            return;
        }
        ++workerCount;
        ++idleWorkers;
        try {
            std::thread{[self = shared_from_this()]() -> void { self->run(); }}.detach();
        } catch (const std::system_error &) {
            --workerCount;
            --idleWorkers;
            if (workerCount == 0U) {
                throw WorkerAdmissionError{"The background executor could not start a worker."_el};
            }
        }
    }

    /// Find the next eligible workload without modifying fairness state.
    [[nodiscard]] auto runnable() -> std::deque<Work>::iterator {
        for (auto offset = std::size_t{0}; offset < 2U; ++offset) {
            const auto workClass = (nextClass + offset) % 2U;
            if (active[workClass] >= concurrency[workClass]) {
                continue;
            }
            const auto found = std::find_if(queue.begin(), queue.end(), [workClass](const Work &work) -> bool {
                return work.operation->workClass == workClass && !work.operation->running;
            });
            if (found != queue.end()) {
                return found;
            }
        }
        return queue.end();
    }

    /// Drain accepted callbacks until shutdown or idle retirement.
    void run() {
        while (true) {
            auto work = Work{};
            {
                auto lock = std::unique_lock{mutex};
                auto found = runnable();
                while (found == queue.end()) {
                    if (stopping && queue.empty()) {
                        --workerCount;
                        --idleWorkers;
                        return;
                    }
                    if (condition.wait_for(lock, idleTimeout) == std::cv_status::timeout &&
                        workerCount > minimumWorkers) {
                        --workerCount;
                        --idleWorkers;
                        return;
                    }
                    found = runnable();
                }
                --idleWorkers;
                work = std::move(*found);
                queue.erase(found);
                nextClass = (work.operation->workClass + 1U) % 2U;
                --work.operation->queued;
                work.operation->running = true;
                ++active[work.operation->workClass];
                if (idleWorkers == 0U && runnable() != queue.end()) {
                    addWorker();
                }
            }
            work.callback();
            work.callback = {};
            auto finishShutdown = false;
            {
                const auto lock = std::scoped_lock{mutex};
                work.operation->running = false;
                --active[work.operation->workClass];
                ++idleWorkers;
                finishShutdown = stopping && queue.empty();
            }
            work.operation.reset();
            if (finishShutdown) {
                // Sleepers may have observed shutdown while serialized callbacks were still queued.
                condition.notify_all();
            } else {
                // Completion releases one operation and one execution slot.
                condition.notify_one();
            }
        }
    }

    std::mutex mutex;                          ///< Synchronization for admission and scheduling.
    std::condition_variable condition;         ///< Worker wake-up signal.
    std::deque<Work> queue;                    ///< Bounded admitted callbacks.
    std::array<unsigned int, 2> concurrency{}; ///< Per-class execution ceilings.
    std::array<unsigned int, 2> active{};      ///< Currently running work by class.
    std::chrono::milliseconds idleTimeout{};   ///< Idle worker retirement interval.
    std::size_t maximumOperations{};           ///< Total admission ceiling.
    std::size_t operationCount{};              ///< Retained admission count.
    std::array<std::size_t, 2> admitted{};     ///< Retained admission by class.
    std::size_t nextClass{};                   ///< Next fair scheduling preference.
    unsigned int maximumWorkers{};             ///< Worker ceiling.
    unsigned int minimumWorkers{};             ///< Idle worker floor.
    unsigned int workerCount{};                ///< Live detached workers.
    unsigned int idleWorkers{};                ///< Waiting workers.
    bool stopping{};                           ///< Reject new operations while accepted work drains.
};

}
