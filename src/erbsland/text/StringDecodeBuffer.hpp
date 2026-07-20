// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AnyStringEditor.hpp"
#include "Char.hpp"
#include "EncodingErrorMode.hpp"
#include "String.hpp"
#include "StringBomMode.hpp"
#include "StringEditor.hpp"
#include "StringEncoding.hpp"

#include "u16/U16StringEditor.hpp"
#include "u32/U32StringEditor.hpp"
#include "u8/U8StringEditor.hpp"

#include "../mem/Byte.hpp"
#include "../mem/ByteBlock.hpp"
#include "../unit/ByteIndex.hpp"
#include "../unit/ByteLength.hpp"
#include "../unit/CpLength.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace erbsland::text::impl {
class UnsafeDecodeBufferAccess;
}

namespace erbsland::text {

/// A bounded byte buffer for incrementally decoding encoded string data.
/// The buffer keeps incomplete trailing code points pending until more data is written or `finish()` is called.
/// @tested{StringDecodeBufferTest}
class StringDecodeBuffer final {
    friend class impl::UnsafeDecodeBufferAccess;

public:
    /// The status of the buffered data at the code-point boundary.
    enum class CodePointStatus : uint8_t {
        Complete,     ///< The buffer ends at a complete code-point boundary.
        NeedMoreData, ///< The buffer ends with a valid prefix that needs more bytes.
        Invalid,      ///< The buffer contains invalid bytes.
    };

public:
    /// Create a decode buffer.
    /// @param bufferLength The byte capacity. Must be at least four bytes.
    /// @param encoding The configured text encoding.
    /// @param bomMode How byte order marks are handled.
    /// @param errorMode How decoding errors are handled.
    /// @throws err::ParameterError If `bufferLength` is smaller than four bytes.
    explicit StringDecodeBuffer(
        unit::ByteLength bufferLength,
        StringEncoding encoding,
        StringBomMode bomMode = StringBomMode::Automatic,
        EncodingErrorMode errorMode = EncodingErrorMode::Replace);

