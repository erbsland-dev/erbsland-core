// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cterm {

/// A terminal flag.
enum class TerminalFlag : uint8_t {
    /// Disables signal handling to restore the screen when the application is terminated.
    /// If this flag is set, you must ensure that the `restoreScreen()` method is called when the application
    /// is terminated by a signal. Otherwise, the terminal will not be restored properly.
    NoSignalHandling = 1 << 0,
};

}
