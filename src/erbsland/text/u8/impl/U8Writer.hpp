// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8Encoding.hpp"

#include "../../../mem/ByteWriter.hpp"
#include "../../../mem/impl/RingBufferWriter.hpp"
#include "../../u16/impl/U16Encoding.hpp"
#include "../../u32/impl/U32Encoding.hpp"

#include <span>
#include <string>
#include <type_traits>

namespace erbsland::text::impl {

/// A writer for UTF-8 encoded text.
/// @tested{U8WriterTest}
template <typename tTarget>
class U8Writer {};

/// A writer for UTF-8 encoded text into a character span.
/// @tested{U8WriterTest}
template <typename tChar8>
    requires(std::is_same_v<tChar8, char> || std::is_same_v<tChar8, char8_t>)
class U8Writer<std::span<tChar8>> {
public:
    explicit constexpr U8Writer(const std::span<tChar8> data) noexcept : _data{data} {}

public:
    /// Get the current position in the destination buffer.
    [[nodiscard]] auto position() const noexcept -> std::size_t { return _position; }
    /// Write one Unicode scalar value as a UTF-8 sequence into the destination buffer.
    /// A call for invalid characters is ignored.
    /// @param character A valid Unicode code-point to write.
    [[nodiscard]] auto write(const Char character) noexcept {
        if (!character.isValidUnicode()) {
            return;
        }
        const auto codePoint = character.toRawValue();
        if (codePoint < 0x80U) {
            writeChar8(static_cast<tChar8>(codePoint));
        } else if (codePoint < 0x800U) {
            writeChar8(static_cast<tChar8>(0xC0U | ((codePoint >> 6U) & 0x1FU)));
            writeChar8(static_cast<tChar8>(0x80U | (codePoint & 0x3FU)));
        } else if (codePoint < 0x10000U) {
            writeChar8(static_cast<tChar8>(0xE0U | ((codePoint >> 12U) & 0x0FU)));
            writeChar8(static_cast<tChar8>(0x80U | ((codePoint >> 6U) & 0x3FU)));
            writeChar8(static_cast<tChar8>(0x80U | (codePoint & 0x3FU)));
        } else {
            writeChar8(static_cast<tChar8>(0xF0U | ((codePoint >> 18U) & 0x07U)));
            writeChar8(static_cast<tChar8>(0x80U | ((codePoint >> 12U) & 0x3FU)));
            writeChar8(static_cast<tChar8>(0x80U | ((codePoint >> 6U) & 0x3FU)));
            writeChar8(static_cast<tChar8>(0x80U | (codePoint & 0x3FU)));
        }
    }
    /// Write a UTF-8 byte order mark.
    void writeBom() noexcept {
        writeChar8(static_cast<tChar8>(0xEFU));
        writeChar8(static_cast<tChar8>(0xBBU));
        writeChar8(static_cast<tChar8>(0xBFU));
    }

private:
    /// Write a single byte to the buffer if there is space.
    void writeChar8(const tChar8 byte) noexcept {
        if (_position >= _data.size()) {
            return;
        }
        _data[_position] = byte;
        _position += 1U;
    }

private:
    std::span<tChar8> _data;
    std::size_t _position{0};
};

/// A writer for UTF-8 encoded text into a byte writer.
/// @tested{U8WriterTest}
template <typename tWriter>
    requires(std::is_same_v<tWriter, mem::ByteWriter> || std::is_same_v<tWriter, mem::impl::RingBufferWriter>)
class U8Writer<tWriter> {
public:
    explicit constexpr U8Writer(tWriter &writer) noexcept : _writer{writer} {}

public:
    /// Get the current position in the destination buffer.
    [[nodiscard]] auto position() const noexcept -> std::size_t { return _writer.position().toSizeT(); }
    /// Write one Unicode scalar value as a UTF-8 sequence into the destination buffer.
    /// A call for invalid characters is ignored.
    /// @param character A valid Unicode code-point to write.
    [[nodiscard]] auto write(const Char character) noexcept {
        if (!character.isValidUnicode()) {
            return;
        }
        const auto codePoint = character.toRawValue();
        if (codePoint < 0x80U) {
            writeChar8(static_cast<uint8_t>(codePoint));
        } else if (codePoint < 0x800U) {
            writeChar8(static_cast<uint8_t>(0xC0U | ((codePoint >> 6U) & 0x1FU)));
            writeChar8(static_cast<uint8_t>(0x80U | (codePoint & 0x3FU)));
        } else if (codePoint < 0x10000U) {
            writeChar8(static_cast<uint8_t>(0xE0U | ((codePoint >> 12U) & 0x0FU)));
            writeChar8(static_cast<uint8_t>(0x80U | ((codePoint >> 6U) & 0x3FU)));
            writeChar8(static_cast<uint8_t>(0x80U | (codePoint & 0x3FU)));
        } else {
            writeChar8(static_cast<uint8_t>(0xF0U | ((codePoint >> 18U) & 0x07U)));
            writeChar8(static_cast<uint8_t>(0x80U | ((codePoint >> 12U) & 0x3FU)));
            writeChar8(static_cast<uint8_t>(0x80U | ((codePoint >> 6U) & 0x3FU)));
            writeChar8(static_cast<uint8_t>(0x80U | (codePoint & 0x3FU)));
        }
    }
    /// Write a UTF-8 byte order mark.
    void writeBom() noexcept {
        writeChar8(0xEFU);
        writeChar8(0xBBU);
        writeChar8(0xBFU);
    }

private:
    /// Write a single byte.
    void writeChar8(const uint8_t byte) noexcept { _writer.writeUInt8(byte); }

private:
    tWriter &_writer;
};

template <typename tChar8>
U8Writer(std::span<tChar8>) -> U8Writer<std::span<tChar8>>;

template <typename tWriter>
    requires(std::is_same_v<tWriter, mem::ByteWriter> || std::is_same_v<tWriter, mem::impl::RingBufferWriter>)
U8Writer(tWriter &) -> U8Writer<tWriter>;

/// Generic method to create a UTF-8 encoded string from UTF-8 encoded text.
/// @tested{U8WriterTest}
template <typename tString>
    requires(std::is_same_v<tString, std::string> || std::is_same_v<tString, std::u8string>)
[[nodiscard]] auto createUtf8String(const std::span<const char> data) noexcept -> tString {
    auto result = tString{};
    if (data.empty()) {
        return result;
    }
    auto reservedSize = unit::ByteLength::zero();
    utf8::forEachDecodedCharacter(data, EncodingErrorMode::Replace, [&](const Char character) -> void {
        reservedSize += utf8::encodedLength(character);
    });
    if (reservedSize.isZero()) {
        return result;
    }
    result.resize(reservedSize.toSizeT());
    U8Writer writer{std::span{result.data(), result.size()}};
    utf8::forEachDecodedCharacter(
        data, EncodingErrorMode::Replace, [&](const Char character) -> void { writer.write(character); });
    return result;
}

/// Generic method to create a UTF-8 encoded string from UTF-16 encoded text.
/// @tested{U8WriterTest}
template <typename tString>
    requires(std::is_same_v<tString, std::string> || std::is_same_v<tString, std::u8string>)
[[nodiscard]] auto createUtf8String(const std::span<const char16_t> data) noexcept -> tString {
    auto result = tString{};
    if (data.empty()) {
        return result;
    }
    auto reservedSize = unit::ByteLength::zero();
    utf16::forEachDecodedCharacter(data, EncodingErrorMode::Replace, [&](const Char character) -> void {
        reservedSize += utf8::encodedLength(character);
    });
    if (reservedSize.isZero()) {
        return result;
    }
    result.resize(reservedSize.toSizeT());
    U8Writer writer{std::span{result.data(), result.size()}};
    utf16::forEachDecodedCharacter(
        data, EncodingErrorMode::Replace, [&](const Char character) -> void { writer.write(character); });
    return result;
}

/// Generic method to create a UTF-8 encoded string from UTF-32 encoded text.
/// @tested{U8WriterTest}
template <typename tString>
    requires(std::is_same_v<tString, std::string> || std::is_same_v<tString, std::u8string>)
[[nodiscard]] auto createUtf8String(const std::span<const char32_t> data) noexcept -> tString {
    auto result = tString{};
    if (data.empty()) {
        return result;
    }
    auto reservedSize = unit::ByteLength::zero();
    utf32::forEachDecodedCharacter(data, EncodingErrorMode::Replace, [&](const Char character) -> void {
        reservedSize += utf8::encodedLength(character);
    });
    if (reservedSize.isZero()) {
        return result;
    }
    result.resize(reservedSize.toSizeT());
    U8Writer writer{std::span{result.data(), result.size()}};
    utf32::forEachDecodedCharacter(
        data, EncodingErrorMode::Replace, [&](const Char character) -> void { writer.write(character); });
    return result;
}

}
