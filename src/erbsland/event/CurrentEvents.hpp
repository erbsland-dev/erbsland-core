// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Events_fwd.hpp"

namespace erbsland::event {

/// Access the events interface for the event loop currently running on this thread.
/// @return The events interface for the loop bound to the current thread.
/// @throws err::LogicError If the current thread is not running an event loop.
[[nodiscard]] auto currentEvents() -> EventsPtr;

}
