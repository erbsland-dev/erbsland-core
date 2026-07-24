// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AnyStringEditor.hpp"
#include "Char.hpp"
#include "EncodingMode.hpp"
#include "String.hpp"
#include "StringBomMode.hpp"
#include "StringEditor.hpp"
#include "StringEncoding.hpp"

#include "u16/U16StringEditor.hpp"
#include "u32/U32StringEditor.hpp"
#include "u8/U8StringEditor.hpp"

#include "../err/ParameterError.hpp"
#include "../mem/Byte.hpp"
#include "../mem/ByteBlock.hpp"
#include "../mem/ByteBuffer.hpp"
#include "../mem/ByteSpan.hpp"
#include "../unit/ByteIndex.hpp"
#include "../unit/ByteLength.hpp"
#include "../unit/CpLength.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace erbsland::text::impl {
class UnsafeDecodeBufferAccess;
}

namespace erbsland::text {

/// A bounded byte buffer for incrementally decoding encoded string data.
/// Sensitive mode protects discarded input bytes and marks UTF-8 results.
/// @seedoc{/reference/text/string_decode_buffer}
/// @tested{StringDecodeBufferTest}
class StringDecodeBuffer final {
    friend class impl::UnsafeDecodeBufferAccess;

public:
    /// The status of buffered data at the code-point boundary.
    enum class CodePointStatus : uint8_t {
        Complete,     ///< The buffer ends at a complete code-point boundary.
        NeedMoreData, ///< A valid trailing prefix needs more bytes.
        Invalid,      ///< The buffer contains invalid bytes.
    };

private:
    struct ScanResult final {
        CodePointStatus status{CodePointStatus::NeedMoreData}; ///< Boundary status.
        unit::ByteLength byteLength{};                         ///< Bytes covered by this result.
    };

    /// A byte and character range selected for one decode operation.
    struct DecodedRange final {
        unit::ByteLength byteLength;    ///< Source bytes covered by the selection.
        unit::CpLength characterLength; ///< Characters produced by the selection.
        bool isValid{true};             ///< Whether the selection contains only valid encoded characters.
    };

    /// A decoded UTF-8 string with the character count found while decoding.
    struct DecodedU8String final {
        U8String text;                  ///< Decoded text.
        unit::CpLength characterLength; ///< Number of decoded characters.
    };

public:
    /// Create a decode buffer.
    /// @param bufferLength The fixed byte capacity. Must be at least four bytes.
    /// @param encoding The configured text encoding.
    /// @param bomMode How byte order marks are handled.
    /// @param mode How malformed input is handled.
    /// @throws err::ParameterError If `bufferLength` is smaller than four bytes.
    explicit StringDecodeBuffer(
        unit::ByteLength bufferLength,
        StringEncoding encoding,
        StringBomMode bomMode = StringBomMode::Automatic,
        EncodingMode mode = EncodingMode::Tolerant);

