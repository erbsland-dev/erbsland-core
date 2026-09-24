// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Lexer_fwd.hpp"
#include "LexerToken.hpp"
#include "TokenGenerator.hpp"

#include "../decoder/TokenDecoder.hpp"
#include "../placeholder/PlaceholderResolver_fwd.hpp"
#include "../utilities/InternalView.hpp"

#include "../../../cryptology/HashAlgorithm.hpp"

namespace erbsland::conf::impl {

/// This lexer returns a low-level stream with tokens of the document syntax.
/// Each token contains the raw text of the document with the start and end positions, including tokens for
/// spacing, comments, and line-breaks. This is intentionally done to allow using this lexer for syntax highlighting.
/// On a successful run, there is *always* a last `EndOfData` token, with no raw text and no positions. This last
/// token makes sure that the exception that occurs after the last actual text is correctly propagated to the caller.
/// The method `tokens()` can only be called once.
class Lexer final {
    class PrivateTag {};

public:
    /// Create a new lexer, using the given decoder.
    /// @param decoder The decoder to use.
    /// @param placeholderResolver The optional placeholder registry.
    /// @return An instance of the lexer.
    [[nodiscard]] static auto create(
        CharStreamPtr decoder, placeholder::PlaceholderResolverPtr placeholderResolver = {}) noexcept -> LexerPtr {
        return std::make_shared<Lexer>(
            TokenDecoder::create(std::move(decoder), std::move(placeholderResolver)), PrivateTag{});
    }

    /// Create a new lexer, using the given decoder.
    /// @param decoder The buffered decoder to use.
    explicit Lexer(TokenDecoderPtr decoder, PrivateTag /*pt*/) noexcept : _decoder(std::move(decoder)) {
        assert(_decoder != nullptr);
    }

    // defaults
    ~Lexer() = default;

public:
    /// Access the source identifier for error messages.
    auto sourceIdentifier() const noexcept -> SourceIdentifierPtr;

    /// Get the tokens for the decoded document.
    /// You can call this method only once for a given decoder.
    /// Fully consume the generator after its terminal `EndOfData` token to finalize the document digest.
    /// @return A stream of `LexerToken` objects, on success, always ending with a last `EndOfData` token.
    /// @throws ConfError in case of any error while processing the input.
    auto tokens() -> TokenGenerator;

    /// Get the digest from the tokenized document.
    /// Must be called *after* fully consuming the token generator, including advancing past its end-of-data token.
    /// The stored digest remains available after the lexer releases its decoder.
    /// @return The digest for the document, or empty if none was created.
    [[nodiscard]] auto digest() const -> mem::ByteBlock;

    /// Get the algorithm that was used to create the hash digest for the document.
    [[nodiscard]] static auto hashAlgorithm() -> cryptology::HashAlgorithm;

    /// Closes this lexer, releasing the decoder and all resources.
    void close() noexcept;

private:
    /// Access the active token decoder.
    [[nodiscard]] auto decoder() const noexcept -> TokenDecoder & { return *_decoder; }

public: // testing
#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
    friend auto internalView(const Lexer &object) -> InternalViewPtr;
#endif

private:
    TokenDecoderPtr _decoder; ///< The token decoder.
    mem::ByteBlock _digest;   ///< The digest of the document.
};

}
