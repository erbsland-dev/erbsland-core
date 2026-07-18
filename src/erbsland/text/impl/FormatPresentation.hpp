// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::impl {

/// Presentation type for formatted fields.
enum class FormatPresentation : uint8_t {
    Default,
    StringEditor,
    Decimal,
    Hex,
    Binary,
    Octal,
    FloatFixed,
    FloatScientific,
    FloatGeneral,
    FloatHex,
    EscapedText,
};

}
