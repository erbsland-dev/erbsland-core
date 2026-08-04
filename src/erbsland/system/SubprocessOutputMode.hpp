// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::system {

/// Selects how a subprocess output stream is handled.
enum class SubprocessOutputMode : std::uint8_t {
    Inherit, ///< Inherit the matching stream from the parent process.
    Discard, ///< Discard all bytes written to the stream.
    Capture, ///< Capture a bounded prefix and continue draining excess bytes.
};

}
