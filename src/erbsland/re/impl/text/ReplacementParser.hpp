// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Replacement.hpp"

#include "../engine/CaptureGroupNames.hpp"

#include "../../../text/String.hpp"
#include "../../../text/StringCharReader.hpp"
#include "../../../text/StringCIHashMap.hpp"
#include "../../CaptureGroup.hpp"

namespace erbsland::re::impl {

/// A parser for replacement text.
class ReplacementParser {
public:
    /// Create a parser for a replacement `expression` and its capture-group names.
    ReplacementParser(const text::String &expression, const CaptureGroupNames &groupNames);

public:
    /// Parse the configured replacement expression.
    [[nodiscard]] auto parse() -> Replacement;

private:
    /// Read the next replacement-expression character.
    void readNext();
    /// Parse a replacement-expression fragment.
    void parseExpression();
    /// Parse a numeric capture-group reference.
    void parseGroupIndex();
    /// Parse a named capture-group reference.
    void parseGroupName();
    /// Require another expression character.
    void requireMoreContent();
    /// Throw an error for the replacement expression.
    [[noreturn]] void throwError(text::String description) const;

private:
    Replacement _replacement;
    text::StringCharReader _reader;
    text::Char _currentChar{text::Char::noCodePoint()};
    text::StringCIHashMap<CaptureGroupIndex> _nameMap;
    text::StringEditor _staticText;
};

}
