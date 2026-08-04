// Copyright (c) 2024-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Content.hpp"
#include "TokenType.hpp"

#include "../utilities/InternalView.hpp"

#include "../../../text/String.hpp"
#include "../../../unit/CodeLocation.hpp"

#include <cassert>

namespace erbsland::conf::impl {

/// A single lexer token.
class LexerToken final {
public: // construction
    /// Create a new lexer token.
    /// @param type The token type.
    /// @param begin The first character of the token.
    /// @param end After the last character of the token.
    /// @param rawText The raw text that was used to parse the token.
    /// @param value The converted value of the token.
    template <typename T>
    constexpr LexerToken(
        const TokenType type,
        const unit::CodeLocation begin,
        const unit::CodeLocation end,
        text::String rawText,
        T &&value = NoContent{}) noexcept :
        _type{type}, _begin{begin}, _end{end}, _rawText{std::move(rawText)}, _content{std::forward<T>(value)} {}

    /// Create a simple lexer token of a given type with no content.
    /// Meant to explicitly create end-of-data and error tokens.
    /// @param type Either `TokenType::EndOfData` or `TokenType::EndOfData`
    explicit constexpr LexerToken(const TokenType type) noexcept : _type{type} {
        assert(type == TokenType::EndOfData || type == TokenType::Error);
    }

    // defaults
    LexerToken() = default; // Implicitly created an end-of-data token.
    ~LexerToken() = default;
    LexerToken(const LexerToken &) = default;
    LexerToken(LexerToken &&) = default;
    auto operator=(const LexerToken &) -> LexerToken & = default;
    auto operator=(LexerToken &&) -> LexerToken & = default;

public: // accessors
    /// Get the token type.
    [[nodiscard]] constexpr auto type() const noexcept -> TokenType { return _type; }
    /// Get the source location at the start of the token.
    [[nodiscard]] constexpr auto begin() const noexcept -> unit::CodeLocation { return _begin; }
    /// Get the source location at the end of the token.
    [[nodiscard]] constexpr auto end() const noexcept -> unit::CodeLocation { return _end; }
    /// Get the raw source text of the token.
    [[nodiscard]] auto rawText() const noexcept -> text::String { return _rawText; }
    /// Get the parsed token content.
    [[nodiscard]] constexpr auto content() const noexcept -> const Content & { return _content; }

public:
#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
    friend auto internalView(const LexerToken &token) noexcept -> InternalViewPtr;
#endif

private:
    TokenType _type{TokenType::EndOfData};
    unit::CodeLocation _begin;
    unit::CodeLocation _end;
    text::String _rawText;
    Content _content{NoContent{}};
};

}