    // defaults/deletions
    ~StringDecodeBuffer() = default;
    StringDecodeBuffer(const StringDecodeBuffer &) = delete;
    StringDecodeBuffer(StringDecodeBuffer &&) = default;
    auto operator=(const StringDecodeBuffer &) -> StringDecodeBuffer & = delete;
    auto operator=(StringDecodeBuffer &&) -> StringDecodeBuffer & = default;

public: // state
    /// Get the configured encoding.
    [[nodiscard]] auto encoding() const noexcept -> StringEncoding { return _encoding; }
    /// Get the effective encoding after BOM resolution.
    [[nodiscard]] auto effectiveEncoding() const noexcept -> StringEncoding { return _effectiveEncoding; }
    /// Get the byte capacity.
    [[nodiscard]] auto capacity() const noexcept -> unit::ByteLength { return _buffer.length(); }
    /// Get the available byte space.
    [[nodiscard]] auto availableSpace() const noexcept -> unit::ByteLength { return capacity() - _byteLength; }
    /// Get the number of buffered bytes.
    [[nodiscard]] auto byteLength() const noexcept -> unit::ByteLength { return _byteLength; }
    /// Count complete characters available for decoding.
    [[nodiscard]] auto decodableCharacters(unit::CpLength maximum = unit::CpLength::infinite()) -> unit::CpLength;
    /// Test if no bytes are buffered.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _byteLength.isZero(); }
    /// Test if input was marked complete.
    [[nodiscard]] auto isFinished() const noexcept -> bool { return _finished; }
    /// Get the code-point boundary status.
    [[nodiscard]] auto codePointStatus() -> CodePointStatus;
    /// Test if buffered data ends at a complete code-point boundary.
    [[nodiscard]] auto isCodePointComplete() -> bool { return codePointStatus() == CodePointStatus::Complete; }

public: // input
    /// Write borrowed bytes into available storage.
    void write(mem::ConstByteSpan bytes);
    /// Write an ordinary byte block.
    void write(const mem::ByteBlock &bytes);
    /// Write explicit byte values.
    void write(const std::vector<mem::Byte> &bytes);
    /// Write unsigned byte values.
    void write(const std::vector<uint8_t> &bytes);
    /// Write character byte values.
    void write(const std::vector<char> &bytes);
    /// Write character bytes.
    void write(std::string_view bytes);
    /// Write raw UTF-8 bytes from a string.
    void writeStringBytes(const String &bytes);
    /// Mark input complete.
    void finish() noexcept { _finished = true; }
    /// Reset decoder state, preserving its sensitivity mode.
    void reset() noexcept;

public: // sensitivity
    /// Test if discarded decoder storage is securely erased and UTF-8 results are marked.
    [[nodiscard]] auto isSensitive() const noexcept -> bool { return _buffer.isSensitive(); }
    /// Enable or disable sensitive decoder storage.
    /// Disabling securely erases buffered input and resets decoder state.
    void setSensitive(bool sensitive) noexcept;

public: // peek
    /// Decode available bytes to a string matching the effective encoding.
    [[nodiscard]] auto peekAnyString(unit::CpLength maximum = unit::CpLength::infinite()) -> AnyString;
    /// Decode available bytes to the default string type.
    [[nodiscard]] auto peekString(unit::CpLength maximum = unit::CpLength::infinite()) -> String;
    /// Decode available bytes to a UTF-8 string.
    [[nodiscard]] auto peekU8String(unit::CpLength maximum = unit::CpLength::infinite()) -> U8String;
    /// Decode available bytes to a UTF-16 string.
    [[nodiscard]] auto peekU16String(unit::CpLength maximum = unit::CpLength::infinite()) -> U16String;
    /// Decode available bytes to a UTF-32 string.
    [[nodiscard]] auto peekU32String(unit::CpLength maximum = unit::CpLength::infinite()) -> U32String;

public: // take
    /// Decode and consume one character.
    [[nodiscard]] auto readChar() -> std::optional<Char>;
    /// Decode and consume available bytes to a string matching the effective encoding.
    [[nodiscard]] auto takeAnyString(unit::CpLength maximum = unit::CpLength::infinite()) -> AnyString;
    /// Decode and consume available bytes to the default string type.
    [[nodiscard]] auto takeString(unit::CpLength maximum = unit::CpLength::infinite()) -> String;
    /// Decode and consume one line to the default string type.
    [[nodiscard]] auto takeStringLine(unit::CpLength maximum = unit::CpLength::infinite()) -> String;
    /// Decode and consume available bytes to a UTF-8 string.
    [[nodiscard]] auto takeU8String(unit::CpLength maximum = unit::CpLength::infinite()) -> U8String;
    /// Decode and consume available bytes to a UTF-16 string.
    [[nodiscard]] auto takeU16String(unit::CpLength maximum = unit::CpLength::infinite()) -> U16String;
    /// Decode and consume available bytes to a UTF-32 string.
    [[nodiscard]] auto takeU32String(unit::CpLength maximum = unit::CpLength::infinite()) -> U32String;

private:
    [[nodiscard]] auto writableSpan() noexcept -> mem::ByteSpan;
    void commitWritten(unit::ByteLength length);
    [[nodiscard]] auto consumedByteLength() const noexcept -> unit::ByteLength { return _consumedByteLength; }
    [[nodiscard]] auto isBomResolved() const noexcept -> bool { return _bomResolved; }
    void resetForContinuation(StringEncoding effectiveEncoding) noexcept;
    [[nodiscard]] auto takeStringWithLength(unit::CpLength maximum, bool stopAtLineEnd)
        -> std::pair<String, unit::CpLength>;
    [[nodiscard]] auto decodableByteLength(
        unit::CpLength maximum, unit::CpLength *characterCount = nullptr, bool *isValid = nullptr) -> unit::ByteLength;
    [[nodiscard]] auto lineByteLength(
        unit::CpLength maximum, unit::CpLength *characterCount = nullptr, bool *isValid = nullptr) -> unit::ByteLength;
    [[nodiscard]] auto decodedRange(unit::CpLength maximum, bool stopAtLineEnd) -> DecodedRange;
    template <typename Function>
    auto forEachDecodedCharacter(DecodedRange range, bool consumeDecoded, Function function) -> unit::CpLength;
    void copyUtf8Range(DecodedRange range, std::span<char> destination, bool consumeDecoded);
    void consume(unit::ByteLength length) noexcept;
    [[nodiscard]] auto writeIndex() const noexcept -> unit::ByteIndex;
    [[nodiscard]] auto byteAt(unit::ByteIndex index) const noexcept -> mem::Byte;
    [[nodiscard]] auto byteMatches(mem::ConstByteSpan prefix) const noexcept -> bool;
    [[nodiscard]] auto byteMatchesAvailable(mem::ConstByteSpan prefix) const noexcept -> bool;
    void erasePrefix(unit::ByteLength length) noexcept;
    [[nodiscard]] auto ensureBomResolved() -> bool;
    [[nodiscard]] auto scanCodePoint(unit::ByteIndex index) const noexcept -> ScanResult;
    [[nodiscard]] auto decodeCharacter(unit::ByteIndex index, unit::ByteLength byteLength) const noexcept -> Char;
    [[nodiscard]] auto scanUtf8(unit::ByteIndex index) const noexcept -> ScanResult;
    [[nodiscard]] auto scanUtf16(unit::ByteIndex index) const noexcept -> ScanResult;
    [[nodiscard]] auto scanUtf32(unit::ByteIndex index) const noexcept -> ScanResult;
    [[nodiscard]] auto readUInt16(unit::ByteIndex index) const noexcept -> char16_t;
    [[nodiscard]] auto readUInt32(unit::ByteIndex index) const noexcept -> char32_t;
    [[nodiscard]] static auto isContinuationByte(uint8_t value) noexcept -> bool;
    [[nodiscard]] auto storageWritableSpan() noexcept -> mem::ByteSpan;
    void resetStorage() noexcept {
        if (_buffer.isSensitive()) {
            _buffer.secureErase();
        }
    }
    [[nodiscard]] auto decodeToU8(unit::CpLength maximum, bool stopAtLineEnd, bool consumeDecoded) -> DecodedU8String;
    [[nodiscard]] auto decodeToU16(unit::CpLength maximum, bool consumeDecoded) -> U16String;
    [[nodiscard]] auto decodeToU32(unit::CpLength maximum, bool consumeDecoded) -> U32String;

private:
    mem::ByteBuffer _buffer;                                 ///< Fixed-capacity ring storage.
    unit::ByteIndex _readIndex{};                            ///< First buffered byte in ring storage.
    unit::ByteLength _byteLength{};                          ///< Number of buffered bytes.
    StringEncoding _encoding{StringEncoding::Utf8};          ///< Configured encoding.
    StringEncoding _effectiveEncoding{StringEncoding::Utf8}; ///< BOM-resolved encoding.
    StringBomMode _bomMode{StringBomMode::Automatic};        ///< BOM handling mode.
    EncodingMode _mode{EncodingMode::Tolerant};              ///< Invalid-input handling mode.
    bool _finished{false};                                   ///< Whether the source is complete.
    bool _bomResolved{false};                                ///< Whether initial BOM handling is complete.
    unit::ByteLength _consumedByteLength{};                  ///< Bytes consumed since reset.
};

}

#include "impl/StringDecodeBuffer.tpp"
