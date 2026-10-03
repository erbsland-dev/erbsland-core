#pragma once

#include "WorkerExecutor_fwd.hpp"
#include "WorkerReservation.hpp"

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>

namespace erbsland::util::impl {

/// Bounded execution domain with admission reserved for an operation's complete lifetime.
/// Workers never join during destruction; accepted callbacks drain before idle workers exit.
/// Callbacks must handle operational failures; escaping exceptions terminate the process.
/// @tested{WorkerExecutorTest}
class WorkerExecutor final {
public:
    /// Scheduling classes sharing this domain, with separate concurrency accounting.
    enum class WorkClass {
        General, ///< Existing blocking operations.
        Security ///< Potentially blocking trust and native signing operations.
    };

public:
    /// Create an independently scheduled execution domain.
    /// @param maximumWorkers Positive worker ceiling.
    /// @param maximumOperations Positive admission ceiling, including idle retained reservations.
    /// @param maximumSecurityWorkers Positive security concurrency ceiling, at most maximumWorkers.
    /// @param idleTimeout Positive idle retirement interval.
    explicit WorkerExecutor(
        unsigned int maximumWorkers = 128U,
        std::size_t maximumOperations = 1024U,
        unsigned int maximumSecurityWorkers = 32U,
        std::chrono::milliseconds idleTimeout = std::chrono::seconds{30});
    /// Stop new admission without waiting for accepted native work.
    ~WorkerExecutor();

    // defaults/deletions
    WorkerExecutor(const WorkerExecutor &) = delete;
    WorkerExecutor(WorkerExecutor &&) = delete;
    auto operator=(const WorkerExecutor &) -> WorkerExecutor & = delete;
    auto operator=(WorkerExecutor &&) -> WorkerExecutor & = delete;

public: // operations
    /// Reserve bounded capacity before accepting an operation and its native resources.
    /// @param workClass Scheduling and concurrency class.
    /// @return Admission retained until the last reservation and its callbacks are released.
    /// @throws WorkerAdmissionError If the admission limit is reached or the executor is stopping.
    [[nodiscard]] auto reserve(WorkClass workClass = WorkClass::General) -> WorkerReservation;
    /// Admit and execute one callback without retaining admission in the caller.
    /// @param work Callback that handles its own operational failures.
    /// @param workClass Scheduling and concurrency class.
    /// @throws WorkerAdmissionError If the admission limit is reached or the executor is stopping.
    void submit(std::function<void()> work, WorkClass workClass = WorkClass::General);

private:
    std::shared_ptr<WorkerExecutorState> _state; ///< Also retained by detached workers and admitted operations.
};

}
