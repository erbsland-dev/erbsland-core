// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Token.hpp"

#include "../EnvironmentOptions.hpp"

#include "../../../text/StringCharReader.hpp"

#include <optional>

namespace erbsland::text::render::impl {

/// Stream lexical tokens from one complete layout source.
/// @tested{RenderTokenizerTest RenderCompilerTest RenderExpressionTest RenderStatementTest}
class Tokenizer final {
    /// The tokenizer's current source mode.
    enum class Mode : uint8_t {
        Text,       ///< Reading raw layout text.
        Expression, ///< Reading an output expression.
        Statement,  ///< Reading a statement.
    };

    /// A matched tag opening and its lexical configuration.
    struct Opening {
        Mode mode;                    ///< The mode after the opening delimiter.
        TokenKind tokenKind;          ///< The token emitted for the opening delimiter.
        const Delimiters *delimiters; ///< The matching delimiter configuration.
        bool isComment{false};        ///< Whether the complete tag must be discarded.
    };

public:
    /// Create a tokenizer for one complete layout source.
    Tokenizer(String layout, String origin, String source, EnvironmentOptions options);

    // defaults/deletions
    ~Tokenizer() = default;
    Tokenizer(const Tokenizer &) = delete;
    Tokenizer(Tokenizer &&) = delete;
    auto operator=(const Tokenizer &) -> Tokenizer & = delete;
    auto operator=(Tokenizer &&) -> Tokenizer & = delete;

public:
    /// Get the current token.
    [[nodiscard]] auto current() const noexcept -> const Token & { return _token; }
    /// Advance to the next token.
    void advance();

private: // modes
    /// Read the next raw-text or tag-opening token.
    [[nodiscard]] auto readTextToken() -> Token;
    /// Read the next token inside an expression or statement tag.
    [[nodiscard]] auto readTagToken() -> Token;
    /// Skip one complete comment tag.
    void skipComment(const Opening &opening, unit::CodeLocation tagLocation);

private: // lexical tokens
    /// Read an identifier or reserved keyword token.
    [[nodiscard]] auto readIdentifier(unit::CodeLocation location) -> Token;
    /// Read a signed decimal integer or float token.
    [[nodiscard]] auto readNumber(unit::CodeLocation location) -> Token;
    /// Read and decode a quoted string token.
    [[nodiscard]] auto readString(Char quote, unit::CodeLocation location) -> Token;
    /// Read a Unicode escape, including an optional surrogate pair.
    [[nodiscard]] auto readUnicodeEscape(unit::CodeLocation location) -> Char;
    /// Read exactly four hexadecimal digits.
    [[nodiscard]] auto readHexCodeUnit(unit::CodeLocation location) -> uint16_t;

private: // delimiter handling
    /// Find an opening delimiter at the current reader position.
    [[nodiscard]] auto openingAtCurrent() -> std::optional<Opening>;
    /// Test whether the active closing delimiter begins at the current position.
    [[nodiscard]] auto isAtClosingDelimiter() -> bool;
    /// Test whether the right-trim marker and active closing delimiter begin at the current position.
    [[nodiscard]] auto isAtRightTrimClosingDelimiter() -> bool;
    /// Test whether a reader begins with the given text.
    [[nodiscard]] auto matches(const String &expected) -> bool;
    /// Consume text that was already verified to match and update the source location.
    void consume(const String &expected);

private: // reader tools
    /// Read one character and update the source location.
    auto read() -> Char;
    /// Read one expected character unless it begins the active closing delimiter.
    [[nodiscard]] auto readIf(Char expected) -> bool;
    /// Read matching ASCII characters without crossing an active closing delimiter.
    auto readWhile(AsciiCategory category) -> unit::CpLength;
    /// Test whether the character after the current one is an ASCII digit and not a delimiter.
    [[nodiscard]] auto nextIsAsciiDigit() -> bool;
    /// Advance the source location for one consumed character.
    void advanceLocation(Char character) noexcept;
    /// Advance the source location for known consumed text.
    void advanceLocation(const String &text);
    /// Skip raw ASCII whitespace after a right-trim marker.
    void skipRawWhitespace();
    /// Throw a source-aware syntax error.
    [[noreturn]] void throwSyntax(String title, String description, unit::CodeLocation location) const;

private:
    String _layout;                               ///< Logical layout name for diagnostics.
    String _origin;                               ///< Source origin for diagnostics.
    String _source;                               ///< Shared complete source text.
    EnvironmentOptions _options;                  ///< Validated syntax options.
    Char _expressionOpeningCharacter;             ///< First expression-opening delimiter character.
    Char _statementOpeningCharacter;              ///< First statement-opening delimiter character.
    Char _commentOpeningCharacter;                ///< First comment-opening delimiter character.
    CharSet _openingCharacters;                   ///< Candidate characters for raw-text delimiter scanning.
    StringCharReader _reader;                     ///< The single primary source reader.
    unit::CodeLocation _location;                 ///< Location of the next unread character.
    Token _token;                                 ///< Current one-token lookahead.
    Mode _mode{Mode::Text};                       ///< Current lexical mode.
    const Delimiters *_activeDelimiters{nullptr}; ///< Delimiters for the active tag.
    Char _activeClosingCharacter;                 ///< First active closing delimiter character.
    unit::CodeLocation _tagLocation;              ///< Location of the active tag opener.
    std::optional<Token> _pendingToken;           ///< Structural token pending after emitted raw text.
};

}
