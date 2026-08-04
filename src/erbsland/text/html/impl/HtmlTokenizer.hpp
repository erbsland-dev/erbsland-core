// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HtmlToken.hpp"

#include "../../../util/CoGenerator.hpp"
#include "../../AnyString.hpp"
#include "../../Char.hpp"
#include "../../String.hpp"
#include "../../StringCharReader.hpp"

#include <optional>

namespace erbsland::text::html::impl {

/// A tolerant tokenizer for the HTML subset parsed by `HtmlParser`.
/// @tested{HtmlTokenizerTest}
class HtmlTokenizer final {
public:
    using Token = HtmlToken;
    using TokenType = HtmlTokenType;
    using TokenGenerator = util::CoGenerator<HtmlToken>;

public:
    /// Create a tokenizer for the given HTML source text.
    /// @param html The HTML fragment or document to tokenize.
    explicit HtmlTokenizer(AnyString html);

    // defaults
    ~HtmlTokenizer() = default;
    HtmlTokenizer(const HtmlTokenizer &) = delete;
    HtmlTokenizer(HtmlTokenizer &&) = delete;
    auto operator=(const HtmlTokenizer &) -> HtmlTokenizer & = delete;
    auto operator=(HtmlTokenizer &&) -> HtmlTokenizer & = delete;

public:
    /// Tokenize the configured HTML input.
    /// @return A token generator that yields all parsed tokens.
    [[nodiscard]] auto tokenize() -> TokenGenerator;

private:
    /// Tokenize the tag that begins at the reader's current position.
    [[nodiscard]] auto tokenizeTag() -> bool;
    /// Tokenize an HTML declaration.
    [[nodiscard]] auto tokenizeDeclaration() -> bool;
    /// Tokenize an HTML closing tag.
    [[nodiscard]] auto tokenizeClosingTag() -> bool;
    /// Tokenize an HTML opening tag.
    [[nodiscard]] auto tokenizeOpeningTag() -> bool;
    /// Tokenize text outside an HTML tag.
    void tokenizeText();
    /// Tokenize the literal content of an HTML tag.
    void tokenizeLiteralTagText();
    /// Tokenize an HTML comment.
    [[nodiscard]] auto tokenizeComment() -> bool;
    /// Tokenize an HTML document type declaration.
    [[nodiscard]] auto tokenizeDocType() -> bool;
    /// Decode an HTML character entity at the reader's current position.
    [[nodiscard]] auto decodeEntity(Char &decodedCharacter) -> bool;
    /// Parse the value of the current HTML attribute.
    [[nodiscard]] auto parseAttributeValue(String &value) -> bool;
    /// Parse an HTML name at the reader's current position.
    [[nodiscard]] auto parseName() -> std::optional<String>;
    /// Advance past whitespace at the reader's current position.
    void skipWhitespace() noexcept;
    /// Take the accumulated buffer as a string.
    [[nodiscard]] auto takeBufferString() -> String;
    /// Create a trimmed string from the accumulated buffer.
    [[nodiscard]] auto trimmedBufferString() const -> String;
    /// Read and consume the next input character.
    [[nodiscard]] auto peekNext() noexcept -> Char;

    /// Test if a character ends an HTML name.
    [[nodiscard]] static auto isNameTerminator(Char character) noexcept -> bool;
    /// Test if a character ends an HTML attribute value.
    [[nodiscard]] static auto isAttributeValueTerminator(Char character) noexcept -> bool;

private:
    StringCharReader _reader; ///< The character reader used for all tokenizer input and buffering.
    HtmlToken _currentToken;  ///< The token currently being assembled.
};

}
