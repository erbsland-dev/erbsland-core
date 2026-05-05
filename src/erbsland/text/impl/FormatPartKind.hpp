// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::impl {

/// The formatting mode for an argument field.
/// @tested{U8FormatTest}
enum class FormatPartKind : uint8_t {
    Field,
    StaticText,
};

}
