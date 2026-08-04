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
    explicit Parser(const text::String &pattern, GroupFlags flags = {}, Settings settings = {}) :
        Parser{text::StringCharReader{pattern}, flags, std::move(settings)} {}
    /// Create a new parser for a UTF-16 pattern.
    explicit Parser(const text::U16String &pattern, GroupFlags flags = {}, Settings settings = {}) :
        Parser{text::StringCharReader{pattern}, flags, std::move(settings)} {}
    /// Create a new parser for a UTF-32 pattern.
    explicit Parser(const text::U32String &pattern, GroupFlags flags = {}, Settings settings = {}) :
        Parser{text::StringCharReader{pattern}, flags, std::move(settings)} {}

    /// Create an empty parser, just for compatibility and tests.
    Parser() : Parser(text::StringCharReader{}, GroupFlags{}) {}

    // defaults/deletions
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
    /// Handle a hash character in the parser state.
    static void handleHashCharacter(ParserState &state);
    /// Handle ignorable spacing in the parser state.
    static void handleSpacing(ParserState &state);
    /// Throw an error for the current unexpected input character.
    [[noreturn]] static void throwUnexpectedCharacterError(ParserState &state);

private:
    ParserState _state;
};

}
