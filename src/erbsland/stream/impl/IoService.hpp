// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>

namespace erbsland::stream::impl {

/// Process-wide worker service for synchronous native I/O operations.
/// The worker count is bounded; excess work stays queued until a worker becomes available. The service has process
/// lifetime so shutdown can never wait for a deadlocked platform call.
/// @tested{BufferedStreamTest}
class IoService final {
public:
    /// Maximum number of native I/O workers that may exist at once.
    static constexpr auto cMaximumWorkerCount = 128U;

public:
    /// Access the process-wide I/O service.
    /// @return The service instance with process lifetime.
    [[nodiscard]] static auto service() -> IoService &;
    /// Submit synchronous native I/O work for background execution.
    /// @param work The work callback to execute on a service worker.
    static void submitIoWork(std::function<void()> work);

    // defaults/deletions
    IoService(const IoService &) = delete;
    IoService(IoService &&) = delete;
    auto operator=(const IoService &) -> IoService & = delete;
    auto operator=(IoService &&) -> IoService & = delete;

private:
    /// Create the process-wide I/O service.
    IoService();
    /// Submit work for asynchronous execution.
    void submit(std::function<void()> work);
    /// Add a worker that processes queued work.
    void addWorker();
    /// Run queued work on the current thread.
    void run();

private:
    std::mutex _mutex;
    std::condition_variable _condition;
    std::deque<std::function<void()>> _queue;
    unsigned int _minimumWorkerCount{0U};
    unsigned int _workerCount{0U};
    unsigned int _idleWorkerCount{0U};
};

}
