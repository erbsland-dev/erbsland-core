// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringConversionTools_fwd.hpp"

#include "../EncodingErrorMode.hpp"
#include "../StringBomMode.hpp"
#include "../StringEncoding.hpp"
#include "../u16/U16String_fwd.hpp"
#include "../u16/U16StringCharView_fwd.hpp"
#include "../u16/U16StringView_fwd.hpp"
#include "../u32/U32String_fwd.hpp"
#include "../u32/U32StringView_fwd.hpp"
#include "../u8/U8String_fwd.hpp"
#include "../u8/U8StringCharView_fwd.hpp"
#include "../u8/U8StringView_fwd.hpp"

#include "../../mem/ByteBlock_fwd.hpp"
#include "../../mem/ByteBlockView_fwd.hpp"

#include <string>
#include <string_view>

namespace erbsland::text::impl {

/// Shared implementation for string conversion entry points.
/// @tested{StringConverterTest StringEncoderTest StringDecoderTest}
class StringConversionTools final {
public:
    [[nodiscard]] static auto toU8String(std::string_view str, EncodingErrorMode errorMode) -> U8String;
    [[nodiscard]] static auto toU8String(std::u8string_view str, EncodingErrorMode errorMode) -> U8String;
    [[nodiscard]] static auto toU8String(std::u16string_view str, EncodingErrorMode errorMode) -> U8String;
    [[nodiscard]] static auto toU8String(std::u32string_view str, EncodingErrorMode errorMode) -> U8String;
    [[nodiscard]] static auto toU8String(std::wstring_view str, EncodingErrorMode errorMode) -> U8String;
    [[nodiscard]] static auto toU8String(const U8String &str, EncodingErrorMode errorMode) -> U8String;
    [[nodiscard]] static auto toU8String(const U8StringView &str, EncodingErrorMode errorMode) -> U8String;
    [[nodiscard]] static auto toU8String(const U16String &str, EncodingErrorMode errorMode) -> U8String;
    [[nodiscard]] static auto toU8String(const U16StringView &str, EncodingErrorMode errorMode) -> U8String;
    [[nodiscard]] static auto toU8String(const U32String &str, EncodingErrorMode errorMode) -> U8String;
    [[nodiscard]] static auto toU8String(const U32StringView &str, EncodingErrorMode errorMode) -> U8String;

    [[nodiscard]] static auto toU16String(std::string_view str, EncodingErrorMode errorMode) -> U16String;
    [[nodiscard]] static auto toU16String(std::u8string_view str, EncodingErrorMode errorMode) -> U16String;
    [[nodiscard]] static auto toU16String(std::u16string_view str, EncodingErrorMode errorMode) -> U16String;
    [[nodiscard]] static auto toU16String(std::u32string_view str, EncodingErrorMode errorMode) -> U16String;
    [[nodiscard]] static auto toU16String(std::wstring_view str, EncodingErrorMode errorMode) -> U16String;
    [[nodiscard]] static auto toU16String(const U8String &str, EncodingErrorMode errorMode) -> U16String;
    [[nodiscard]] static auto toU16String(const U8StringView &str, EncodingErrorMode errorMode) -> U16String;
    [[nodiscard]] static auto toU16String(const U16String &str, EncodingErrorMode errorMode) -> U16String;
    [[nodiscard]] static auto toU16String(const U16StringView &str, EncodingErrorMode errorMode) -> U16String;
    [[nodiscard]] static auto toU16String(const U32String &str, EncodingErrorMode errorMode) -> U16String;
    [[nodiscard]] static auto toU16String(const U32StringView &str, EncodingErrorMode errorMode) -> U16String;

    [[nodiscard]] static auto toU32String(std::string_view str, EncodingErrorMode errorMode) -> U32String;
    [[nodiscard]] static auto toU32String(std::u8string_view str, EncodingErrorMode errorMode) -> U32String;
    [[nodiscard]] static auto toU32String(std::u16string_view str, EncodingErrorMode errorMode) -> U32String;
    [[nodiscard]] static auto toU32String(std::u32string_view str, EncodingErrorMode errorMode) -> U32String;
    [[nodiscard]] static auto toU32String(std::wstring_view str, EncodingErrorMode errorMode) -> U32String;
    [[nodiscard]] static auto toU32String(const U8String &str, EncodingErrorMode errorMode) -> U32String;
    [[nodiscard]] static auto toU32String(const U8StringView &str, EncodingErrorMode errorMode) -> U32String;
    [[nodiscard]] static auto toU32String(const U16String &str, EncodingErrorMode errorMode) -> U32String;
    [[nodiscard]] static auto toU32String(const U16StringView &str, EncodingErrorMode errorMode) -> U32String;
    [[nodiscard]] static auto toU32String(const U32String &str, EncodingErrorMode errorMode) -> U32String;
    [[nodiscard]] static auto toU32String(const U32StringView &str, EncodingErrorMode errorMode) -> U32String;

    [[nodiscard]] static auto toStdString(const U8String &str, EncodingErrorMode errorMode) -> std::string;
    [[nodiscard]] static auto toStdString(const U8StringView &str, EncodingErrorMode errorMode) -> std::string;
    [[nodiscard]] static auto toStdString(const U16String &str, EncodingErrorMode errorMode) -> std::string;
    [[nodiscard]] static auto toStdString(const U16StringView &str, EncodingErrorMode errorMode) -> std::string;
    [[nodiscard]] static auto toStdString(const U32String &str, EncodingErrorMode errorMode) -> std::string;
    [[nodiscard]] static auto toStdString(const U32StringView &str, EncodingErrorMode errorMode) -> std::string;

