// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../IpAddress.hpp"

#include "../../util/List.hpp"

#include <functional>

namespace erbsland::network {

/// A callback receiving the resolved addresses for a host lookup.
using HostResolvedFn = std::function<void(const util::List<IpAddress> &)>;

}
