// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringConversionTools_fwd.hpp"

#include "../EncodingMode.hpp"
#include "../StringBomMode.hpp"
#include "../StringEncoding.hpp"
#include "../u16/U16String_fwd.hpp"
#include "../u16/U16StringEditor_fwd.hpp"
#include "../u32/U32String_fwd.hpp"
#include "../u32/U32StringEditor_fwd.hpp"
#include "../u8/U8String_fwd.hpp"
#include "../u8/U8StringEditor_fwd.hpp"

#include "../../mem/ByteBlock_fwd.hpp"
#include "../../mem/ByteSpan.hpp"
#include "../../mem/RingBuffer_fwd.hpp"
#include "../../unit/ByteLength_fwd.hpp"
#include "../../util/Result.hpp"

#include <span>
#include <string>
#include <string_view>

namespace erbsland::text::impl {

/// Shared implementation for string conversion entry points.
/// @tested{StringConverterTest StringEncoderTest StringDecoderTest}
class StringConversionTools final {
public:
    /// Converts a standard string to an editable UTF-8 string.
    [[nodiscard]] static auto toU8StringEditor(std::string_view str, EncodingMode mode) -> U8StringEditor;
    /// Converts a standard UTF-8 string to an editable UTF-8 string.
    [[nodiscard]] static auto toU8StringEditor(std::u8string_view str, EncodingMode mode) -> U8StringEditor;
    /// Converts a standard UTF-16 string to an editable UTF-8 string.
    [[nodiscard]] static auto toU8StringEditor(std::u16string_view str, EncodingMode mode) -> U8StringEditor;
    /// Converts a standard UTF-32 string to an editable UTF-8 string.
    [[nodiscard]] static auto toU8StringEditor(std::u32string_view str, EncodingMode mode) -> U8StringEditor;
    /// Converts a standard wide string to an editable UTF-8 string.
    [[nodiscard]] static auto toU8StringEditor(std::wstring_view str, EncodingMode mode) -> U8StringEditor;
    /// Converts an editable UTF-8 string to an editable UTF-8 string.
    [[nodiscard]] static auto toU8StringEditor(const U8StringEditor &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U8StringEditor;
    /// Converts a UTF-8 string to an editable UTF-8 string.
    [[nodiscard]] static auto toU8StringEditor(const U8String &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U8String;
    /// Converts an editable UTF-16 string to an editable UTF-8 string.
    [[nodiscard]] static auto toU8StringEditor(const U16StringEditor &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U8StringEditor;
    /// Converts a UTF-16 string to an editable UTF-8 string.
    [[nodiscard]] static auto toU8StringEditor(const U16String &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U8StringEditor;
    /// Converts an editable UTF-32 string to an editable UTF-8 string.
    [[nodiscard]] static auto toU8StringEditor(const U32StringEditor &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U8StringEditor;
    /// Converts a UTF-32 string to an editable UTF-8 string.
    [[nodiscard]] static auto toU8StringEditor(const U32String &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U8StringEditor;

    /// Converts a standard string to an editable UTF-16 string.
    [[nodiscard]] static auto toU16StringEditor(std::string_view str, EncodingMode mode) -> U16StringEditor;
    /// Converts a standard UTF-8 string to an editable UTF-16 string.
    [[nodiscard]] static auto toU16StringEditor(std::u8string_view str, EncodingMode mode) -> U16StringEditor;
    /// Converts a standard UTF-16 string to an editable UTF-16 string.
    [[nodiscard]] static auto toU16StringEditor(std::u16string_view str, EncodingMode mode) -> U16StringEditor;
    /// Converts a standard UTF-32 string to an editable UTF-16 string.
    [[nodiscard]] static auto toU16StringEditor(std::u32string_view str, EncodingMode mode) -> U16StringEditor;
    /// Converts a standard wide string to an editable UTF-16 string.
    [[nodiscard]] static auto toU16StringEditor(std::wstring_view str, EncodingMode mode) -> U16StringEditor;
    /// Converts an editable UTF-8 string to an editable UTF-16 string.
    [[nodiscard]] static auto toU16StringEditor(const U8StringEditor &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U16StringEditor;
    /// Converts a UTF-8 string to an editable UTF-16 string.
    [[nodiscard]] static auto toU16StringEditor(const U8String &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U16StringEditor;
    /// Converts an editable UTF-16 string to an editable UTF-16 string.
    [[nodiscard]] static auto toU16StringEditor(const U16StringEditor &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U16StringEditor;
    /// Converts a UTF-16 string to an editable UTF-16 string.
    [[nodiscard]] static auto toU16StringEditor(const U16String &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U16String;
    /// Converts an editable UTF-32 string to an editable UTF-16 string.
    [[nodiscard]] static auto toU16StringEditor(const U32StringEditor &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U16StringEditor;
    /// Converts a UTF-32 string to an editable UTF-16 string.
    [[nodiscard]] static auto toU16StringEditor(const U32String &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U16StringEditor;

    /// Converts a standard string to an editable UTF-32 string.
    [[nodiscard]] static auto toU32StringEditor(std::string_view str, EncodingMode mode) -> U32StringEditor;
    /// Converts a standard UTF-8 string to an editable UTF-32 string.
    [[nodiscard]] static auto toU32StringEditor(std::u8string_view str, EncodingMode mode) -> U32StringEditor;
    /// Converts a standard UTF-16 string to an editable UTF-32 string.
    [[nodiscard]] static auto toU32StringEditor(std::u16string_view str, EncodingMode mode) -> U32StringEditor;
    /// Converts a standard UTF-32 string to an editable UTF-32 string.
    [[nodiscard]] static auto toU32StringEditor(std::u32string_view str, EncodingMode mode) -> U32StringEditor;
    /// Converts a standard wide string to an editable UTF-32 string.
    [[nodiscard]] static auto toU32StringEditor(std::wstring_view str, EncodingMode mode) -> U32StringEditor;
    /// Converts an editable UTF-8 string to an editable UTF-32 string.
    [[nodiscard]] static auto toU32StringEditor(const U8StringEditor &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U32StringEditor;
    /// Converts a UTF-8 string to an editable UTF-32 string.
    [[nodiscard]] static auto toU32StringEditor(const U8String &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U32StringEditor;
    /// Converts an editable UTF-16 string to an editable UTF-32 string.
    [[nodiscard]] static auto toU32StringEditor(const U16StringEditor &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U32StringEditor;
    /// Converts a UTF-16 string to an editable UTF-32 string.
    [[nodiscard]] static auto toU32StringEditor(const U16String &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U32StringEditor;
    /// Converts an editable UTF-32 string to an editable UTF-32 string.
    [[nodiscard]] static auto toU32StringEditor(const U32StringEditor &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U32StringEditor;
    /// Converts a UTF-32 string to an editable UTF-32 string.
    [[nodiscard]] static auto toU32StringEditor(const U32String &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U32String;

    /// Converts an editable UTF-8 string to a standard string.
    [[nodiscard]] static auto toStdString(const U8StringEditor &str) -> std::string;
    /// Converts a UTF-8 string to a standard string.
    [[nodiscard]] static auto toStdString(const U8String &str) -> std::string;
    /// Converts an editable UTF-16 string to a standard string.
    [[nodiscard]] static auto toStdString(const U16StringEditor &str) -> std::string;
    /// Converts a UTF-16 string to a standard string.
    [[nodiscard]] static auto toStdString(const U16String &str) -> std::string;
    /// Converts an editable UTF-32 string to a standard string.
    [[nodiscard]] static auto toStdString(const U32StringEditor &str) -> std::string;
    /// Converts a UTF-32 string to a standard string.
    [[nodiscard]] static auto toStdString(const U32String &str) -> std::string;

    /// Converts an editable UTF-8 string to a standard UTF-8 string.
    [[nodiscard]] static auto toStdU8String(const U8StringEditor &str) -> std::u8string;
    /// Converts a UTF-8 string to a standard UTF-8 string.
    [[nodiscard]] static auto toStdU8String(const U8String &str) -> std::u8string;
    /// Converts an editable UTF-16 string to a standard UTF-8 string.
    [[nodiscard]] static auto toStdU8String(const U16StringEditor &str) -> std::u8string;
    /// Converts a UTF-16 string to a standard UTF-8 string.
    [[nodiscard]] static auto toStdU8String(const U16String &str) -> std::u8string;
    /// Converts an editable UTF-32 string to a standard UTF-8 string.
    [[nodiscard]] static auto toStdU8String(const U32StringEditor &str) -> std::u8string;
    /// Converts a UTF-32 string to a standard UTF-8 string.
    [[nodiscard]] static auto toStdU8String(const U32String &str) -> std::u8string;

    /// Converts an editable UTF-8 string to a standard UTF-16 string.
    [[nodiscard]] static auto toStdU16String(const U8StringEditor &str) -> std::u16string;
    /// Converts a UTF-8 string to a standard UTF-16 string.
    [[nodiscard]] static auto toStdU16String(const U8String &str) -> std::u16string;
    /// Converts an editable UTF-16 string to a standard UTF-16 string.
    [[nodiscard]] static auto toStdU16String(const U16StringEditor &str) -> std::u16string;
    /// Converts a UTF-16 string to a standard UTF-16 string.
    [[nodiscard]] static auto toStdU16String(const U16String &str) -> std::u16string;
    /// Converts an editable UTF-32 string to a standard UTF-16 string.
    [[nodiscard]] static auto toStdU16String(const U32StringEditor &str) -> std::u16string;
    /// Converts a UTF-32 string to a standard UTF-16 string.
    [[nodiscard]] static auto toStdU16String(const U32String &str) -> std::u16string;

    /// Converts an editable UTF-8 string to a standard UTF-32 string.
    [[nodiscard]] static auto toStdU32String(const U8StringEditor &str) -> std::u32string;
    /// Converts a UTF-8 string to a standard UTF-32 string.
    [[nodiscard]] static auto toStdU32String(const U8String &str) -> std::u32string;
    /// Converts an editable UTF-16 string to a standard UTF-32 string.
    [[nodiscard]] static auto toStdU32String(const U16StringEditor &str) -> std::u32string;
    /// Converts a UTF-16 string to a standard UTF-32 string.
    [[nodiscard]] static auto toStdU32String(const U16String &str) -> std::u32string;
    /// Converts an editable UTF-32 string to a standard UTF-32 string.
    [[nodiscard]] static auto toStdU32String(const U32StringEditor &str) -> std::u32string;
    /// Converts a UTF-32 string to a standard UTF-32 string.
    [[nodiscard]] static auto toStdU32String(const U32String &str) -> std::u32string;

    /// Converts an editable UTF-8 string to a standard wide string.
    [[nodiscard]] static auto toStdWString(const U8StringEditor &str) -> std::wstring;
    /// Converts a UTF-8 string to a standard wide string.
    [[nodiscard]] static auto toStdWString(const U8String &str) -> std::wstring;
    /// Converts an editable UTF-16 string to a standard wide string.
    [[nodiscard]] static auto toStdWString(const U16StringEditor &str) -> std::wstring;
    /// Converts a UTF-16 string to a standard wide string.
    [[nodiscard]] static auto toStdWString(const U16String &str) -> std::wstring;
    /// Converts an editable UTF-32 string to a standard wide string.
    [[nodiscard]] static auto toStdWString(const U32StringEditor &str) -> std::wstring;
    /// Converts a UTF-32 string to a standard wide string.
    [[nodiscard]] static auto toStdWString(const U32String &str) -> std::wstring;

    /// Encodes an editable UTF-8 string into a byte block.
    [[nodiscard]] static auto encode(const U8StringEditor &str, StringEncoding encoding, StringBomMode bomMode)
        -> mem::ByteBlock;
    /// Encodes a UTF-8 string into a byte block.
    [[nodiscard]] static auto encode(const U8String &str, StringEncoding encoding, StringBomMode bomMode)
        -> mem::ByteBlock;
    /// Encodes an editable UTF-16 string into a byte block.
    [[nodiscard]] static auto encode(const U16StringEditor &str, StringEncoding encoding, StringBomMode bomMode)
        -> mem::ByteBlock;
    /// Encodes a UTF-16 string into a byte block.
    [[nodiscard]] static auto encode(const U16String &str, StringEncoding encoding, StringBomMode bomMode)
        -> mem::ByteBlock;
    /// Encodes an editable UTF-32 string into a byte block.
    [[nodiscard]] static auto encode(const U32StringEditor &str, StringEncoding encoding, StringBomMode bomMode)
        -> mem::ByteBlock;
    /// Encodes a UTF-32 string into a byte block.
    [[nodiscard]] static auto encode(const U32String &str, StringEncoding encoding, StringBomMode bomMode)
        -> mem::ByteBlock;

    /// Gets the encoded length of an editable UTF-8 string.
    [[nodiscard]] static auto encodedLength(const U8StringEditor &str, StringEncoding encoding, StringBomMode bomMode)
        -> unit::ByteLength;
    /// Gets the encoded length of a UTF-8 string.
    [[nodiscard]] static auto encodedLength(const U8String &str, StringEncoding encoding, StringBomMode bomMode)
        -> unit::ByteLength;
    /// Gets the encoded length of an editable UTF-16 string.
    [[nodiscard]] static auto encodedLength(const U16StringEditor &str, StringEncoding encoding, StringBomMode bomMode)
        -> unit::ByteLength;
    /// Gets the encoded length of a UTF-16 string.
    [[nodiscard]] static auto encodedLength(const U16String &str, StringEncoding encoding, StringBomMode bomMode)
        -> unit::ByteLength;
    /// Gets the encoded length of an editable UTF-32 string.
    [[nodiscard]] static auto encodedLength(const U32StringEditor &str, StringEncoding encoding, StringBomMode bomMode)
        -> unit::ByteLength;
    /// Gets the encoded length of a UTF-32 string.
    [[nodiscard]] static auto encodedLength(const U32String &str, StringEncoding encoding, StringBomMode bomMode)
        -> unit::ByteLength;

    /// Encodes an editable UTF-8 string into a ring buffer.
    [[nodiscard]] static auto encodeTo(
        const U8StringEditor &str, mem::RingBuffer &buffer, StringEncoding encoding, StringBomMode bomMode)
        -> util::Result;
    /// Encodes a UTF-8 string into a ring buffer.
    [[nodiscard]] static auto encodeTo(
        const U8String &str, mem::RingBuffer &buffer, StringEncoding encoding, StringBomMode bomMode) -> util::Result;
    /// Encodes an editable UTF-16 string into a ring buffer.
    [[nodiscard]] static auto encodeTo(
        const U16StringEditor &str, mem::RingBuffer &buffer, StringEncoding encoding, StringBomMode bomMode)
        -> util::Result;
    /// Encodes a UTF-16 string into a ring buffer.
    [[nodiscard]] static auto encodeTo(
        const U16String &str, mem::RingBuffer &buffer, StringEncoding encoding, StringBomMode bomMode) -> util::Result;
    /// Encodes an editable UTF-32 string into a ring buffer.
    [[nodiscard]] static auto encodeTo(
        const U32StringEditor &str, mem::RingBuffer &buffer, StringEncoding encoding, StringBomMode bomMode)
        -> util::Result;
    /// Encodes a UTF-32 string into a ring buffer.
    [[nodiscard]] static auto encodeTo(
        const U32String &str, mem::RingBuffer &buffer, StringEncoding encoding, StringBomMode bomMode) -> util::Result;

    /// Decodes a byte block into an editable UTF-8 string.
    [[nodiscard]] static auto decodeU8String(
        const mem::ByteBlock &data, StringEncoding encoding, StringBomMode bomMode, EncodingMode mode)
        -> U8StringEditor;
    /// Decodes a byte block into an editable UTF-16 string.
    [[nodiscard]] static auto decodeU16String(
        const mem::ByteBlock &data, StringEncoding encoding, StringBomMode bomMode, EncodingMode mode)
        -> U16StringEditor;
    /// Decodes a byte block into an editable UTF-32 string.
    [[nodiscard]] static auto decodeU32String(
        const mem::ByteBlock &data, StringEncoding encoding, StringBomMode bomMode, EncodingMode mode)
        -> U32StringEditor;
    /// Strictly validate encoded byte data without constructing a string.
    static void validateEncodedData(const mem::ByteBlock &data, StringEncoding encoding, StringBomMode bomMode);

private:
    /// Tests whether the encoding matches the native representation of a character type.
    template <typename T>
    [[nodiscard]] static auto isMatchingRepresentation(StringEncoding encoding) noexcept -> bool;
    /// Views a character span as a byte span.
    template <typename T>
    [[nodiscard]] static auto asByteSpan(std::span<const T> source) noexcept -> mem::ConstByteSpan;
    /// Calculates the length of directly encoded character data.
    template <typename T>
    [[nodiscard]] static auto rawEncodedLength(
        std::span<const T> source, StringEncoding encoding, StringBomMode bomMode) -> unit::ByteLength;
    /// Copies source bytes into the destination at the current offset.
    static void copyBytes(mem::ByteSpan destination, std::size_t &offset, mem::ConstByteSpan source) noexcept;
    /// Directly encodes character data with a matching representation.
    template <typename T>
    [[nodiscard]] static auto encodeRaw(std::span<const T> source, StringEncoding encoding, StringBomMode bomMode)
        -> mem::ByteBlock;
    /// Directly encodes character data with a matching representation into a ring buffer.
    template <typename T>
    [[nodiscard]] static auto encodeRawTo(
        std::span<const T> source, mem::RingBuffer &buffer, StringEncoding encoding, StringBomMode bomMode)
        -> util::Result;
    /// Encodes a string using direct copying or character conversion as appropriate.
    template <typename tData, typename tTools, typename tDataView>
    [[nodiscard]] static auto encodeString(tDataView dataView, StringEncoding encoding, StringBomMode bomMode)
        -> mem::ByteBlock;
    /// Calculates an encoded string length using direct copying or character conversion as appropriate.
    template <typename tData, typename tTools, typename tDataView>
    [[nodiscard]] static auto encodedStringLength(tDataView dataView, StringEncoding encoding, StringBomMode bomMode)
        -> unit::ByteLength;
    /// Encodes a string into a ring buffer using direct copying or character conversion as appropriate.
    template <typename tData, typename tTools, typename tDataView>
    [[nodiscard]] static auto encodeStringTo(
        tDataView dataView, mem::RingBuffer &buffer, StringEncoding encoding, StringBomMode bomMode) -> util::Result;
};

}
