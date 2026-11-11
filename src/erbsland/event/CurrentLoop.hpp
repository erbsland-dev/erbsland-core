// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventTarget_fwd.hpp"

namespace erbsland::event {

/// Access the target interface of the event loop currently running on this thread.
/// The calling thread must be running an event loop.
/// @return The event target for the loop bound to the current thread.
/// @tested{ApplicationEventTest EventThreadTest EventLoopTest}
[[nodiscard]] auto currentLoop() -> EventTargetPtr;

}
