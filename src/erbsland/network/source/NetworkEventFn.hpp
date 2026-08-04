// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <functional>

namespace erbsland::network {

/// A callback for a network state transition without associated data.
using NetworkEventFn = std::function<void()>;

}
