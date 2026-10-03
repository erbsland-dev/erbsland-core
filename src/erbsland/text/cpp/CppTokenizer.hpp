// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CppToken.hpp"

#include "../StringCharReader.hpp"

#include "../../util/CoGenerator.hpp"

namespace erbsland::text::cpp {

/// Scan C++ source into tokens while sharing immutable source storage.
/// @seedoc{/reference/text/cpp_tokenizer}
/// @tested{CppTokenizerTest}
class CppTokenizer final {
public:
    /// Bind a source string and begin scanning at its first code point.
    /// @param source The immutable source; its storage is retained by the tokenizer.
    explicit CppTokenizer(const String &source);

public: // scanning
    /// Skip whitespace and read the next token, retaining comments, directives, and literals.
    /// @return The next token, or an End token at the source length on every call after the last token.
    /// @throws err::ParseError For an unterminated comment or literal, or an invalid raw-string delimiter.
    [[nodiscard]] auto next() -> CppToken;

public: // generators
    /// Iterate tokens while retaining the source in the coroutine; omit the End token.
    /// @param source The source string whose storage the coroutine retains.
    /// @return A lazy sequence of tokens in source order.
    /// @throws err::ParseError During iteration if a comment or literal is malformed.
    [[nodiscard]] static auto tokens(String source) -> util::CoGenerator<CppToken>;

private:
    /// Capture the consumed token with its start offset.
    [[nodiscard]] auto capture(CppToken::Kind kind, unit::ByteIndex position, unit::CpIndex characterPosition)
        -> CppToken;
    /// Classify and consume a token using its already decoded first character.
    [[nodiscard]] auto readToken(Char first) -> CppToken::Kind;
    /// Probe a slash-prefixed comment, restoring the cursor when it is punctuation.
    [[nodiscard]] auto readComment() -> bool;
    /// Consume a block comment after its opening delimiter.
    void readBlockComment();
    /// Probe a literal prefix once, restoring the cursor when it is an identifier.
    [[nodiscard]] auto readLiteral(Char first) -> bool;
    /// Consume a numeric token, retaining the decoded character for each decision.
    void readNumber(Char character);
    /// Consume an identifier starting at its known first character.
    void readIdentifier();
    /// Consume punctuation using longest-match decisions from its first character.
    void readPunctuation(Char first);
    /// Consume a logical line, including backslash continuations and CRLF endings.
    void readLine();
    /// Consume an ordinary quoted string or character literal after its opening delimiter.
    void readQuoted(Char quote);
    /// Consume a raw string after its opening quote.
    void readRaw();
    /// Report malformed input at the current decoded code-point position.
    [[noreturn]] void fail(const String &message) const;

private:
    StringCharReader _reader; ///< Shared source and independent cursor/capture state.
    bool _lineStart = true;   ///< Whether only whitespace/comments precede this token on its line.
};

}
