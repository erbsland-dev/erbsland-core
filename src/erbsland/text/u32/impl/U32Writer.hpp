// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32Encoding.hpp"

#include "../../../mem/ByteWriter.hpp"
#include "../../u16/impl/U16Encoding.hpp"
#include "../../u8/impl/U8Encoding.hpp"

#include <concepts>
#include <span>
#include <string>
#include <type_traits>

namespace erbsland::text::impl {

/// A writer for UTF-32 encoded text.
/// @tested{U32WriterTest}
template <typename tTarget>
class U32Writer {};

/// A writer for UTF-32 encoded text into a character span.
/// @tested{U32WriterTest}
template <typename tChar32>
    requires(std::is_same_v<tChar32, char32_t> || std::is_same_v<tChar32, wchar_t>)
class U32Writer<std::span<tChar32>> {
public:
    explicit constexpr U32Writer(const std::span<tChar32> data) noexcept : _data{data} {}

public:
    /// Get the current position in the destination buffer.
    [[nodiscard]] auto position() const noexcept -> std::size_t { return _position; }
    /// Write one Unicode scalar value as a UTF-32 code unit into the destination buffer.
    /// A call for invalid characters is ignored.
    /// @param character A valid Unicode code-point to write.
    [[nodiscard]] auto write(const Char character) noexcept {
        if (!character.isValidUnicode()) {
            return;
        }
        writeChar32(static_cast<tChar32>(character.toRawValue()));
    }
    /// Write a UTF-32 byte order mark.
    [[nodiscard]] auto writeBom() noexcept { write(Char{0xFEFFU}); }

private:
    /// Write a single UTF-32 code unit if there is space.
    void writeChar32(const tChar32 char32) noexcept {
        if (_position >= _data.size()) {
            return;
        }
        _data[_position] = char32;
        _position += 1U;
    }

private:
    std::span<tChar32> _data;
    std::size_t _position{0};
};

/// A writer for UTF-32 encoded text into a byte writer.
/// @tested{U32WriterTest}
template <>
class U32Writer<mem::ByteWriter> {
public:
    explicit constexpr U32Writer(mem::ByteWriter &writer) noexcept : _writer{writer} {}

public:
    /// Get the current position in the destination buffer.
    [[nodiscard]] auto position() const noexcept -> std::size_t { return _writer.position().toSizeT(); }
    /// Write one Unicode scalar value as a UTF-32 code unit into the destination buffer.
    /// A call for invalid characters is ignored.
    /// @param character A valid Unicode code-point to write.
    [[nodiscard]] auto write(const Char character) noexcept {
        if (!character.isValidUnicode()) {
            return;
        }
        _writer.writeUInt32(character.toRawValue());
    }
    /// Write a UTF-32 byte order mark.
    [[nodiscard]] auto writeBom() noexcept { write(Char{0xFEFFU}); }

private:
    mem::ByteWriter &_writer;
};

template <typename tChar32>
U32Writer(std::span<tChar32>) -> U32Writer<std::span<tChar32>>;

U32Writer(mem::ByteWriter &) -> U32Writer<mem::ByteWriter>;

/// Create a UTF-32 encoded string from UTF-8 encoded data.
/// @tested{U32WriterTest}
template <typename tString>
    requires(std::is_same_v<tString, std::u32string> || std::is_same_v<tString, std::wstring>)
[[nodiscard]] auto createUtf32String(const std::span<const char> data) noexcept -> tString {
    auto result = tString{};
    if (data.empty()) {
        return result;
    }
    result.reserve(data.size());
    utf8::forEachDecodedCharacter(data, EncodingErrorMode::Replace, [&](const Char character) -> void {
        result.push_back(static_cast<tString::value_type>(character.toRawValue()));
    });
    return result;
}

/// Create a UTF-32 encoded string from UTF-16 encoded data.
/// @tested{U32WriterTest}
template <typename tString>
    requires(std::is_same_v<tString, std::u32string> || std::is_same_v<tString, std::wstring>)
[[nodiscard]] auto createUtf32String(const std::span<const char16_t> data) noexcept -> tString {
    auto result = tString{};
    if (data.empty()) {
        return result;
    }
    result.reserve(data.size());
    utf16::forEachDecodedCharacter(data, EncodingErrorMode::Replace, [&](const Char character) -> void {
        result.push_back(static_cast<tString::value_type>(character.toRawValue()));
    });
    return result;
}

/// Create a UTF-32 encoded string from UTF-32 encoded data.
/// @tested{U32WriterTest}
template <typename tString>
    requires(std::is_same_v<tString, std::u32string> || std::is_same_v<tString, std::wstring>)
[[nodiscard]] auto createUtf32String(const std::span<const char32_t> data) noexcept -> tString {
    auto result = tString{};
    if (data.empty()) {
        return result;
    }
    result.reserve(data.size());
    utf32::forEachDecodedCharacter(data, EncodingErrorMode::Replace, [&](const Char character) -> void {
        result.push_back(static_cast<tString::value_type>(character.toRawValue()));
    });
    return result;
}

}
