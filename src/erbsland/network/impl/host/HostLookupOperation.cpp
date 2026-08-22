// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HostLookupOperation.hpp"

#include <utility>

namespace erbsland::network::impl {

HostLookupOperation::HostLookupOperation(Host operationHost, HostLookupOptions operationOptions) :
    host{std::move(operationHost)},
    options{operationOptions},
    deadline{time::TimePoint::inFuture(operationOptions.timeout())} {
}

}
