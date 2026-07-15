// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventLoopErrorAction.hpp"

#include <exception>
#include <functional>

namespace erbsland::event {

/// A callback that decides how an event loop reacts to an exception.
using EventLoopErrorHandler = std::function<EventLoopErrorAction(std::exception_ptr error)>;

}
