// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HostLookupOperation.hpp"

#include "../../../util/impl/worker_executor/WorkerServices.hpp"

#include <utility>

namespace erbsland::network::impl {

HostLookupOperation::HostLookupOperation(
    Host operationHost, HostLookupOptions operationOptions, const event::EventsPtr &ownerEvents) :
    resultCompletion{event::impl::EventContinuation::create(ownerEvents)},
    cancellationCompletion{event::impl::EventContinuation::create(ownerEvents)},
    deadlineCompletion{event::impl::EventContinuation::create(ownerEvents)},
    host{std::move(operationHost)},
    options{operationOptions},
    deadline{time::TimePoint::inFuture(operationOptions.timeout())} {
    if (host.name().has_value()) {
        work = util::impl::worker_services::blocking().reserve();
    }
}

}
