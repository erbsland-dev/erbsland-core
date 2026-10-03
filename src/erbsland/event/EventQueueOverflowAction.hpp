// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::event {

/// The action when an event queue reaches capacity.
enum class EventQueueOverflowAction : uint8_t {
    Terminate,       ///< Terminate the process.
    FailApplication, ///< Forward a queue-full failure to the owning application.
    Quit,            ///< Reject new work and drain accepted events.
    Block,           ///< Wait for space; terminate if the consumer would wait.
    Ignore,          ///< Discard the incoming event.
};

}
