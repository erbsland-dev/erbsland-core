#pragma once

#include "WorkerExecutorState_fwd.hpp"

#include <cstddef>
#include <memory>

namespace erbsland::util::impl {

/// Lifetime admission and serialization state for one accepted operation.
/// @tested{WorkerExecutorTest}
struct WorkerOperation final {
    /// Release admission after the last reservation and callback relinquish ownership.
    ~WorkerOperation();

    std::shared_ptr<WorkerExecutorState> state; ///< Owning execution domain.
    std::size_t workClass{};                    ///< Fairness and concurrency class index.
    std::size_t queued{};                       ///< Continuation and cleanup callbacks awaiting execution.
    bool running{};                             ///< Whether this operation currently occupies a worker.
};

}
