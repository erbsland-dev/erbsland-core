// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Assignment.hpp"
#include "AssignmentGenerator.hpp"
#include "AssignmentStream_fwd.hpp"

#include "../lexer/Lexer.hpp"

#include <cassert>

namespace erbsland::conf::impl {

/// A stream that uses a lexer to parse a document and return a stream of value assignments.
/// What this stream does:
/// - The stream verifies the syntax of the document structure, returned by the lexer and also assembles multi-line
///   text and value lists.
/// - The value stream does not interpret the parsed names and values, it returns them as a sequential stream
///   of value assignments.
/// - Verifies the meta `\@version` and `\@features`.
/// - Verifies if values and meta-values are defined at the right places.
/// - Builds absolute section names from relative ones and verifies that relative sections are defined
///   after an absolute one.
/// - Verifies that `\@signature` is in the first line.
/// - Resets the context after `\@include`, so no value or relative section is allowed after it.
/// What a user of this stream needs to handle:
/// - Verify and handle name conflicts.
/// - Handle errors when text/regular names are mixed in a section.
/// - Create intermediate sections.
/// - Convert intermediate sections in regular ones on assignment.
/// - Convert regular section into text sections after first text-name assignment.
/// - Handle the meta `\@signature` *after* all assignments, call `Lexer::digest()` for the digest.
/// - Handle `\@include` meta-commands to include documents.
/// What this stream returns:
/// - An assignment instance for each encountered section, list-section entry, value and meta-value or meta-command.
/// - One `AssignmentType::EndOfDocument` at the end of the assignment stream.
/// - All returned name paths are absolute name paths from the document root.
/// - All values are completely assembled to the assignment level, yet detached (not in a container).
///   This includes value lists and nested value lists.
///     `AssignmentStreamSectionListTest`, `AssignmentStreamTextNameTest`.
class AssignmentStream final {
    class PrivateTag {};

    /// The document area.
    enum class DocumentArea : uint8_t {
        /// The root area, before the first section definition.
        Root,
        /// After a section definition.
        AfterSection,
    };

public:
    /// Create a new assignment stream.
    /// @param lexer The lexer for the stream.
    /// @return An instance of the assignment stream.
    [[nodiscard]] static auto create(LexerPtr lexer) -> AssignmentStreamPtr;

    /// Create a new assignment stream.
    /// @param lexer The lexer for the stream.
    explicit AssignmentStream(LexerPtr lexer, PrivateTag);

    // defaults
    ~AssignmentStream() = default;

    // defaults/deletions
    AssignmentStream(const AssignmentStream &) = delete;
    auto operator=(const AssignmentStream &) -> AssignmentStream & = delete;

public:
    /// Generate a list of assignments from the document.
    auto assignments() -> AssignmentGenerator;

private:
    /// Initialize the token generator and read its first token.
    void initialize();

    /// Read the next non-spacing token.
    void next();

    // All `expect...` methods are like `asserts`, as they just test what the lexer should already verify.

    /// Expect a next token of any type.
    /// Also throws an exception, if the next token is a line-break or the end-of-data.
    void expectNext();

    /// Verify that the current token is either a line-break or the end of the document.
    /// If the current token is a line-break, also consume this token.
    /// If the current token is neither a line-break, nor the end of the document, a ConfError is thrown.
    void verifyAndConsumeEndOfLine();

    /// Expect one or more token types.
    template <typename... ExpectedTokens>
        requires(std::is_same_v<ExpectedTokens, TokenType::Value> || ...)
    void expectNext(ExpectedTokens... expectedTokens) {
        next();
        if (!((token().type() == expectedTokens) || ...)) {
            // Coverage: If this exception is thrown, it means the lexer did not capture a basic syntax test.
            throwSyntaxError("Unexpected character sequence"_el);
        }
    }

    /// Get the next token and verify it is of a certain type.
    /// This is actual verification on the next token, where we could get a different token from the lexer.
    /// @param expectedTokenType The expected token type.
    /// @param errorMessage The error message, if no token or not the expected token follows.
    void nextAndVerify(TokenType expectedTokenType, text::String errorMessage);

    /// Access the current token.
    [[nodiscard]] auto token() const -> const LexerToken &;

    /// Throw an error with the start position of the current token.
    /// @{
    [[noreturn]] void throwSyntaxError(text::String message) const;
    /// Throw a syntax error at a name-path location.
    [[noreturn]] void throwSyntaxError(text::String message, const NamePath &namePath) const;
    /// Throw an error for unsupported assignment syntax.
    [[noreturn]] void throwUnsupportedError(text::String message) const;
    /// Throw an error for unexpected end of input.
    [[noreturn]] void throwUnexpectedEndError(text::String message) const;
    /// Throw an error for an exceeded implementation limit.
    [[noreturn]] void throwLimitExceededError(text::String message) const;
    /// @}

    /// Get the location, based on the given lexer source and token.
    [[nodiscard]] auto currentLocation() const noexcept -> Location;

    /// Handle meta values.
    [[nodiscard]] auto handleMetaValue() -> Assignment;

    /// Handle regular values.
    [[nodiscard]] auto handleValue() -> Assignment;

    /// Handle a single value, or value list.
    [[nodiscard]] auto handleValueOrValueList() -> std::vector<ValuePtr>;

    /// Handle a multi-line value list.
    [[nodiscard]] auto handleMultiLineValueList() -> std::vector<ValuePtr>;

    /// Handle multi-line text.
    [[nodiscard]] auto handleMultiLineText() -> text::String;

    /// Handle multi-line regular expressions.
    [[nodiscard]] auto handleMultiLineRegEx() -> text::String;

    /// Create a lazily compiled regular expression.
    [[nodiscard]] auto createRegEx(const text::String &pattern, bool isVerbose) const -> re::RegExPtr;

    /// Handle multi-line bytes.
    [[nodiscard]] auto handleMultiLineBytes() -> mem::ByteBlock;

    /// Handle sections.
    [[nodiscard]] auto handleSection() -> Assignment;

    /// Verify if the given list of features matches the supported features of this parser.
    void verifyFeatures(const text::String &text) const;

private:
    LexerPtr _lexer;                                ///< The lexer instance passed to this assignment stream.
    TokenGenerator _lexerGenerator;                 ///< The token generator.
    LexerToken _token{TokenType::EndOfData};        ///< The current token.
    bool _readMetaVersion{false};                   ///< If a `version` meta-value was read.
    bool _readMetaFeatures{false};                  ///< If a `features` meta-value was read.
    DocumentArea _documentArea{DocumentArea::Root}; ///< The document area.
    NamePath _lastAbsolutePath;                     ///< The last absolute name path definition.
    NamePath _currentSectionPath;                   ///< The name path for the current section.
};

}
