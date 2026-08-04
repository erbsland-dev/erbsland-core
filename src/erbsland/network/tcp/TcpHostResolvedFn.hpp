// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../IpEndpoint.hpp"

#include "../../util/List.hpp"

#include <functional>

namespace erbsland::network {

/// A callback receiving ordered resolved endpoints before an outgoing TCP connection begins.
using TcpHostResolvedFn = std::function<void(const util::List<IpEndpoint> &)>;

}
