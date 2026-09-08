// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::system::impl {

/// A normalized process termination input.
enum class ProcessSignal : std::uint8_t {
    Interrupt,
    Terminate,
    Hangup,
    Quit,
    ConsoleBreak,
    ConsoleClose,
};

}