    // defaults
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
    [[nodiscard]] auto capacity() const noexcept -> unit::ByteLength;
    /// Get the available byte space.
    [[nodiscard]] auto availableSpace() const noexcept -> unit::ByteLength;
    /// Get the number of buffered bytes.
    [[nodiscard]] auto byteLength() const noexcept -> unit::ByteLength;
    /// Count how many complete characters can be decoded now.
    [[nodiscard]] auto decodableCharacters(unit::CpLength maximum = unit::CpLength::infinite()) -> unit::CpLength;
    /// Test if no bytes are buffered.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _byteLength.isZero(); }
    /// Test if `finish()` was called.
    [[nodiscard]] auto isFinished() const noexcept -> bool { return _finished; }
    /// Get the code-point boundary status of the buffered data.
    [[nodiscard]] auto codePointStatus() -> CodePointStatus;
    /// Test if the buffered data ends at a complete code-point boundary.
    [[nodiscard]] auto isCodePointComplete() -> bool { return codePointStatus() == CodePointStatus::Complete; }

public: // input
    /// Write bytes to the buffer.
    /// @throws err::ParameterError If the data does not fit into the available space.
    void write(std::span<const mem::Byte> bytes);
    /// Write bytes to the buffer.
    /// @throws err::ParameterError If the data does not fit into the available space.
    void write(const mem::ByteBlock &bytes);
    /// Write bytes to the buffer.
    /// @throws err::ParameterError If the data does not fit into the available space.
    void write(const std::vector<mem::Byte> &bytes);
    /// Write bytes to the buffer.
    /// @throws err::ParameterError If the data does not fit into the available space.
    void write(const std::vector<uint8_t> &bytes);
    /// Write bytes to the buffer.
    /// @throws err::ParameterError If the data does not fit into the available space.
    void write(const std::vector<char> &bytes);
    /// Write bytes to the buffer.
    /// @throws err::ParameterError If the data does not fit into the available space.
    void write(std::string_view bytes);
    /// Write raw UTF-8 bytes from a read-only string.
    /// @throws err::ParameterError If the data does not fit into the available space.
    void writeStringBytes(const String &bytes);
    /// Mark the input as complete.
    void finish() noexcept { _finished = true; }
    /// Reset the buffer.
    void reset() noexcept;

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
    /// @return The decoded character, or empty if no complete character is available.
    /// @throws text::EncodingError If the buffer was configured to throw on decoding errors and decoding fails.
    [[nodiscard]] auto readChar() -> std::optional<Char>;
    /// Decode and consume available bytes to a string matching the effective encoding.
    [[nodiscard]] auto takeAnyString(unit::CpLength maximum = unit::CpLength::infinite()) -> AnyString;
    /// Decode and consume available bytes to the default string type.
    [[nodiscard]] auto takeString(unit::CpLength maximum = unit::CpLength::infinite()) -> String;
    /// Decode and consume one line to the default string type.
    /// The returned line ends after a decoded LF character if one is available within `maximum`.
    [[nodiscard]] auto takeStringLine(unit::CpLength maximum = unit::CpLength::infinite()) -> String;
    /// Decode and consume available bytes to a UTF-8 string.
    [[nodiscard]] auto takeU8String(unit::CpLength maximum = unit::CpLength::infinite()) -> U8String;
    /// Decode and consume available bytes to a UTF-16 string.
    [[nodiscard]] auto takeU16String(unit::CpLength maximum = unit::CpLength::infinite()) -> U16String;
    /// Decode and consume available bytes to a UTF-32 string.
    [[nodiscard]] auto takeU32String(unit::CpLength maximum = unit::CpLength::infinite()) -> U32String;

private:
    struct ScanResult final {
        CodePointStatus status{CodePointStatus::NeedMoreData};
        unit::ByteLength byteLength{};
    };

private:
    [[nodiscard]] auto writableSpan() noexcept -> std::span<mem::Byte>;
    void commitWritten(unit::ByteLength length);
    void appendByte(mem::Byte byte) noexcept;
    [[nodiscard]] auto writeIndex() const noexcept -> unit::ByteIndex;
    [[nodiscard]] auto byteAt(unit::ByteIndex index) const noexcept -> mem::Byte;
    [[nodiscard]] auto byteMatches(std::span<const mem::Byte> prefix) const noexcept -> bool;
    [[nodiscard]] auto byteMatchesAvailable(std::span<const mem::Byte> prefix) const noexcept -> bool;
    [[nodiscard]] auto materialize(unit::ByteLength length) const -> mem::ByteBlock;
    [[nodiscard]] auto decodeContentToU8(const mem::ByteBlock &data) const -> U8String;
    [[nodiscard]] auto decodeContentToU16(const mem::ByteBlock &data) const -> U16String;
    [[nodiscard]] auto decodeContentToU32(const mem::ByteBlock &data) const -> U32String;
    void consume(unit::ByteLength length) noexcept;
    [[nodiscard]] auto consumedByteLength() const noexcept -> unit::ByteLength;
    [[nodiscard]] auto isBomResolved() const noexcept -> bool;
    void resetForContinuation(StringEncoding effectiveEncoding) noexcept;
    [[nodiscard]] auto ensureBomResolved() -> bool;
    [[nodiscard]] auto decodableByteLength(unit::CpLength maximum, unit::CpLength *characterCount = nullptr)
        -> unit::ByteLength;
    [[nodiscard]] auto lineByteLength(unit::CpLength maximum, unit::CpLength *characterCount = nullptr)
        -> unit::ByteLength;
    [[nodiscard]] auto scanCodePoint(unit::ByteIndex index) const noexcept -> ScanResult;
    [[nodiscard]] auto decodeCharacter(unit::ByteIndex index, unit::ByteLength byteLength) const noexcept -> Char;
    [[nodiscard]] auto scanUtf8(unit::ByteIndex index) const noexcept -> ScanResult;
    [[nodiscard]] auto scanUtf16(unit::ByteIndex index) const noexcept -> ScanResult;
    [[nodiscard]] auto scanUtf32(unit::ByteIndex index) const noexcept -> ScanResult;
    [[nodiscard]] auto readUInt16(unit::ByteIndex index) const noexcept -> char16_t;
    [[nodiscard]] auto readUInt32(unit::ByteIndex index) const noexcept -> char32_t;
    [[nodiscard]] static auto isContinuationByte(uint8_t value) noexcept -> bool;

private:
    std::vector<mem::Byte> _buffer; ///< The ring buffer storage.
    unit::ByteIndex _readIndex{};   ///< The first buffered byte in the ring storage.
    unit::ByteLength _byteLength{}; ///< The number of buffered bytes.
    StringEncoding _encoding{StringEncoding::Utf8};
    StringEncoding _effectiveEncoding{StringEncoding::Utf8};
    StringBomMode _bomMode{StringBomMode::Automatic};
    EncodingErrorMode _errorMode{EncodingErrorMode::Replace};
    bool _finished{false};
    bool _bomResolved{false};
    unit::ByteLength _consumedByteLength{}; ///< Bytes consumed since construction or the last reset.
};

}
