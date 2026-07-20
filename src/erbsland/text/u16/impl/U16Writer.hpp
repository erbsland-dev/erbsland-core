// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16Encoding.hpp"

#include "../../../mem/ByteWriter.hpp"
#include "../../../mem/impl/RingBufferWriter.hpp"
#include "../../Char.hpp"
#include "../../u8/impl/U8Encoding.hpp"

#include <span>
#include <string>
#include <type_traits>

namespace erbsland::text::impl {

/// A writer for UTF-16 encoded text.
/// @tested{U16WriterTest}
template <typename tTarget>
class U16Writer {};

/// A writer for UTF-16 encoded text into a character span.
/// @tested{U16WriterTest}
template <typename tChar16>
    requires(std::is_same_v<tChar16, char16_t> || std::is_same_v<tChar16, wchar_t>)
class U16Writer<std::span<tChar16>> {
public:
    explicit constexpr U16Writer(const std::span<tChar16> data) noexcept : _data{data} {}

public:
    /// Get the current position in the destination buffer.
    [[nodiscard]] auto position() const noexcept -> std::size_t { return _position; }
    /// Write one Unicode scalar value as a UTF-16 sequence into the destination buffer.
    /// A call for invalid characters is ignored.
    /// @param character A valid Unicode code-point to write.
    [[nodiscard]] auto write(const Char character) noexcept {
        if (!character.isValidUnicode()) {
            return;
        }
        const auto codePoint = character.toRawValue();
        if (codePoint <= 0xFFFFU) {
            writeChar16(static_cast<tChar16>(codePoint));
            return;
        }
        const auto shifted = codePoint - 0x10000U;
        writeChar16(static_cast<tChar16>(0xD800U + ((shifted >> 10U) & 0x03FFU)));
        writeChar16(static_cast<tChar16>(0xDC00U + (shifted & 0x03FFU)));
    }
    /// Write a UTF-16 byte order mark.
    void writeBom() noexcept { writeChar16(static_cast<tChar16>(0xFEFFU)); }

private:
    /// Write a single char16_t to the buffer if there is space.
    void writeChar16(const tChar16 char16) noexcept {
        if (_position >= _data.size()) {
            return;
        }
        _data[_position] = char16;
        _position += 1U;
    }

private:
    std::span<tChar16> _data;
    std::size_t _position{0};
};

/// A writer for UTF-16 encoded text into a byte writer.
/// @tested{U16WriterTest}
template <typename tWriter>
    requires(std::is_same_v<tWriter, mem::ByteWriter> || std::is_same_v<tWriter, mem::impl::RingBufferWriter>)
class U16Writer<tWriter> {
public:
    explicit constexpr U16Writer(tWriter &writer) noexcept : _writer{writer} {}

public:
    /// Get the current position in the destination buffer.
    [[nodiscard]] auto position() const noexcept -> std::size_t { return _writer.position().toSizeT(); }
    /// Write one Unicode scalar value as a UTF-16 sequence into the destination buffer.
    /// A call for invalid characters is ignored.
    /// @param character A valid Unicode code-point to write.
    [[nodiscard]] auto write(const Char character) noexcept {
        if (!character.isValidUnicode()) {
            return;
        }
        const auto codePoint = character.toRawValue();
        if (codePoint <= 0xFFFFU) {
            writeChar16(static_cast<uint16_t>(codePoint));
            return;
        }
        const auto shifted = codePoint - 0x10000U;
        writeChar16(static_cast<uint16_t>(0xD800U + ((shifted >> 10U) & 0x03FFU)));
        writeChar16(static_cast<uint16_t>(0xDC00U + (shifted & 0x03FFU)));
    }
    /// Write a UTF-16 byte order mark.
    void writeBom() noexcept { writeChar16(0xFEFFU); }

private:
    /// Write a single UTF-16 code unit.
    void writeChar16(const uint16_t char16) noexcept { _writer.writeUInt16(char16); }

private:
    tWriter &_writer;
};

template <typename tChar16>
U16Writer(std::span<tChar16>) -> U16Writer<std::span<tChar16>>;

template <typename tWriter>
    requires(std::is_same_v<tWriter, mem::ByteWriter> || std::is_same_v<tWriter, mem::impl::RingBufferWriter>)
U16Writer(tWriter &) -> U16Writer<tWriter>;

/// Generic method to create a UTF-16 encoded string from UTF-8 encoded text.
/// @tested{U16WriterTest}
template <typename tString>
    requires(std::is_same_v<tString, std::u16string> || std::is_same_v<tString, std::wstring>)
[[nodiscard]] auto createUtf16String(const std::span<const char> data) noexcept -> tString {
    auto result = tString{};
    if (data.empty()) {
        return result;
    }
    auto reservedSize = unit::U16DataLength::zero();
    utf8::forEachDecodedCharacter(data, EncodingErrorMode::Replace, [&](const Char character) -> void {
        reservedSize += utf16::encodedLength(character);
    });
    if (reservedSize.isZero()) {
        return result;
    }
    result.resize(reservedSize.toSizeT());
    U16Writer writer{std::span{result.data(), result.size()}};
    utf8::forEachDecodedCharacter(
        data, EncodingErrorMode::Replace, [&](const Char character) -> void { writer.write(character); });
    return result;
}

/// Generic method to create a UTF-16 encoded string from UTF-16 encoded text.
/// @tested{U16WriterTest}
template <typename tString>
    requires(std::is_same_v<tString, std::u16string> || std::is_same_v<tString, std::wstring>)
[[nodiscard]] auto createUtf16String(const std::span<const char16_t> data) noexcept -> tString {
    auto result = tString{};
    if (data.empty()) {
        return result;
    }
    auto reservedSize = unit::U16DataLength::zero();
    utf16::forEachDecodedCharacter(data, EncodingErrorMode::Replace, [&](const Char character) -> void {
        reservedSize += utf16::encodedLength(character);
    });
    if (reservedSize.isZero()) {
        return result;
    }
    result.resize(reservedSize.toSizeT());
    U16Writer writer{std::span{result.data(), result.size()}};
    utf16::forEachDecodedCharacter(
        data, EncodingErrorMode::Replace, [&](const Char character) -> void { writer.write(character); });
    return result;
}

}
