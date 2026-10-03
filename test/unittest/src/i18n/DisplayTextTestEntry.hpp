// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/StringLiteral.hpp>

/// Literal fixture entry with a separately specified domain.
/// @notest{Literal unit-test fixture.}
struct DisplayTextTestEntry {
    el::text::StringLiteral domain; ///< Literal domain path.
    el::text::StringLiteral key;    ///< Literal final name.
    el::text::StringLiteral text;   ///< Literal source text.
};
