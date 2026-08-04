// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>

namespace erbsland::network::impl {

/// Process-wide bounded worker service for blocking host resolution.
/// @tested{HostLookupTest}
class ResolverService final {
public:
    /// Maximum number of resolver workers.
    static constexpr auto cMaximumWorkerCount = 128U;

public:
    /// Submit blocking resolver work.
    /// @param work The work callback.
    /// @warning The callback must handle every expected exception before returning.
    static void submit(std::function<void()> work);

    // defaults/deletions
    ResolverService(const ResolverService &) = delete;
    ResolverService(ResolverService &&) = delete;
    auto operator=(const ResolverService &) -> ResolverService & = delete;
    auto operator=(ResolverService &&) -> ResolverService & = delete;

private:
    /// Create the process-wide resolver service.
    ResolverService();
    /// Access the process-wide resolver service.
    [[nodiscard]] static auto service() -> ResolverService &;
    /// Queue resolver work for an available worker.
    void submitWork(std::function<void()> work);
    /// Start one worker thread.
    void addWorker();
    /// Execute queued resolver work in a worker thread.
    void run();

private:
    std::mutex _mutex;                        ///< Protects the worker state.
    std::condition_variable _condition;       ///< Signals queued work.
    std::deque<std::function<void()>> _queue; ///< Queued resolver work.
    unsigned int _minimumWorkerCount{0U};     ///< Workers retained while idle.
    unsigned int _workerCount{0U};            ///< Current worker count.
    unsigned int _idleWorkerCount{0U};        ///< Currently idle workers.
};

}
