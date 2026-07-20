// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::impl {

/// Domain selected by a typed named format specification.
enum class NamedFormatDomain : uint8_t {
    Text,
    Number,
    Boolean,
    Bytes,
};

}
