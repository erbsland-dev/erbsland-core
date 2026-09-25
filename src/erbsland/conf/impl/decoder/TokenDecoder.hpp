// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Decoder.hpp"
#include "TokenDecoder_fwd.hpp"
#include "Transaction.hpp"

#include "../char/CharStream.hpp"
#include "../lexer/LexerToken.hpp"

#include "../../../text/placeholder/impl/Registry_fwd.hpp"

#include <cassert>
#include <memory>

namespace erbsland::conf::impl {

/// A wrapper around a decoder tailored for decoding tokens.
/// - Adding transactions
/// - Adding exception helpers.
/// - Adding indentation states.
class TokenDecoder final : public Decoder {
public:
    /// Create a token decoder for a character stream.
    static auto create(CharStreamPtr decoder, text::placeholder::impl::RegistryPtr placeholderRegistry = {}) noexcept
        -> TokenDecoderPtr;

    /// Create a token decoder around a character stream.
    /// @param decoder The character stream to decode.
    /// @param placeholderRegistry The optional placeholder resolver.
    explicit TokenDecoder(
        CharStreamPtr decoder, text::placeholder::impl::RegistryPtr placeholderRegistry = {}) noexcept;

    // defaults
    TokenDecoder() = default;
    ~TokenDecoder() override = default;

public: // implement Decoder
    void initialize() override;

    [[nodiscard]] auto character() const noexcept -> text::Char override { return _currentCharacter.character(); }

    [[nodiscard]] auto location() const -> Location override;

    [[nodiscard]] auto sourceIdentifier() const noexcept -> SourceIdentifierPtr override;
    [[nodiscard]] auto codeSnippet(unit::CodeLocation location) const noexcept
        -> std::optional<text::CodeSnippet> override {
        return _decoder->source()->codeSnippet(location);
    }

    void next() override;

    /// Check for an error and throw it.
    /// Exceptions from the lower layers (like character-, or encoding-errors) would propagate faster
    /// than the actual yield of tokens. Therefore, the point of a such error is marked using an "Error"-character
    /// that caused the exception being rethrown *after* the last successfully parsed token.
    /// Calling this method checks if the current character contains the error mark. In this case, an exception with
    /// the details found in `_currentError` is thrown.
    void checkForErrorAndThrowIt() const override;

public:
    /// Get the position of the current character.
    [[nodiscard]] auto characterPosition() const -> unit::CodeLocation { return _currentCharacter.codeLocation(); }

    /// Get the digest from the decoder.
    /// Must be called *after* receiving the end-of-data token to get the digest of the document.
    /// @return The digest for the document, or empty if none was created.
    [[nodiscard]] auto digest() const noexcept -> mem::ByteBlock;

    /// Move to the next character and start a new token.
    void nextToken();

    /// Access the token start position.
    [[nodiscard]] auto tokenStartPosition() const noexcept -> const unit::CodeLocation & { return _tokenStartPosition; }

    /// Reset the token start position.
    void resetTokenStartPosition() { _tokenStartPosition = characterPosition(); }

    /// Get the current token size in characters.
    /// @note Only works for single line tokens.
    [[nodiscard]] auto tokenSize() const noexcept -> int;

public: // placeholder expansion
    /// Test if placeholder expansion is configured.
    [[nodiscard]] auto hasPlaceholders() const noexcept -> bool;

    /// Resolve a placeholder source.
    /// @param name The normalized source name.
    /// @param parameter The source parameter.
    /// @return The resolved text.
    [[nodiscard]] auto resolvePlaceholder(const text::String &name, const text::String &parameter) const
        -> text::String;

    /// Apply a placeholder filter.
    /// @param name The normalized filter name.
    /// @param parameter The filter parameter.
    /// @param value The current placeholder value.
    /// @return The filtered text.
    [[nodiscard]] auto applyPlaceholderFilter(
        const text::String &name, const text::String &parameter, const text::String &value) const -> text::String;

public: // Constraining functions.
    /// Expect more content in the current line.
    void expectMoreInLine(text::String message) const;

public: // create tokens from captured content
    /// Generic helper to create a token, capturing the current positions and raw text.
    template <typename T>
    [[nodiscard]] auto createToken(TokenType type, T &&value) -> LexerToken {
        // Capture all text up to the current character or the end of the document.
        auto capturedText =
            (_currentCharacter.character().isEndOfData() ? _decoder->captureToEndOfLine()
                                                         : _decoder->captureTo(_currentCharacter.index()));
        // Create the token and reset the start position.
        auto token = LexerToken{
            type, tokenStartPosition(), characterPosition(), std::move(capturedText), std::forward<T>(value)};
        resetTokenStartPosition();
        return token;
    }

    /// Create a token with no content.
    [[nodiscard]] auto createToken(TokenType type) -> LexerToken { return createToken(type, NoContent{}); }

    /// Create the end-of-line token.
    [[nodiscard]] auto createEndOfLineToken() -> LexerToken;

    /// Create the end-of-data token.
    [[nodiscard]] auto createEndOfDataToken() -> LexerToken;

public: // indentation handling.
    /// Test if a reusable indentation pattern is configured.
    [[nodiscard]] auto hasIndentationPattern() const noexcept -> bool { return !_currentIndentationPattern.isEmpty(); }
    /// Get the reusable indentation pattern.
    [[nodiscard]] auto indentationPattern() const noexcept -> text::String { return _currentIndentationPattern; }
    /// Set the reusable indentation pattern.
    void setIndentationPattern(text::String pattern) noexcept { _currentIndentationPattern = std::move(pattern); }
    /// Clear the reusable indentation pattern.
    void clearIndentationPattern() noexcept { _currentIndentationPattern = {}; }

public: // testing
#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
    friend auto internalView(const TokenDecoder &object) -> InternalViewPtr;
#endif

private: // implement Decoder transactions
    [[nodiscard]] auto decoderState() const noexcept -> DecoderState override;
    void restoreDecoderState(const DecoderState &state) noexcept override;
    [[nodiscard]] auto captureFromDecoderState(const DecoderState &state) const noexcept -> text::String override;

private:
    CharStreamPtr _decoder;                                         ///< The wrapped decoder.
    text::placeholder::impl::RegistryPtr _placeholderRegistry;      ///< Optional placeholder resolver.
    DecodedChar _currentCharacter{text::Char::endOfData(), {}, {}}; ///< The current decoded character.
    unit::CodeLocation _tokenStartPosition;                         ///< The start position of the current token.
    text::String _currentIndentationPattern;                        ///< The current indentation pattern.
    bool _hasUpcomingError{false};  ///< Set to `true` if an delayed error was scheduled.
    ConfErrorContext _currentError; ///< Details in case we got an error in the stream.
};

}