    [[nodiscard]] static auto toStdU8String(const U8String &str, EncodingErrorMode errorMode) -> std::u8string;
    [[nodiscard]] static auto toStdU8String(const U8StringView &str, EncodingErrorMode errorMode) -> std::u8string;
    [[nodiscard]] static auto toStdU8String(const U16String &str, EncodingErrorMode errorMode) -> std::u8string;
    [[nodiscard]] static auto toStdU8String(const U16StringView &str, EncodingErrorMode errorMode) -> std::u8string;
    [[nodiscard]] static auto toStdU8String(const U32String &str, EncodingErrorMode errorMode) -> std::u8string;
    [[nodiscard]] static auto toStdU8String(const U32StringView &str, EncodingErrorMode errorMode) -> std::u8string;

    [[nodiscard]] static auto toStdU16String(const U8String &str, EncodingErrorMode errorMode) -> std::u16string;
    [[nodiscard]] static auto toStdU16String(const U8StringView &str, EncodingErrorMode errorMode) -> std::u16string;
    [[nodiscard]] static auto toStdU16String(const U16String &str, EncodingErrorMode errorMode) -> std::u16string;
    [[nodiscard]] static auto toStdU16String(const U16StringView &str, EncodingErrorMode errorMode) -> std::u16string;
    [[nodiscard]] static auto toStdU16String(const U32String &str, EncodingErrorMode errorMode) -> std::u16string;
    [[nodiscard]] static auto toStdU16String(const U32StringView &str, EncodingErrorMode errorMode) -> std::u16string;

    [[nodiscard]] static auto toStdU32String(const U8String &str, EncodingErrorMode errorMode) -> std::u32string;
    [[nodiscard]] static auto toStdU32String(const U8StringView &str, EncodingErrorMode errorMode) -> std::u32string;
    [[nodiscard]] static auto toStdU32String(const U16String &str, EncodingErrorMode errorMode) -> std::u32string;
    [[nodiscard]] static auto toStdU32String(const U16StringView &str, EncodingErrorMode errorMode) -> std::u32string;
    [[nodiscard]] static auto toStdU32String(const U32String &str, EncodingErrorMode errorMode) -> std::u32string;
    [[nodiscard]] static auto toStdU32String(const U32StringView &str, EncodingErrorMode errorMode) -> std::u32string;

    [[nodiscard]] static auto toStdWString(const U8String &str, EncodingErrorMode errorMode) -> std::wstring;
    [[nodiscard]] static auto toStdWString(const U8StringView &str, EncodingErrorMode errorMode) -> std::wstring;
    [[nodiscard]] static auto toStdWString(const U16String &str, EncodingErrorMode errorMode) -> std::wstring;
    [[nodiscard]] static auto toStdWString(const U16StringView &str, EncodingErrorMode errorMode) -> std::wstring;
    [[nodiscard]] static auto toStdWString(const U32String &str, EncodingErrorMode errorMode) -> std::wstring;
    [[nodiscard]] static auto toStdWString(const U32StringView &str, EncodingErrorMode errorMode) -> std::wstring;

    [[nodiscard]] static auto encode(
        const U8String &str, StringEncoding encoding, StringBomMode bomMode, EncodingErrorMode errorMode)
        -> mem::ByteBlock;
    [[nodiscard]] static auto encode(
        const U8StringView &str, StringEncoding encoding, StringBomMode bomMode, EncodingErrorMode errorMode)
        -> mem::ByteBlock;
    [[nodiscard]] static auto encode(
        const U8StringCharView &str, StringEncoding encoding, StringBomMode bomMode, EncodingErrorMode errorMode)
        -> mem::ByteBlock;
    [[nodiscard]] static auto encode(
        const U16String &str, StringEncoding encoding, StringBomMode bomMode, EncodingErrorMode errorMode)
        -> mem::ByteBlock;
    [[nodiscard]] static auto encode(
        const U16StringView &str, StringEncoding encoding, StringBomMode bomMode, EncodingErrorMode errorMode)
        -> mem::ByteBlock;
    [[nodiscard]] static auto encode(
        const U16StringCharView &str, StringEncoding encoding, StringBomMode bomMode, EncodingErrorMode errorMode)
        -> mem::ByteBlock;
    [[nodiscard]] static auto encode(
        const U32String &str, StringEncoding encoding, StringBomMode bomMode, EncodingErrorMode errorMode)
        -> mem::ByteBlock;
    [[nodiscard]] static auto encode(
        const U32StringView &str, StringEncoding encoding, StringBomMode bomMode, EncodingErrorMode errorMode)
        -> mem::ByteBlock;

    [[nodiscard]] static auto decodeU8String(
        const mem::ByteBlockView &data, StringEncoding encoding, StringBomMode bomMode, EncodingErrorMode errorMode)
        -> U8String;
    [[nodiscard]] static auto decodeU16String(
        const mem::ByteBlockView &data, StringEncoding encoding, StringBomMode bomMode, EncodingErrorMode errorMode)
        -> U16String;
    [[nodiscard]] static auto decodeU32String(
        const mem::ByteBlockView &data, StringEncoding encoding, StringBomMode bomMode, EncodingErrorMode errorMode)
        -> U32String;
};

}
