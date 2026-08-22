// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::core {

/// The lifecycle state of an application-part manager.
enum class ApplicationPartManagerState : uint8_t {
    Uninitialized, ///< Parts may still be registered.
    Ready,         ///< The graph and all part instances are prepared.
    Starting,      ///< Automatic startup is in progress.
    Running,       ///< Automatic startup settled.
    Stopping,      ///< Complete manager shutdown is in progress.
    Stopped,       ///< The manager completed normal shutdown.
    Failed,        ///< The manager completed shutdown after a fatal failure.
};

}
