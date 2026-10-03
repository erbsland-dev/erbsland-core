// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::conf::syntax {

/// Placement policy for values without multiline literal payloads or list bullets.
enum class ValueWrapping {
    Keep,           ///< Preserve whether the value starts on the assignment line or the next line.
    SameLine,       ///< Put the complete value on the assignment line, even if it exceeds the soft limit.
    NextLine,       ///< Put the complete value on an indented continuation line.
    NextLineIfFits, ///< Move an overlong value only when its continuation line fits the soft limit.
    WhenTooLong,    ///< Move an overlong value even when its continuation line also exceeds the limit.
};

}
