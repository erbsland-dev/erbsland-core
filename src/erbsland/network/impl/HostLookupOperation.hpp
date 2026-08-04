// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HostLookupOperation_fwd.hpp"

#include "../Host.hpp"
#include "../host_lookup/HostLookupOptions.hpp"

#include "../../time/TimePoint.hpp"

#include <atomic>
#include <cstddef>

namespace erbsland::network::impl {

/// State retained for one asynchronous host lookup operation.
/// @tested{HostLookupTest}
struct HostLookupOperation final {
    /// Create the record for one lookup operation.
    /// @param operationHost The host being resolved.
    /// @param operationOptions The options captured for this operation.
    HostLookupOperation(Host operationHost, HostLookupOptions operationOptions);

    Host host;                                  ///< Host being resolved.
    HostLookupOptions options;                  ///< Options captured for this operation.
    time::TimePoint deadline;                   ///< Complete-operation deadline.
    std::atomic<std::size_t> attemptCount{0U};  ///< Number of submitted native attempts.
    std::atomic<bool> completionClaimed{false}; ///< Whether a terminal path won the operation.
};

}
