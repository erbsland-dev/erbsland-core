// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Decoder.hpp"
#include "Transaction.hpp"

#include "../../../unit/ByteIndex.hpp"

namespace erbsland::conf::impl {

/// A minimal decoder for the name lexer.
class FastNameDecoder final : public Decoder {
    using Buffer = text::String;

public:
    /// Create a new instance of the name decoder.
    /// @param buffer The buffer to decode.
    explicit FastNameDecoder(Buffer buffer);

    // defaults
    ~FastNameDecoder() override = default;
    FastNameDecoder(const FastNameDecoder &) = delete;
    FastNameDecoder(FastNameDecoder &&) = delete;
    auto operator=(const FastNameDecoder &) -> FastNameDecoder & = delete;
    auto operator=(FastNameDecoder &&) -> FastNameDecoder & = delete;

public: // implement Decoder
    void initialize() override;

    [[nodiscard]] auto character() const noexcept -> text::Char override { return _currentChar; }

    [[nodiscard]] auto location() const -> Location override;

    [[nodiscard]] auto sourceIdentifier() const noexcept -> SourceIdentifierPtr override;

    void next() override;

public:
    /// Access the buffer
    [[nodiscard]] auto buffer() const noexcept -> const Buffer & { return _buffer; }

    /// Check if there is more.
    [[nodiscard]] auto hasNext() const noexcept -> bool { return _readIndex < unit::ByteIndex::end(_buffer.length()); }

    /// Read the next UTF-8 character and update the current decoder state.
    void readCurrentCharacter();

private: // implement Decoder transactions
    [[nodiscard]] auto decoderState() const noexcept -> DecoderState override;
    void restoreDecoderState(const DecoderState &state) noexcept override;
    [[nodiscard]] auto captureFromDecoderState(const DecoderState &state) const noexcept -> text::String override;

private:
    Buffer _buffer;                                   ///< The buffer to decode.
    text::Char _currentChar{text::Char::endOfData()}; ///< The current character.
    unit::ByteIndex _charIndex;                       ///< The byte index of the current character.
    unit::ByteIndex _readIndex;                       ///< The byte index of the next character.
    unit::CodeLocation _position;                     ///< The location of the current character.
};

}
