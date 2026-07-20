// Copyright (c) 2024-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CharStream_fwd.hpp"

#include "../constants/Defaults.hpp"
#include "../constants/Limits.hpp"
#include "../decoder/DecodedChar.hpp"
#include "../utilities/InternalView.hpp"

#include "../../../cryptology/Hasher.hpp"
#include "../../../unit/ByteIndex.hpp"
#include "../../../unit/ByteRange.hpp"
#include "../../Source.hpp"

#include <cassert>
#include <span>

namespace erbsland::conf::impl {

using namespace text::literals;

/// The UTF-8 and line decoder.
/// - Reads the lines from the source.
/// - Keeps track of the location.
/// - Decodes and verifies the UTF-8 input.
/// - Handles end of a file.
/// @tested{CharStreamTest DecoderHashTest}
class CharStream final {
public:
    /// Create a new char stream from the given source.
    static auto create(SourcePtr source) noexcept -> CharStreamPtr;
    /// Create a new char stream from the given source.
    explicit CharStream(SourcePtr source) noexcept;

    // defaults
    ~CharStream() = default;

public:
    /// Decode the next character in the stream.
    [[nodiscard]] auto next() -> DecodedChar;
    /// Capture the text up to the given position.
    /// @param endPosition The byte-index of the end position to capture.
    [[nodiscard]] auto captureTo(unit::ByteIndex endPosition) -> text::String;
    /// Capture everything up to the end of the line.
    [[nodiscard]] auto captureToEndOfLine() noexcept -> text::String;
    /// Capture a COW slice from the current line.
    /// @param startPosition The inclusive byte start.
    /// @param endPosition The exclusive byte end.
    [[nodiscard]] auto captureRange(unit::ByteIndex startPosition, unit::ByteIndex endPosition) const noexcept
        -> text::String;
    /// Access the source used by this decoder.
    [[nodiscard]] auto source() const noexcept -> const SourcePtr & { return _source; }
    /// Access the last start index of the current character.
    /// @note This call is used, in case of an error, to get the precise error location. For the token decoder, storing
    /// the precise location of the error is important to preserve the delayed error while a speculative decoder
    /// checkpoint is restored.
    [[nodiscard]] auto lastCharacterStartIndex() const noexcept -> unit::ByteIndex { return _lineCharacterStartIndex; }
    /// Access the byte index of the next unread character in the current line.
    [[nodiscard]] auto nextReadIndex() const noexcept -> unit::ByteIndex { return _lineReadIndex; }
    /// Restore the line decoder to a previously checkpointed character.
    /// The source itself is not rewound because parser transactions cannot cross a line boundary.
    void restore(
        unit::ByteIndex nextReadIndex, unit::ByteIndex characterStartIndex, unit::CodeLocation position) noexcept;
    /// Get the hash digest for the document.
    /// Call this function, *after* you received the end-of-document character. The digest is only available
    /// when the decoder detected a `\@signature` value in the first line of the document.
    /// @return The digest, or empty if no digest was created.
    [[nodiscard]] auto digest() const noexcept -> mem::ByteBlock { return _digest; }
    /// Manually enable hash calculation.
    void enableHash() noexcept { _hashEnabled = true; }
    /// Determine whether the current line starts with a signature marker.
    /// @return `true` if a "@signature" value is detected.
    [[nodiscard]] auto isSignatureLine() const noexcept -> bool;

private:
    /// Reads the next line from the source into the internal buffer.
    /// - Resets the line's current index and its character start index.
    /// - Updates the capture-related metadata to prepare for further
    ///   data decoding.
    /// - Resets the start of text capture fields for the current line.
    /// @return `true` if a line was read, or `false` at the end of the source.
    /// @throws ConfError passes all exceptions from the `Source::readLine()` call.
    auto readNextLine() -> bool;
    /// Decode the next UTF-8 sequence in the line buffer.
    /// @return The decoded character.
    auto decodeNext() -> DecodedChar;
    /// Create the end-of-data character.
    /// Updates the current position and finalizes the digest when called the first time.
    /// @return The special end-of-data marker.
    [[nodiscard]] auto createEndOfData() -> DecodedChar;
    /// Throw a character-related error at the current document position.
    /// @param message The diagnostic text.
    /// @throws ConfError Always thrown.
    void throwCharacterError(text::String message) const;
    /// Throw an internal error at the current document position.
    /// @param message The diagnostic text.
    /// @throws ConfError Always thrown.
    void throwInternalError(text::String message) const;
    /// Check if the decoder reached the end of the current line buffer.
    /// @return `true` when the line index matches the line length.
    [[nodiscard]] auto isAtEndOfLine() const noexcept -> bool { return _lineReadIndex >= _lineEndIndex; }

public: // testing
#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
    friend auto internalView(const CharStream &object) -> InternalViewPtr;
#endif

private:
    SourcePtr _source;                                          ///< The input source.
    bool _endOfData{false};                                     ///< True, if the end of the data was reached.
    text::String _line;                                         ///< The current COW line from the source.
    std::span<const char> _lineBytes;                           ///< The immutable UTF-8 bytes of the current line.
    unit::ByteIndex _lineEndIndex;                              ///< The exclusive byte end of the current line.
    unit::ByteIndex _lineReadIndex;                             ///< The next UTF-8 byte to decode in the current line.
    unit::ByteIndex _lineCharacterStartIndex;                   ///< The byte index where the last character started.
    unit::LineIndex _captureStartLine{unit::LineIndex::zero()}; ///< The capture start line (for integrity checks).
    unit::ByteIndex _captureStartIndex;                         ///< The start byte index for the next capture.
    unit::CodeLocation _position;                               ///< The current location.
    bool _hashEnabled{false};                                   ///< True if a `\@signature` line is encountered.
    bool _hashFinalized{false};                                 ///< True after the document digest was finalized.
    cryptology::Hasher _hash{defaults::documentHashAlgorithm};  ///< The hash function for signed documents.
    mem::ByteBlock _digest;                                     ///< The hash digest.
};

}
