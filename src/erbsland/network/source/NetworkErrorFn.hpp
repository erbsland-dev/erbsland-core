// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NetworkErrorContext_fwd.hpp"

#include <functional>

namespace erbsland::network {

/// A callback receiving structured context for an asynchronous network error.
using NetworkErrorFn = std::function<void(const NetworkErrorContext &)>;

}
