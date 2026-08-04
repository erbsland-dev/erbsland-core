// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpConnectionFilterResult.hpp"

#include "../IpEndpoint.hpp"

#include <functional>

namespace erbsland::network {

/// A synchronous callback deciding whether an incoming TCP endpoint is admitted.
using TcpConnectionFilterFn = std::function<TcpConnectionFilterResult(const IpEndpoint &)>;

}
