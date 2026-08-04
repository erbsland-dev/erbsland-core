// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::network::impl {

struct HostLookupOperation;

/// Shared storage for one asynchronous host lookup operation.
using HostLookupOperationPtr = std::shared_ptr<HostLookupOperation>;

}
