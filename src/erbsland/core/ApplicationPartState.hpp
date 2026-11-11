// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Definitions.hpp"

#include <cstdint>

namespace erbsland::core {

/// The lifecycle state of an application part.
enum class ApplicationPartState : uint8_t {
    Uninitialized, ///< The part was prepared but not started.
    Starting,      ///< The part thread is running its initialization callback.
    Running,       ///< Initialization completed; the running callback and normal event work may execute.
    Stopping,      ///< The part is performing graceful shutdown.
    Stopped,       ///< The part completed a normal one-shot lifecycle.
    Failed,        ///< The part completed its lifecycle after a failure.
};

}
