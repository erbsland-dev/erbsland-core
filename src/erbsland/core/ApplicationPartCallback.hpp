// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationPartErrorAction.hpp"
#include "ApplicationPartIdentifier_fwd.hpp"
#include "ApplicationPartManagerState.hpp"
#include "ApplicationPartState.hpp"

#include <exception>
#include <functional>

namespace erbsland::core {

/// A callback deciding how a manager handles a part failure.
using ApplicationPartErrorHandler =
    std::function<ApplicationPartErrorAction(ApplicationPartIdentifierPtr identifier, std::exception_ptr error)>;
/// A callback observing manager state transitions.
using ApplicationPartManagerStateChangedFn = std::function<void(ApplicationPartManagerState state)>;
/// A callback observing part state transitions.
using ApplicationPartStateChangedFn =
    std::function<void(ApplicationPartIdentifierPtr identifier, ApplicationPartState state)>;

}
