// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace tool::idna {

/// Source-derived IDNA status values.
/// @tested{UnicodeSourceTest PublicSuffixFullTest}
enum class IdnaStatus : uint8_t {
    Disallowed, ///< The code point is disallowed or unassigned.
    PValid,     ///< The code point is protocol-valid.
    ContextJ,   ///< The code point requires a CONTEXTJ rule.
    ContextO,   ///< The code point requires a CONTEXTO rule.
};

}
