// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HostLookupOperation_fwd.hpp"

#include "../../../event/impl/EventContinuation.hpp"
#include "../../../time/TimePoint.hpp"
#include "../../../util/impl/worker_executor/WorkerExecutor.hpp"
#include "../../Host.hpp"
#include "../../host_lookup/HostLookupOptions.hpp"

#include <atomic>
#include <cstddef>

namespace erbsland::network::impl {

/// State retained for one asynchronous host lookup operation.
/// @tested{HostLookupTest}
struct HostLookupOperation final {
    /// Create the record for one lookup operation.
    /// @param operationHost The host being resolved.
    /// @param operationOptions The options captured for this operation.
    /// @param ownerEvents Owner event loop whose completion capacity is reserved before admission.
    HostLookupOperation(Host operationHost, HostLookupOptions operationOptions, const event::EventsPtr &ownerEvents);

    event::impl::EventContinuationPtr resultCompletion;       ///< Serialized result and retry capacity.
    event::impl::EventContinuationPtr cancellationCompletion; ///< Independent cancellation capacity.
    event::impl::EventContinuationPtr deadlineCompletion;     ///< Independently scheduled deadline capacity.
    util::impl::WorkerReservation work;                       ///< Admission retained across retries.
    Host host;                                                ///< Host being resolved.
    HostLookupOptions options;                                ///< Options captured for this operation.
    time::TimePoint deadline;                                 ///< Complete-operation deadline.
    std::atomic<std::size_t> attemptCount{0U};                ///< Number of submitted native attempts.
    std::atomic<bool> completionClaimed{false};               ///< Whether a terminal path won the operation.
};

}
