// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HtmlToken.hpp"

#include "../../../util/CoGenerator.hpp"
#include "../../AnyStringView.hpp"
#include "../../Char.hpp"
#include "../../StringCharReader.hpp"
#include "../../StringView.hpp"

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
    explicit HtmlTokenizer(AnyStringView html);

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
    [[nodiscard]] auto tokenizeTag() -> bool;
    [[nodiscard]] auto tokenizeDeclaration() -> bool;
    [[nodiscard]] auto tokenizeClosingTag() -> bool;
    [[nodiscard]] auto tokenizeOpeningTag() -> bool;
    void tokenizeText();
    void tokenizeLiteralTagText();
    [[nodiscard]] auto tokenizeComment() -> bool;
    [[nodiscard]] auto tokenizeDocType() -> bool;
    [[nodiscard]] auto decodeEntity(Char &decodedCharacter) -> bool;
    [[nodiscard]] auto parseAttributeValue(StringView &value) -> bool;
    [[nodiscard]] auto parseName() -> std::optional<StringView>;
    void skipWhitespace() noexcept;
    [[nodiscard]] auto takeBufferStringView() -> StringView;
    [[nodiscard]] auto trimmedBufferStringView() const -> StringView;
    [[nodiscard]] auto peekNext() noexcept -> Char;

    [[nodiscard]] static auto isNameTerminator(Char character) noexcept -> bool;
    [[nodiscard]] static auto isAttributeValueTerminator(Char character) noexcept -> bool;

private:
    StringCharReader _reader; ///< The character reader used for all tokenizer input and buffering.
    HtmlToken _currentToken;  ///< The token currently being assembled.
};

}
