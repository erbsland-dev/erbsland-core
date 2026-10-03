// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Match16_fwd.hpp"
#include "Match32_fwd.hpp"
#include "Match_fwd.hpp"
#include "RegEx_fwd.hpp"

#include "../text/String_fwd.hpp"
#include "../text/StringLiteral.hpp"
#include "../text/u16/U16String_fwd.hpp"
#include "../text/u32/U32String_fwd.hpp"

namespace erbsland::re {

/// Compile a literal pattern using the bounded application-scoped cache.
/// @param pattern Literal pattern; inline flags are supported with default settings.
/// @return An owning immutable compiled expression, also valid after cache eviction.
/// @throws RegExError If compilation fails.
/// @tested{RegExCachedTest}
[[nodiscard]] auto compile(text::StringLiteral pattern) -> ConstRegExPtr;

/// Match at the start of UTF-8 text using a cached literal pattern.
/// @param pattern Literal pattern compiled with default settings.
/// @param text Subject text.
/// @return The corresponding RegEx::match result.
/// @throws RegExError If compilation or execution fails.
/// @tested{RegExCachedTest}
[[nodiscard]] auto match(text::StringLiteral pattern, const text::String &text) -> MatchPtr;

/// Match at the start of UTF-16 text using a cached literal pattern.
/// @param pattern Literal pattern compiled with default settings.
/// @param text Subject text.
/// @return The corresponding RegEx::match result.
/// @throws RegExError If compilation or execution fails.
/// @tested{RegExCachedTest}
[[nodiscard]] auto match(text::StringLiteral pattern, const text::U16String &text) -> Match16Ptr;

/// Match at the start of UTF-32 text using a cached literal pattern.
/// @param pattern Literal pattern compiled with default settings.
/// @param text Subject text.
/// @return The corresponding RegEx::match result.
/// @throws RegExError If compilation or execution fails.
/// @tested{RegExCachedTest}
[[nodiscard]] auto match(text::StringLiteral pattern, const text::U32String &text) -> Match32Ptr;

/// Match the complete UTF-8 text using a cached literal pattern.
/// @param pattern Literal pattern compiled with default settings.
/// @param text Subject text.
/// @return The corresponding RegEx::fullMatch result.
/// @throws RegExError If compilation or execution fails.
/// @tested{RegExCachedTest}
[[nodiscard]] auto fullMatch(text::StringLiteral pattern, const text::String &text) -> MatchPtr;

/// Match the complete UTF-16 text using a cached literal pattern.
/// @param pattern Literal pattern compiled with default settings.
/// @param text Subject text.
/// @return The corresponding RegEx::fullMatch result.
/// @throws RegExError If compilation or execution fails.
/// @tested{RegExCachedTest}
[[nodiscard]] auto fullMatch(text::StringLiteral pattern, const text::U16String &text) -> Match16Ptr;

/// Match the complete UTF-32 text using a cached literal pattern.
/// @param pattern Literal pattern compiled with default settings.
/// @param text Subject text.
/// @return The corresponding RegEx::fullMatch result.
/// @throws RegExError If compilation or execution fails.
/// @tested{RegExCachedTest}
[[nodiscard]] auto fullMatch(text::StringLiteral pattern, const text::U32String &text) -> Match32Ptr;

/// Find the first match in UTF-8 text using a cached literal pattern.
/// @param pattern Literal pattern compiled with default settings.
/// @param text Subject text.
/// @return The corresponding RegEx::findFirst result.
/// @throws RegExError If compilation or execution fails.
/// @tested{RegExCachedTest}
[[nodiscard]] auto findFirst(text::StringLiteral pattern, const text::String &text) -> MatchPtr;

/// Find the first match in UTF-16 text using a cached literal pattern.
/// @param pattern Literal pattern compiled with default settings.
/// @param text Subject text.
/// @return The corresponding RegEx::findFirst result.
/// @throws RegExError If compilation or execution fails.
/// @tested{RegExCachedTest}
[[nodiscard]] auto findFirst(text::StringLiteral pattern, const text::U16String &text) -> Match16Ptr;

/// Find the first match in UTF-32 text using a cached literal pattern.
/// @param pattern Literal pattern compiled with default settings.
/// @param text Subject text.
/// @return The corresponding RegEx::findFirst result.
/// @throws RegExError If compilation or execution fails.
/// @tested{RegExCachedTest}
[[nodiscard]] auto findFirst(text::StringLiteral pattern, const text::U32String &text) -> Match32Ptr;

/// Lazily find all matches in UTF-8 text using a cached literal pattern.
/// @param pattern Literal pattern compiled with default settings.
/// @param text Subject text.
/// @return The corresponding RegEx::findAll result.
/// @throws RegExError If compilation or execution fails.
/// @tested{RegExCachedTest}
[[nodiscard]] auto findAll(text::StringLiteral pattern, const text::String &text) -> MatchGenerator;

/// Lazily find all matches in UTF-16 text using a cached literal pattern.
/// @param pattern Literal pattern compiled with default settings.
/// @param text Subject text.
/// @return The corresponding RegEx::findAll result.
/// @throws RegExError If compilation or execution fails.
/// @tested{RegExCachedTest}
[[nodiscard]] auto findAll(text::StringLiteral pattern, const text::U16String &text) -> Match16Generator;

/// Lazily find all matches in UTF-32 text using a cached literal pattern.
/// @param pattern Literal pattern compiled with default settings.
/// @param text Subject text.
/// @return The corresponding RegEx::findAll result.
/// @throws RegExError If compilation or execution fails.
/// @tested{RegExCachedTest}
[[nodiscard]] auto findAll(text::StringLiteral pattern, const text::U32String &text) -> Match32Generator;

/// Collect all matches in UTF-8 text using a cached literal pattern.
/// @param pattern Literal pattern compiled with default settings.
/// @param text Subject text.
/// @return The corresponding RegEx::collectAll result.
/// @throws RegExError If compilation or execution fails.
/// @tested{RegExCachedTest}
[[nodiscard]] auto collectAll(text::StringLiteral pattern, const text::String &text) -> MatchList;

/// Collect all matches in UTF-16 text using a cached literal pattern.
/// @param pattern Literal pattern compiled with default settings.
/// @param text Subject text.
/// @return The corresponding RegEx::collectAll result.
/// @throws RegExError If compilation or execution fails.
/// @tested{RegExCachedTest}
[[nodiscard]] auto collectAll(text::StringLiteral pattern, const text::U16String &text) -> Match16List;

/// Collect all matches in UTF-32 text using a cached literal pattern.
/// @param pattern Literal pattern compiled with default settings.
/// @param text Subject text.
/// @return The corresponding RegEx::collectAll result.
/// @throws RegExError If compilation or execution fails.
/// @tested{RegExCachedTest}
[[nodiscard]] auto collectAll(text::StringLiteral pattern, const text::U32String &text) -> Match32List;

/// Replace all UTF-8 matches using a cached literal pattern and a literal replacement expression.
/// @param pattern Literal pattern compiled with default settings.
/// @param text Subject text.
/// @param replacement Literal replacement text: `{0}` or `{}` inserts the full match, `{1}` or `{name}` a capture;
/// `{{` and `}}` insert literal braces. For example, `"[{name}]"_el` wraps a named capture.
/// @return Replaced text.
/// @throws RegExError If compilation, execution, or replacement parsing fails.
/// @tested{RegExCachedTest}
[[nodiscard]] auto replaceAll(text::StringLiteral pattern, const text::String &text, text::StringLiteral replacement)
    -> text::String;

}
