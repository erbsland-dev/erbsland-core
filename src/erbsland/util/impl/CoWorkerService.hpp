// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>

namespace erbsland::util::impl {

/// Process-wide worker service for bounded coroutine work.
///
/// The service is intentionally independent from the stream native-I/O service. Coroutine work may wait for a
/// complete bounded stream operation and must therefore never occupy workers that are required to fill or drain the
/// stream buffers.
/// @tested{CoTaskTest}
class CoWorkerService final {
public:
    /// Maximum number of coroutine workers that may exist at once.
    static constexpr auto cMaximumWorkerCount = 128U;

public:
    /// Submit work for asynchronous execution.
    /// @param work The work callback.
    static void submit(std::function<void()> work);

    CoWorkerService(const CoWorkerService &) = delete;
    CoWorkerService(CoWorkerService &&) = delete;
    auto operator=(const CoWorkerService &) -> CoWorkerService & = delete;
    auto operator=(CoWorkerService &&) -> CoWorkerService & = delete;

private:
    CoWorkerService();
    [[nodiscard]] static auto service() -> CoWorkerService &;
    void submitWork(std::function<void()> work);
    void addWorker();
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
