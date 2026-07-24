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
#include "../../mem/RingBuffer_fwd.hpp"
#include "../../unit/ByteLength_fwd.hpp"
#include "../../util/Result.hpp"

#include <string>
#include <string_view>

namespace erbsland::text::impl {

/// Shared implementation for string conversion entry points.
/// @tested{StringConverterTest StringEncoderTest StringDecoderTest}
class StringConversionTools final {
public:
    [[nodiscard]] static auto toU8StringEditor(std::string_view str, EncodingMode mode) -> U8StringEditor;
    [[nodiscard]] static auto toU8StringEditor(std::u8string_view str, EncodingMode mode) -> U8StringEditor;
    [[nodiscard]] static auto toU8StringEditor(std::u16string_view str, EncodingMode mode) -> U8StringEditor;
    [[nodiscard]] static auto toU8StringEditor(std::u32string_view str, EncodingMode mode) -> U8StringEditor;
    [[nodiscard]] static auto toU8StringEditor(std::wstring_view str, EncodingMode mode) -> U8StringEditor;
    [[nodiscard]] static auto toU8StringEditor(const U8StringEditor &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U8StringEditor;
    [[nodiscard]] static auto toU8StringEditor(const U8String &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U8String;
    [[nodiscard]] static auto toU8StringEditor(const U16StringEditor &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U8StringEditor;
    [[nodiscard]] static auto toU8StringEditor(const U16String &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U8StringEditor;
    [[nodiscard]] static auto toU8StringEditor(const U32StringEditor &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U8StringEditor;
    [[nodiscard]] static auto toU8StringEditor(const U32String &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U8StringEditor;

    [[nodiscard]] static auto toU16StringEditor(std::string_view str, EncodingMode mode) -> U16StringEditor;
    [[nodiscard]] static auto toU16StringEditor(std::u8string_view str, EncodingMode mode) -> U16StringEditor;
    [[nodiscard]] static auto toU16StringEditor(std::u16string_view str, EncodingMode mode) -> U16StringEditor;
    [[nodiscard]] static auto toU16StringEditor(std::u32string_view str, EncodingMode mode) -> U16StringEditor;
    [[nodiscard]] static auto toU16StringEditor(std::wstring_view str, EncodingMode mode) -> U16StringEditor;
    [[nodiscard]] static auto toU16StringEditor(const U8StringEditor &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U16StringEditor;
    [[nodiscard]] static auto toU16StringEditor(const U8String &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U16StringEditor;
    [[nodiscard]] static auto toU16StringEditor(const U16StringEditor &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U16StringEditor;
    [[nodiscard]] static auto toU16StringEditor(const U16String &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U16String;
    [[nodiscard]] static auto toU16StringEditor(const U32StringEditor &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U16StringEditor;
    [[nodiscard]] static auto toU16StringEditor(const U32String &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U16StringEditor;

    [[nodiscard]] static auto toU32StringEditor(std::string_view str, EncodingMode mode) -> U32StringEditor;
    [[nodiscard]] static auto toU32StringEditor(std::u8string_view str, EncodingMode mode) -> U32StringEditor;
    [[nodiscard]] static auto toU32StringEditor(std::u16string_view str, EncodingMode mode) -> U32StringEditor;
    [[nodiscard]] static auto toU32StringEditor(std::u32string_view str, EncodingMode mode) -> U32StringEditor;
    [[nodiscard]] static auto toU32StringEditor(std::wstring_view str, EncodingMode mode) -> U32StringEditor;
    [[nodiscard]] static auto toU32StringEditor(const U8StringEditor &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U32StringEditor;
    [[nodiscard]] static auto toU32StringEditor(const U8String &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U32StringEditor;
    [[nodiscard]] static auto toU32StringEditor(const U16StringEditor &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U32StringEditor;
    [[nodiscard]] static auto toU32StringEditor(const U16String &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U32StringEditor;
    [[nodiscard]] static auto toU32StringEditor(const U32StringEditor &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U32StringEditor;
    [[nodiscard]] static auto toU32StringEditor(const U32String &str, EncodingMode mode = EncodingMode::Tolerant)
        -> U32String;

    [[nodiscard]] static auto toStdString(const U8StringEditor &str) -> std::string;
    [[nodiscard]] static auto toStdString(const U8String &str) -> std::string;
    [[nodiscard]] static auto toStdString(const U16StringEditor &str) -> std::string;
    [[nodiscard]] static auto toStdString(const U16String &str) -> std::string;
    [[nodiscard]] static auto toStdString(const U32StringEditor &str) -> std::string;
    [[nodiscard]] static auto toStdString(const U32String &str) -> std::string;

    [[nodiscard]] static auto toStdU8String(const U8StringEditor &str) -> std::u8string;
    [[nodiscard]] static auto toStdU8String(const U8String &str) -> std::u8string;
    [[nodiscard]] static auto toStdU8String(const U16StringEditor &str) -> std::u8string;
    [[nodiscard]] static auto toStdU8String(const U16String &str) -> std::u8string;
    [[nodiscard]] static auto toStdU8String(const U32StringEditor &str) -> std::u8string;
    [[nodiscard]] static auto toStdU8String(const U32String &str) -> std::u8string;

    [[nodiscard]] static auto toStdU16String(const U8StringEditor &str) -> std::u16string;
    [[nodiscard]] static auto toStdU16String(const U8String &str) -> std::u16string;
    [[nodiscard]] static auto toStdU16String(const U16StringEditor &str) -> std::u16string;
    [[nodiscard]] static auto toStdU16String(const U16String &str) -> std::u16string;
    [[nodiscard]] static auto toStdU16String(const U32StringEditor &str) -> std::u16string;
    [[nodiscard]] static auto toStdU16String(const U32String &str) -> std::u16string;

    [[nodiscard]] static auto toStdU32String(const U8StringEditor &str) -> std::u32string;
    [[nodiscard]] static auto toStdU32String(const U8String &str) -> std::u32string;
    [[nodiscard]] static auto toStdU32String(const U16StringEditor &str) -> std::u32string;
    [[nodiscard]] static auto toStdU32String(const U16String &str) -> std::u32string;
    [[nodiscard]] static auto toStdU32String(const U32StringEditor &str) -> std::u32string;
    [[nodiscard]] static auto toStdU32String(const U32String &str) -> std::u32string;

    [[nodiscard]] static auto toStdWString(const U8StringEditor &str) -> std::wstring;
    [[nodiscard]] static auto toStdWString(const U8String &str) -> std::wstring;
    [[nodiscard]] static auto toStdWString(const U16StringEditor &str) -> std::wstring;
    [[nodiscard]] static auto toStdWString(const U16String &str) -> std::wstring;
    [[nodiscard]] static auto toStdWString(const U32StringEditor &str) -> std::wstring;
    [[nodiscard]] static auto toStdWString(const U32String &str) -> std::wstring;

    [[nodiscard]] static auto encode(const U8StringEditor &str, StringEncoding encoding, StringBomMode bomMode)
        -> mem::ByteBlock;
    [[nodiscard]] static auto encode(const U8String &str, StringEncoding encoding, StringBomMode bomMode)
        -> mem::ByteBlock;
    [[nodiscard]] static auto encode(const U16StringEditor &str, StringEncoding encoding, StringBomMode bomMode)
        -> mem::ByteBlock;
    [[nodiscard]] static auto encode(const U16String &str, StringEncoding encoding, StringBomMode bomMode)
        -> mem::ByteBlock;
    [[nodiscard]] static auto encode(const U32StringEditor &str, StringEncoding encoding, StringBomMode bomMode)
        -> mem::ByteBlock;
    [[nodiscard]] static auto encode(const U32String &str, StringEncoding encoding, StringBomMode bomMode)
        -> mem::ByteBlock;

    [[nodiscard]] static auto encodedLength(const U8StringEditor &str, StringEncoding encoding, StringBomMode bomMode)
        -> unit::ByteLength;
    [[nodiscard]] static auto encodedLength(const U8String &str, StringEncoding encoding, StringBomMode bomMode)
        -> unit::ByteLength;
    [[nodiscard]] static auto encodedLength(const U16StringEditor &str, StringEncoding encoding, StringBomMode bomMode)
        -> unit::ByteLength;
    [[nodiscard]] static auto encodedLength(const U16String &str, StringEncoding encoding, StringBomMode bomMode)
        -> unit::ByteLength;
    [[nodiscard]] static auto encodedLength(const U32StringEditor &str, StringEncoding encoding, StringBomMode bomMode)
        -> unit::ByteLength;
    [[nodiscard]] static auto encodedLength(const U32String &str, StringEncoding encoding, StringBomMode bomMode)
        -> unit::ByteLength;

    [[nodiscard]] static auto encodeTo(
        const U8StringEditor &str, mem::RingBuffer &buffer, StringEncoding encoding, StringBomMode bomMode)
        -> util::Result;
    [[nodiscard]] static auto encodeTo(
        const U8String &str, mem::RingBuffer &buffer, StringEncoding encoding, StringBomMode bomMode) -> util::Result;
    [[nodiscard]] static auto encodeTo(
        const U16StringEditor &str, mem::RingBuffer &buffer, StringEncoding encoding, StringBomMode bomMode)
        -> util::Result;
    [[nodiscard]] static auto encodeTo(
        const U16String &str, mem::RingBuffer &buffer, StringEncoding encoding, StringBomMode bomMode) -> util::Result;
    [[nodiscard]] static auto encodeTo(
        const U32StringEditor &str, mem::RingBuffer &buffer, StringEncoding encoding, StringBomMode bomMode)
        -> util::Result;
    [[nodiscard]] static auto encodeTo(
        const U32String &str, mem::RingBuffer &buffer, StringEncoding encoding, StringBomMode bomMode) -> util::Result;

    [[nodiscard]] static auto decodeU8String(
        const mem::ByteBlock &data, StringEncoding encoding, StringBomMode bomMode, EncodingMode mode)
        -> U8StringEditor;
    [[nodiscard]] static auto decodeU16String(
        const mem::ByteBlock &data, StringEncoding encoding, StringBomMode bomMode, EncodingMode mode)
        -> U16StringEditor;
    [[nodiscard]] static auto decodeU32String(
        const mem::ByteBlock &data, StringEncoding encoding, StringBomMode bomMode, EncodingMode mode)
        -> U32StringEditor;
};

}
