// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::util {

/// The reported status of a loop function.
enum class LoopStatus : uint8_t {
    /// Continue looping.
    Continue,
    /// Stop the loop early in a regular way.
    Stop,
    /// Stop the loop, reporting an error.
    Error,
};

}
