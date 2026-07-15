// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringConversionTools.hpp"

#include "../u16/impl/U16Encoding.hpp"
#include "../u16/impl/U16StringEncodingTools.hpp"
#include "../u16/impl/U16StringReadTools.hpp"
#include "../u16/U16String.hpp"
#include "../u16/U16StringCharView.hpp"
#include "../u16/U16StringView.hpp"
#include "../u32/impl/U32Encoding.hpp"
#include "../u32/impl/U32StringEncodingTools.hpp"
#include "../u32/impl/U32StringReadTools.hpp"
#include "../u32/U32String.hpp"
#include "../u32/U32StringView.hpp"
#include "../u8/impl/U8Encoding.hpp"
#include "../u8/impl/U8StringEncodingTools.hpp"
#include "../u8/impl/U8StringReadTools.hpp"
#include "../u8/U8String.hpp"
#include "../u8/U8StringCharView.hpp"
#include "../u8/U8StringView.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteBlockView.hpp"

namespace erbsland::text::impl {

auto StringConversionTools::toU8String(const std::string_view str, const EncodingErrorMode errorMode) -> U8String {
    return U8StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf8::forEachDecodedCharacter(str, errorMode, function); });
}

auto StringConversionTools::toU8String(const std::u8string_view str, const EncodingErrorMode errorMode) -> U8String {
    return toU8String(std::string_view{reinterpret_cast<const char *>(str.data()), str.size()}, errorMode);
}

auto StringConversionTools::toU8String(const std::u16string_view str, const EncodingErrorMode errorMode) -> U8String {
    return U8StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf16::forEachDecodedCharacter(str, errorMode, function); });
}

auto StringConversionTools::toU8String(const std::u32string_view str, const EncodingErrorMode errorMode) -> U8String {
    return U8StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf32::forEachValidatedCharacter(str, errorMode, function); });
}

auto StringConversionTools::toU8String(const std::wstring_view str, const EncodingErrorMode errorMode) -> U8String {
#ifdef ERBSLAND_WCHAR_16BIT
    return toU8String(std::u16string_view{reinterpret_cast<const char16_t *>(str.data()), str.size()}, errorMode);
#else
    return toU8String(std::u32string_view{reinterpret_cast<const char32_t *>(str.data()), str.size()}, errorMode);
#endif
}

auto StringConversionTools::toU8String(const U8String &str, const EncodingErrorMode errorMode) -> U8String {
    static_cast<void>(errorMode);
    return str;
}

auto StringConversionTools::toU8String(const U8StringView &str, const EncodingErrorMode errorMode) -> U8String {
    static_cast<void>(errorMode);
    return U8String{str};
}

