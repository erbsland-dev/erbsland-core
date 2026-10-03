#pragma once

#include "WorkerExecutor_fwd.hpp"
#include "WorkerOperation.hpp"

#include <functional>
#include <memory>

namespace erbsland::util::impl {

/// An admitted operation retaining capacity for serial work and terminal cleanup.
/// At most two callbacks may wait for this operation; one additional callback may be running.
/// @tested{WorkerExecutorTest}
class WorkerReservation final {
    friend class WorkerExecutor;

public:
    /// Construct an empty reservation.
    WorkerReservation() = default;

public: // operations
    /// Queue work without another admission check; callbacks for this reservation execute serially.
    /// @param work Callback that handles its own operational failures.
    void submit(std::function<void()> work) const;
    /// Test whether this reservation owns admission capacity.
    [[nodiscard]] auto isValid() const noexcept -> bool;

private:
    /// Bind an operation admitted by its executor.
    explicit WorkerReservation(std::shared_ptr<WorkerOperation> operation);

private:
    std::shared_ptr<WorkerOperation> _operation; ///< Shared admission and serialization state.
};

}
