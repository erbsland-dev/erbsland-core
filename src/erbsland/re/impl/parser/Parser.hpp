// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ParserState.hpp"

#include "../../Settings.hpp"

namespace erbsland::re::impl {

/// A parser for regular expressions.
/// Takes a regular expression text and parses it into a node tree.
class Parser {
public:
    /// Create a new parser for the given pattern.
    /// @param reader The reader for the pattern to parse.
    /// @param flags The initial group flags.
    /// @param settings The initial settings.
    explicit Parser(text::StringCharReader reader, GroupFlags flags = {}, Settings settings = {});
    /// Create a new parser for a UTF-8 pattern.
    explicit Parser(const text::StringView &pattern, GroupFlags flags = {}, Settings settings = {}) :
        Parser{text::StringCharReader{pattern}, flags, std::move(settings)} {}
    /// Create a new parser for a UTF-16 pattern.
    explicit Parser(const text::U16StringView &pattern, GroupFlags flags = {}, Settings settings = {}) :
        Parser{text::StringCharReader{pattern}, flags, std::move(settings)} {}
    /// Create a new parser for a UTF-32 pattern.
    explicit Parser(const text::U32StringView &pattern, GroupFlags flags = {}, Settings settings = {}) :
        Parser{text::StringCharReader{pattern}, flags, std::move(settings)} {}

    /// Create an empty parser, just for compatibility and tests.
    Parser() : Parser(text::StringCharReader{}, GroupFlags{}) {}

    // defaults: allow move, disallow copy.
    Parser(const Parser &) = delete;
    Parser(Parser &&) = default;
    auto operator=(const Parser &) -> Parser & = delete;
    auto operator=(Parser &&) -> Parser & = default;
    ~Parser() = default;

public:
    /// Parse the pattern into a node tree.
    [[nodiscard]] auto parse() -> PatternNodePtr;

private:
    /// Pre-flight checks.
    void preFlightChecks() const;
    /// Check the pattern size.
    void checkPatternSize() const;
    static void handleHashCharacter(ParserState &state);
    static void handleSpacing(ParserState &state);
    [[noreturn]] static void throwUnexpectedCharacterError(ParserState &state);

private:
    ParserState _state;
};

}