auto StringConversionTools::toU8String(const U16String &str, const EncodingErrorMode errorMode) -> U8String {
    const auto data = str.dataView().dataSpan();
    return toU8String(std::u16string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU8String(const U16StringView &str, const EncodingErrorMode errorMode) -> U8String {
    const auto data = str.dataView().dataSpan();
    return toU8String(std::u16string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU8String(const U32String &str, const EncodingErrorMode errorMode) -> U8String {
    const auto data = str.dataView().dataSpan();
    return toU8String(std::u32string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU8String(const U32StringView &str, const EncodingErrorMode errorMode) -> U8String {
    const auto data = str.dataView().dataSpan();
    return toU8String(std::u32string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU16String(const std::string_view str, const EncodingErrorMode errorMode) -> U16String {
    return U16StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf8::forEachDecodedCharacter(str, errorMode, function); });
}

auto StringConversionTools::toU16String(const std::u8string_view str, const EncodingErrorMode errorMode) -> U16String {
    return toU16String(std::string_view{reinterpret_cast<const char *>(str.data()), str.size()}, errorMode);
}

auto StringConversionTools::toU16String(const std::u16string_view str, const EncodingErrorMode errorMode) -> U16String {
    return U16StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf16::forEachDecodedCharacter(str, errorMode, function); });
}

auto StringConversionTools::toU16String(const std::u32string_view str, const EncodingErrorMode errorMode) -> U16String {
    return U16StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf32::forEachValidatedCharacter(str, errorMode, function); });
}

auto StringConversionTools::toU16String(const std::wstring_view str, const EncodingErrorMode errorMode) -> U16String {
#ifdef ERBSLAND_WCHAR_16BIT
    return toU16String(std::u16string_view{reinterpret_cast<const char16_t *>(str.data()), str.size()}, errorMode);
#else
    return toU16String(std::u32string_view{reinterpret_cast<const char32_t *>(str.data()), str.size()}, errorMode);
#endif
}

auto StringConversionTools::toU16String(const U8String &str, const EncodingErrorMode errorMode) -> U16String {
    const auto data = str.dataView().dataSpan();
    return toU16String(std::string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU16String(const U8StringView &str, const EncodingErrorMode errorMode) -> U16String {
    const auto data = str.dataView().dataSpan();
    return toU16String(std::string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU16String(const U16String &str, const EncodingErrorMode errorMode) -> U16String {
    static_cast<void>(errorMode);
    return str;
}

auto StringConversionTools::toU16String(const U16StringView &str, const EncodingErrorMode errorMode) -> U16String {
    static_cast<void>(errorMode);
    return U16String{str};
}

auto StringConversionTools::toU16String(const U32String &str, const EncodingErrorMode errorMode) -> U16String {
    const auto data = str.dataView().dataSpan();
    return toU16String(std::u32string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU16String(const U32StringView &str, const EncodingErrorMode errorMode) -> U16String {
    const auto data = str.dataView().dataSpan();
    return toU16String(std::u32string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU32String(const std::string_view str, const EncodingErrorMode errorMode) -> U32String {
    return U32StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf8::forEachDecodedCharacter(str, errorMode, function); });
}

auto StringConversionTools::toU32String(const std::u8string_view str, const EncodingErrorMode errorMode) -> U32String {
    return toU32String(std::string_view{reinterpret_cast<const char *>(str.data()), str.size()}, errorMode);
}

auto StringConversionTools::toU32String(const std::u16string_view str, const EncodingErrorMode errorMode) -> U32String {
    return U32StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf16::forEachDecodedCharacter(str, errorMode, function); });
}

auto StringConversionTools::toU32String(const std::u32string_view str, const EncodingErrorMode errorMode) -> U32String {
    return U32StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf32::forEachValidatedCharacter(str, errorMode, function); });
}

auto StringConversionTools::toU32String(const std::wstring_view str, const EncodingErrorMode errorMode) -> U32String {
#ifdef ERBSLAND_WCHAR_16BIT
    return toU32String(std::u16string_view{reinterpret_cast<const char16_t *>(str.data()), str.size()}, errorMode);
#else
    return toU32String(std::u32string_view{reinterpret_cast<const char32_t *>(str.data()), str.size()}, errorMode);
#endif
}

auto StringConversionTools::toU32String(const U8String &str, const EncodingErrorMode errorMode) -> U32String {
    const auto data = str.dataView().dataSpan();
    return toU32String(std::string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU32String(const U8StringView &str, const EncodingErrorMode errorMode) -> U32String {
    const auto data = str.dataView().dataSpan();
    return toU32String(std::string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU32String(const U16String &str, const EncodingErrorMode errorMode) -> U32String {
    const auto data = str.dataView().dataSpan();
    return toU32String(std::u16string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU32String(const U16StringView &str, const EncodingErrorMode errorMode) -> U32String {
    const auto data = str.dataView().dataSpan();
    return toU32String(std::u16string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU32String(const U32String &str, const EncodingErrorMode errorMode) -> U32String {
    static_cast<void>(errorMode);
    return str;
}

auto StringConversionTools::toU32String(const U32StringView &str, const EncodingErrorMode errorMode) -> U32String {
    static_cast<void>(errorMode);
    return U32String{str};
}

auto StringConversionTools::toStdString(const U8String &str, const EncodingErrorMode errorMode) -> std::string {
    const auto u8String = toU8String(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdString();
}

auto StringConversionTools::toStdString(const U8StringView &str, const EncodingErrorMode errorMode) -> std::string {
    const auto u8String = toU8String(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdString();
}

auto StringConversionTools::toStdString(const U16String &str, const EncodingErrorMode errorMode) -> std::string {
    const auto u8String = toU8String(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdString();
}

auto StringConversionTools::toStdString(const U16StringView &str, const EncodingErrorMode errorMode) -> std::string {
    const auto u8String = toU8String(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdString();
}

auto StringConversionTools::toStdString(const U32String &str, const EncodingErrorMode errorMode) -> std::string {
    const auto u8String = toU8String(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdString();
}

auto StringConversionTools::toStdString(const U32StringView &str, const EncodingErrorMode errorMode) -> std::string {
    const auto u8String = toU8String(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdString();
}

auto StringConversionTools::toStdU8String(const U8String &str, const EncodingErrorMode errorMode) -> std::u8string {
    const auto u8String = toU8String(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdU8String();
}

auto StringConversionTools::toStdU8String(const U8StringView &str, const EncodingErrorMode errorMode) -> std::u8string {
    const auto u8String = toU8String(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdU8String();
}

auto StringConversionTools::toStdU8String(const U16String &str, const EncodingErrorMode errorMode) -> std::u8string {
    const auto u8String = toU8String(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdU8String();
}

auto StringConversionTools::toStdU8String(const U16StringView &str, const EncodingErrorMode errorMode)
    -> std::u8string {
    const auto u8String = toU8String(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdU8String();
}

auto StringConversionTools::toStdU8String(const U32String &str, const EncodingErrorMode errorMode) -> std::u8string {
    const auto u8String = toU8String(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdU8String();
}

auto StringConversionTools::toStdU8String(const U32StringView &str, const EncodingErrorMode errorMode)
    -> std::u8string {
    const auto u8String = toU8String(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdU8String();
}

auto StringConversionTools::toStdU16String(const U8String &str, const EncodingErrorMode errorMode) -> std::u16string {
    const auto u16String = toU16String(str, errorMode);
    return U16StringReadTools{u16String.dataView()}.toStdU16String();
}

auto StringConversionTools::toStdU16String(const U8StringView &str, const EncodingErrorMode errorMode)
    -> std::u16string {
    const auto u16String = toU16String(str, errorMode);
    return U16StringReadTools{u16String.dataView()}.toStdU16String();
}

auto StringConversionTools::toStdU16String(const U16String &str, const EncodingErrorMode errorMode) -> std::u16string {
    const auto u16String = toU16String(str, errorMode);
    return U16StringReadTools{u16String.dataView()}.toStdU16String();
}

auto StringConversionTools::toStdU16String(const U16StringView &str, const EncodingErrorMode errorMode)
    -> std::u16string {
    const auto u16String = toU16String(str, errorMode);
    return U16StringReadTools{u16String.dataView()}.toStdU16String();
}

auto StringConversionTools::toStdU16String(const U32String &str, const EncodingErrorMode errorMode) -> std::u16string {
    const auto u16String = toU16String(str, errorMode);
    return U16StringReadTools{u16String.dataView()}.toStdU16String();
}

auto StringConversionTools::toStdU16String(const U32StringView &str, const EncodingErrorMode errorMode)
    -> std::u16string {
    const auto u16String = toU16String(str, errorMode);
    return U16StringReadTools{u16String.dataView()}.toStdU16String();
}

auto StringConversionTools::toStdU32String(const U8String &str, const EncodingErrorMode errorMode) -> std::u32string {
    const auto u32String = toU32String(str, errorMode);
    return U32StringReadTools{u32String.dataView()}.toStdU32String();
}

auto StringConversionTools::toStdU32String(const U8StringView &str, const EncodingErrorMode errorMode)
    -> std::u32string {
    const auto u32String = toU32String(str, errorMode);
    return U32StringReadTools{u32String.dataView()}.toStdU32String();
}

auto StringConversionTools::toStdU32String(const U16String &str, const EncodingErrorMode errorMode) -> std::u32string {
    const auto u32String = toU32String(str, errorMode);
    return U32StringReadTools{u32String.dataView()}.toStdU32String();
}

auto StringConversionTools::toStdU32String(const U16StringView &str, const EncodingErrorMode errorMode)
    -> std::u32string {
    const auto u32String = toU32String(str, errorMode);
    return U32StringReadTools{u32String.dataView()}.toStdU32String();
}

auto StringConversionTools::toStdU32String(const U32String &str, const EncodingErrorMode errorMode) -> std::u32string {
    const auto u32String = toU32String(str, errorMode);
    return U32StringReadTools{u32String.dataView()}.toStdU32String();
}

auto StringConversionTools::toStdU32String(const U32StringView &str, const EncodingErrorMode errorMode)
    -> std::u32string {
    const auto u32String = toU32String(str, errorMode);
    return U32StringReadTools{u32String.dataView()}.toStdU32String();
}

auto StringConversionTools::toStdWString(const U8String &str, const EncodingErrorMode errorMode) -> std::wstring {
#ifdef ERBSLAND_WCHAR_16BIT
    const auto u16String = toU16String(str, errorMode);
    return U16StringReadTools{u16String.dataView()}.toStdWString();
#else
    const auto u32String = toU32String(str, errorMode);
    return U32StringReadTools{u32String.dataView()}.toStdWString();
#endif
}

auto StringConversionTools::toStdWString(const U8StringView &str, const EncodingErrorMode errorMode) -> std::wstring {
    return toStdWString(toU8String(str, errorMode), EncodingErrorMode::Replace);
}

auto StringConversionTools::toStdWString(const U16String &str, const EncodingErrorMode errorMode) -> std::wstring {
#ifdef ERBSLAND_WCHAR_16BIT
    const auto u16String = toU16String(str, errorMode);
    return U16StringReadTools{u16String.dataView()}.toStdWString();
#else
    const auto u32String = toU32String(str, errorMode);
    return U32StringReadTools{u32String.dataView()}.toStdWString();
#endif
}

auto StringConversionTools::toStdWString(const U16StringView &str, const EncodingErrorMode errorMode) -> std::wstring {
    return toStdWString(toU16String(str, errorMode), EncodingErrorMode::Replace);
}

auto StringConversionTools::toStdWString(const U32String &str, const EncodingErrorMode errorMode) -> std::wstring {
#ifdef ERBSLAND_WCHAR_16BIT
    const auto u16String = toU16String(str, errorMode);
    return U16StringReadTools{u16String.dataView()}.toStdWString();
#else
    const auto u32String = toU32String(str, errorMode);
    return U32StringReadTools{u32String.dataView()}.toStdWString();
#endif
}

auto StringConversionTools::toStdWString(const U32StringView &str, const EncodingErrorMode errorMode) -> std::wstring {
    return toStdWString(toU32String(str, errorMode), EncodingErrorMode::Replace);
}

auto StringConversionTools::encode(
    const U8String &str, const StringEncoding encoding, const StringBomMode bomMode, const EncodingErrorMode errorMode)
    -> mem::ByteBlock {
    return U8StringEncodingTools{str.dataView()}.encode(encoding, bomMode, errorMode);
}

auto StringConversionTools::encode(
    const U8StringView &str,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) -> mem::ByteBlock {
    return U8StringEncodingTools{str.dataView()}.encode(encoding, bomMode, errorMode);
}

auto StringConversionTools::encode(
    const U8StringCharView &str,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) -> mem::ByteBlock {
    return U8StringEncodingTools{str.dataView()}.encode(encoding, bomMode, errorMode);
}

auto StringConversionTools::encode(
    const U16String &str, const StringEncoding encoding, const StringBomMode bomMode, const EncodingErrorMode errorMode)
    -> mem::ByteBlock {
    return U16StringEncodingTools{str.dataView()}.encode(encoding, bomMode, errorMode);
}

auto StringConversionTools::encode(
    const U16StringView &str,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) -> mem::ByteBlock {
    return U16StringEncodingTools{str.dataView()}.encode(encoding, bomMode, errorMode);
}

auto StringConversionTools::encode(
    const U16StringCharView &str,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) -> mem::ByteBlock {
    return U16StringEncodingTools{str.dataView()}.encode(encoding, bomMode, errorMode);
}

auto StringConversionTools::encode(
    const U32String &str, const StringEncoding encoding, const StringBomMode bomMode, const EncodingErrorMode errorMode)
    -> mem::ByteBlock {
    return U32StringEncodingTools{str.dataView()}.encode(encoding, bomMode, errorMode);
}

auto StringConversionTools::encode(
    const U32StringView &str,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) -> mem::ByteBlock {
    return U32StringEncodingTools{str.dataView()}.encode(encoding, bomMode, errorMode);
}

auto StringConversionTools::decodeU8String(
    const mem::ByteBlockView &data,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) -> U8String {
    return U8StringEncodingTools::decode(data, encoding, bomMode, errorMode);
}

auto StringConversionTools::decodeU16String(
    const mem::ByteBlockView &data,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) -> U16String {
    return U16StringEncodingTools::decode(data, encoding, bomMode, errorMode);
}

auto StringConversionTools::decodeU32String(
    const mem::ByteBlockView &data,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) -> U32String {
    return U32StringEncodingTools::decode(data, encoding, bomMode, errorMode);
}

}
