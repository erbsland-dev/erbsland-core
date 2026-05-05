// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::util {

/// A generic result why a loop has ended.
enum class LoopResult : uint8_t {
    /// The main end condition was reached.
    /// - for strings: the end of the string was reached.
    /// - for parsers: the end condition was reached.
    Success,
    /// The read function requested a stop before the end condition was reached.
    Stopped,
    /// A maximum number of iterations/chars was reached, but the loop could potentially continue.
    LimitReached,
    /// The end of the data was reached.
    EndOfData,
    /// The loop function reported an error.
    Error,
};

}
