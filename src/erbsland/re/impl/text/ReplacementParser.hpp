// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Replacement.hpp"

#include "../engine/CaptureGroupNames.hpp"

#include "../../../text/String.hpp"
#include "../../../text/StringCharReader.hpp"
#include "../../CaptureGroup.hpp"

#include <optional>

namespace erbsland::re::impl {

/// A parser for replacement text.
/// @tested{ReplacementParserTest RegExReplaceAllTest}
class ReplacementParser {
public:
    /// Create a parser for a replacement `expression` and its capture-group names.
    ReplacementParser(const text::String &expression, const CaptureGroupNames &groupNames);

public:
    /// Parse the configured replacement expression.
    [[nodiscard]] auto parse() -> Replacement;
    /// Validate the expression without allocating replacement parts or temporary text.
    void validate();

private:
    /// Parse the expression, optionally collecting its replacement parts.
    void parseContent();
    /// Add a captured source slice when collecting replacement parts.
    void addCapturedText();
    /// Parse a replacement-expression fragment.
    [[nodiscard]] auto parseExpression() -> CaptureGroupIndex;
    /// Parse a numeric capture-group reference.
    [[nodiscard]] auto parseGroupIndex() -> CaptureGroupIndex;
    /// Parse a named capture-group reference.
    [[nodiscard]] auto parseGroupName() -> CaptureGroupIndex;
    /// Consume the closing brace or report a missing or invalid reference terminator.
    void requireClosingBrace(const text::String &description);
    /// Throw an error for the replacement expression.
    [[noreturn]] void throwError(text::String description) const;

private:
    text::StringCharReader _reader;
    const CaptureGroupNames &_groupNames;
    std::optional<Replacement> _replacement;
};

}
