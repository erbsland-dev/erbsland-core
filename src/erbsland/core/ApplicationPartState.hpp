// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::core {

/// The lifecycle state of an application part.
enum class ApplicationPartState : uint8_t {
    Uninitialized, ///< The part was prepared but not started.
    Starting,      ///< The part thread is running startup hooks.
    Running,       ///< The part completed startup.
    Stopping,      ///< The part is performing graceful shutdown.
    Stopped,       ///< The part completed a normal one-shot lifecycle.
    Failed,        ///< The part completed its lifecycle after a failure.
};

}
