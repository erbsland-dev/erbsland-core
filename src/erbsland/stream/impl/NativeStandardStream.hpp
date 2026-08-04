// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::stream::impl {

/// The standard output stream to wrap.
enum class NativeStandardStream : uint8_t {
    Out, ///< Process standard output.
    Err, ///< Process standard error.
};

}
