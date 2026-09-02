// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint.hpp"
#include "ConstraintHandlerContext.hpp"

#include "../char/NamedChars.hpp"

#include "../../../text/CharRange.hpp"
#include "../../../text/CharSet.hpp"
#include "../../../text/StringList.hpp"

#include <vector>

namespace erbsland::conf::impl {

/// Constraint that restricts the characters allowed in text.
class CharsConstraint final : public Constraint {
    using NamedRange = std::pair<text::String, text::CharSet>;

public:
    /// Create a character constraint from expected range definitions.
    /// @param expectedValue The configured character-range definitions.
    explicit CharsConstraint(const text::StringList &expectedValue) : Constraint{vr::ConstraintType::Chars} {
        _charSet = parseTextRanges(expectedValue);
    }

protected:
    void validateText(const ValidationContext &context, const text::String &value) const override;

private:
    /// Parse text ranges into character ranges.
    static auto parseTextRanges(const text::StringList &texts) -> text::CharSet;

    /// Access the list of predefined character ranges.
    static auto namedRanges() -> const std::vector<NamedRange> &;

    /// Parse a parenthesized range expression: (a-z)
    static void parseParenRange(const std::vector<text::Char> &cps, const text::String &rawText, text::CharSet &out);

    /// Parse a bracket list: [abc] — each inner character must be unique
    static void parseBracketList(const std::vector<text::Char> &cps, text::CharSet &out);

    /// Try to match a named range; returns true if a known name was appended to out
    [[nodiscard]] static auto tryAppendNamedRange(
        const text::String &text, const std::vector<NamedRange> &named, text::CharSet &out) -> bool;

private:
    text::CharSet _charSet;
};

/// Create a character restriction constraint from its parsed definition.
auto handleCharsConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr;

}
