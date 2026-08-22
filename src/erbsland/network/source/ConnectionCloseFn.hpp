// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConnectionCloseContext_fwd.hpp"

#include <functional>

namespace erbsland::network {

/// A callback for an orderly connection closure.
using ConnectionCloseFn = std::function<void(const ConnectionCloseContext &)>;

}
