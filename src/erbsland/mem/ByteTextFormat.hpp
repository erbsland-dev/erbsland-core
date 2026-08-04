// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::mem {

/// The byte-level framing mode for text values.
enum class ByteTextFormat : uint8_t {
    Dynamic,     ///< A dynamically-sized text value.
    PaddedField, ///< A fixed-size field with optional trailing padding.
};

}
