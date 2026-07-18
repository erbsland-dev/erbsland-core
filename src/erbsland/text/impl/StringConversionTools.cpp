// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringConversionTools.hpp"

#include "../u16/impl/U16Encoding.hpp"
#include "../u16/impl/U16StringEncodingTools.hpp"
#include "../u16/impl/U16StringReadTools.hpp"
#include "../u16/U16String.hpp"
#include "../u16/U16StringEditor.hpp"
#include "../u32/impl/U32Encoding.hpp"
#include "../u32/impl/U32StringEncodingTools.hpp"
#include "../u32/impl/U32StringReadTools.hpp"
#include "../u32/U32String.hpp"
#include "../u32/U32StringEditor.hpp"
#include "../u8/impl/U8Encoding.hpp"
#include "../u8/impl/U8StringEncodingTools.hpp"
#include "../u8/impl/U8StringReadTools.hpp"
#include "../u8/U8String.hpp"
#include "../u8/U8StringEditor.hpp"

#include "../../mem/ByteBlock.hpp"

namespace erbsland::text::impl {

auto StringConversionTools::toU8StringEditor(const std::string_view str, const EncodingErrorMode errorMode)
    -> U8StringEditor {
    return U8StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf8::forEachDecodedCharacter(str, errorMode, function); });
}

auto StringConversionTools::toU8StringEditor(const std::u8string_view str, const EncodingErrorMode errorMode)
    -> U8StringEditor {
    return toU8StringEditor(std::string_view{reinterpret_cast<const char *>(str.data()), str.size()}, errorMode);
}

auto StringConversionTools::toU8StringEditor(const std::u16string_view str, const EncodingErrorMode errorMode)
    -> U8StringEditor {
    return U8StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf16::forEachDecodedCharacter(str, errorMode, function); });
}

auto StringConversionTools::toU8StringEditor(const std::u32string_view str, const EncodingErrorMode errorMode)
    -> U8StringEditor {
    return U8StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf32::forEachValidatedCharacter(str, errorMode, function); });
}

auto StringConversionTools::toU8StringEditor(const std::wstring_view str, const EncodingErrorMode errorMode)
    -> U8StringEditor {
#ifdef ERBSLAND_WCHAR_16BIT
    return toU8StringEditor(std::u16string_view{reinterpret_cast<const char16_t *>(str.data()), str.size()}, errorMode);
#else
    return toU8StringEditor(std::u32string_view{reinterpret_cast<const char32_t *>(str.data()), str.size()}, errorMode);
#endif
}

auto StringConversionTools::toU8StringEditor(const U8StringEditor &str, const EncodingErrorMode errorMode)
    -> U8StringEditor {
    static_cast<void>(errorMode);
    return str;
}

auto StringConversionTools::toU8StringEditor(const U8String &str, const EncodingErrorMode errorMode) -> U8String {
    static_cast<void>(errorMode);
    return str;
}

auto StringConversionTools::toU8StringEditor(const U16StringEditor &str, const EncodingErrorMode errorMode)
    -> U8StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU8StringEditor(std::u16string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU8StringEditor(const U16String &str, const EncodingErrorMode errorMode)
    -> U8StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU8StringEditor(std::u16string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU8StringEditor(const U32StringEditor &str, const EncodingErrorMode errorMode)
    -> U8StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU8StringEditor(std::u32string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU8StringEditor(const U32String &str, const EncodingErrorMode errorMode)
    -> U8StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU8StringEditor(std::u32string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU16StringEditor(const std::string_view str, const EncodingErrorMode errorMode)
    -> U16StringEditor {
    return U16StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf8::forEachDecodedCharacter(str, errorMode, function); });
}

auto StringConversionTools::toU16StringEditor(const std::u8string_view str, const EncodingErrorMode errorMode)
    -> U16StringEditor {
    return toU16StringEditor(std::string_view{reinterpret_cast<const char *>(str.data()), str.size()}, errorMode);
}

auto StringConversionTools::toU16StringEditor(const std::u16string_view str, const EncodingErrorMode errorMode)
    -> U16StringEditor {
    return U16StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf16::forEachDecodedCharacter(str, errorMode, function); });
}

auto StringConversionTools::toU16StringEditor(const std::u32string_view str, const EncodingErrorMode errorMode)
    -> U16StringEditor {
    return U16StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf32::forEachValidatedCharacter(str, errorMode, function); });
}

auto StringConversionTools::toU16StringEditor(const std::wstring_view str, const EncodingErrorMode errorMode)
    -> U16StringEditor {
#ifdef ERBSLAND_WCHAR_16BIT
    return toU16StringEditor(
        std::u16string_view{reinterpret_cast<const char16_t *>(str.data()), str.size()}, errorMode);
#else
    return toU16StringEditor(
        std::u32string_view{reinterpret_cast<const char32_t *>(str.data()), str.size()}, errorMode);
#endif
}

auto StringConversionTools::toU16StringEditor(const U8StringEditor &str, const EncodingErrorMode errorMode)
    -> U16StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU16StringEditor(std::string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU16StringEditor(const U8String &str, const EncodingErrorMode errorMode)
    -> U16StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU16StringEditor(std::string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU16StringEditor(const U16StringEditor &str, const EncodingErrorMode errorMode)
    -> U16StringEditor {
    static_cast<void>(errorMode);
    return str;
}

auto StringConversionTools::toU16StringEditor(const U16String &str, const EncodingErrorMode errorMode) -> U16String {
    static_cast<void>(errorMode);
    return str;
}

auto StringConversionTools::toU16StringEditor(const U32StringEditor &str, const EncodingErrorMode errorMode)
    -> U16StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU16StringEditor(std::u32string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU16StringEditor(const U32String &str, const EncodingErrorMode errorMode)
    -> U16StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU16StringEditor(std::u32string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU32StringEditor(const std::string_view str, const EncodingErrorMode errorMode)
    -> U32StringEditor {
    return U32StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf8::forEachDecodedCharacter(str, errorMode, function); });
}

auto StringConversionTools::toU32StringEditor(const std::u8string_view str, const EncodingErrorMode errorMode)
    -> U32StringEditor {
    return toU32StringEditor(std::string_view{reinterpret_cast<const char *>(str.data()), str.size()}, errorMode);
}

auto StringConversionTools::toU32StringEditor(const std::u16string_view str, const EncodingErrorMode errorMode)
    -> U32StringEditor {
    return U32StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf16::forEachDecodedCharacter(str, errorMode, function); });
}

auto StringConversionTools::toU32StringEditor(const std::u32string_view str, const EncodingErrorMode errorMode)
    -> U32StringEditor {
    return U32StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf32::forEachValidatedCharacter(str, errorMode, function); });
}

auto StringConversionTools::toU32StringEditor(const std::wstring_view str, const EncodingErrorMode errorMode)
    -> U32StringEditor {
#ifdef ERBSLAND_WCHAR_16BIT
    return toU32StringEditor(
        std::u16string_view{reinterpret_cast<const char16_t *>(str.data()), str.size()}, errorMode);
#else
    return toU32StringEditor(
        std::u32string_view{reinterpret_cast<const char32_t *>(str.data()), str.size()}, errorMode);
#endif
}

auto StringConversionTools::toU32StringEditor(const U8StringEditor &str, const EncodingErrorMode errorMode)
    -> U32StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU32StringEditor(std::string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU32StringEditor(const U8String &str, const EncodingErrorMode errorMode)
    -> U32StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU32StringEditor(std::string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU32StringEditor(const U16StringEditor &str, const EncodingErrorMode errorMode)
    -> U32StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU32StringEditor(std::u16string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU32StringEditor(const U16String &str, const EncodingErrorMode errorMode)
    -> U32StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU32StringEditor(std::u16string_view{data.data(), data.size()}, errorMode);
}

auto StringConversionTools::toU32StringEditor(const U32StringEditor &str, const EncodingErrorMode errorMode)
    -> U32StringEditor {
    static_cast<void>(errorMode);
    return str;
}

auto StringConversionTools::toU32StringEditor(const U32String &str, const EncodingErrorMode errorMode) -> U32String {
    static_cast<void>(errorMode);
    return str;
}

auto StringConversionTools::toStdString(const U8StringEditor &str, const EncodingErrorMode errorMode) -> std::string {
    const auto u8String = toU8StringEditor(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdString();
}

auto StringConversionTools::toStdString(const U8String &str, const EncodingErrorMode errorMode) -> std::string {
    const auto u8String = toU8StringEditor(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdString();
}

auto StringConversionTools::toStdString(const U16StringEditor &str, const EncodingErrorMode errorMode) -> std::string {
    const auto u8String = toU8StringEditor(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdString();
}

auto StringConversionTools::toStdString(const U16String &str, const EncodingErrorMode errorMode) -> std::string {
    const auto u8String = toU8StringEditor(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdString();
}

auto StringConversionTools::toStdString(const U32StringEditor &str, const EncodingErrorMode errorMode) -> std::string {
    const auto u8String = toU8StringEditor(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdString();
}

auto StringConversionTools::toStdString(const U32String &str, const EncodingErrorMode errorMode) -> std::string {
    const auto u8String = toU8StringEditor(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdString();
}

auto StringConversionTools::toStdU8String(const U8StringEditor &str, const EncodingErrorMode errorMode)
    -> std::u8string {
    const auto u8String = toU8StringEditor(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdU8String();
}

auto StringConversionTools::toStdU8String(const U8String &str, const EncodingErrorMode errorMode) -> std::u8string {
    const auto u8String = toU8StringEditor(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdU8String();
}

auto StringConversionTools::toStdU8String(const U16StringEditor &str, const EncodingErrorMode errorMode)
    -> std::u8string {
    const auto u8String = toU8StringEditor(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdU8String();
}

auto StringConversionTools::toStdU8String(const U16String &str, const EncodingErrorMode errorMode) -> std::u8string {
    const auto u8String = toU8StringEditor(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdU8String();
}

auto StringConversionTools::toStdU8String(const U32StringEditor &str, const EncodingErrorMode errorMode)
    -> std::u8string {
    const auto u8String = toU8StringEditor(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdU8String();
}

auto StringConversionTools::toStdU8String(const U32String &str, const EncodingErrorMode errorMode) -> std::u8string {
    const auto u8String = toU8StringEditor(str, errorMode);
    return U8StringReadTools{u8String.dataView()}.toStdU8String();
}

auto StringConversionTools::toStdU16String(const U8StringEditor &str, const EncodingErrorMode errorMode)
    -> std::u16string {
    const auto u16String = toU16StringEditor(str, errorMode);
    return U16StringReadTools{u16String.dataView()}.toStdU16String();
}

auto StringConversionTools::toStdU16String(const U8String &str, const EncodingErrorMode errorMode) -> std::u16string {
    const auto u16String = toU16StringEditor(str, errorMode);
    return U16StringReadTools{u16String.dataView()}.toStdU16String();
}

auto StringConversionTools::toStdU16String(const U16StringEditor &str, const EncodingErrorMode errorMode)
    -> std::u16string {
    const auto u16String = toU16StringEditor(str, errorMode);
    return U16StringReadTools{u16String.dataView()}.toStdU16String();
}

auto StringConversionTools::toStdU16String(const U16String &str, const EncodingErrorMode errorMode) -> std::u16string {
    const auto u16String = toU16StringEditor(str, errorMode);
    return U16StringReadTools{u16String.dataView()}.toStdU16String();
}

auto StringConversionTools::toStdU16String(const U32StringEditor &str, const EncodingErrorMode errorMode)
    -> std::u16string {
    const auto u16String = toU16StringEditor(str, errorMode);
    return U16StringReadTools{u16String.dataView()}.toStdU16String();
}

auto StringConversionTools::toStdU16String(const U32String &str, const EncodingErrorMode errorMode) -> std::u16string {
    const auto u16String = toU16StringEditor(str, errorMode);
    return U16StringReadTools{u16String.dataView()}.toStdU16String();
}

auto StringConversionTools::toStdU32String(const U8StringEditor &str, const EncodingErrorMode errorMode)
    -> std::u32string {
    const auto u32String = toU32StringEditor(str, errorMode);
    return U32StringReadTools{u32String.dataView()}.toStdU32String();
}

auto StringConversionTools::toStdU32String(const U8String &str, const EncodingErrorMode errorMode) -> std::u32string {
    const auto u32String = toU32StringEditor(str, errorMode);
    return U32StringReadTools{u32String.dataView()}.toStdU32String();
}

auto StringConversionTools::toStdU32String(const U16StringEditor &str, const EncodingErrorMode errorMode)
    -> std::u32string {
    const auto u32String = toU32StringEditor(str, errorMode);
    return U32StringReadTools{u32String.dataView()}.toStdU32String();
}

auto StringConversionTools::toStdU32String(const U16String &str, const EncodingErrorMode errorMode) -> std::u32string {
    const auto u32String = toU32StringEditor(str, errorMode);
    return U32StringReadTools{u32String.dataView()}.toStdU32String();
}

auto StringConversionTools::toStdU32String(const U32StringEditor &str, const EncodingErrorMode errorMode)
    -> std::u32string {
    const auto u32String = toU32StringEditor(str, errorMode);
    return U32StringReadTools{u32String.dataView()}.toStdU32String();
}

auto StringConversionTools::toStdU32String(const U32String &str, const EncodingErrorMode errorMode) -> std::u32string {
    const auto u32String = toU32StringEditor(str, errorMode);
    return U32StringReadTools{u32String.dataView()}.toStdU32String();
}

auto StringConversionTools::toStdWString(const U8StringEditor &str, const EncodingErrorMode errorMode) -> std::wstring {
#ifdef ERBSLAND_WCHAR_16BIT
    const auto u16String = toU16StringEditor(str, errorMode);
    return U16StringReadTools{u16String.dataView()}.toStdWString();
#else
    const auto u32String = toU32StringEditor(str, errorMode);
    return U32StringReadTools{u32String.dataView()}.toStdWString();
#endif
}

auto StringConversionTools::toStdWString(const U8String &str, const EncodingErrorMode errorMode) -> std::wstring {
#ifdef ERBSLAND_WCHAR_16BIT
    const auto u16String = toU16StringEditor(str, errorMode);
    return U16StringReadTools{u16String.dataView()}.toStdWString();
#else
    const auto u32String = toU32StringEditor(str, errorMode);
    return U32StringReadTools{u32String.dataView()}.toStdWString();
#endif
}

auto StringConversionTools::toStdWString(const U16StringEditor &str, const EncodingErrorMode errorMode)
    -> std::wstring {
#ifdef ERBSLAND_WCHAR_16BIT
    const auto u16String = toU16StringEditor(str, errorMode);
    return U16StringReadTools{u16String.dataView()}.toStdWString();
#else
    const auto u32String = toU32StringEditor(str, errorMode);
    return U32StringReadTools{u32String.dataView()}.toStdWString();
#endif
}

auto StringConversionTools::toStdWString(const U16String &str, const EncodingErrorMode errorMode) -> std::wstring {
#ifdef ERBSLAND_WCHAR_16BIT
    static_cast<void>(errorMode);
    return U16StringReadTools{str.dataView()}.toStdWString();
#else
    const auto u32String = toU32StringEditor(str, errorMode);
    return U32StringReadTools{u32String.dataView()}.toStdWString();
#endif
}

auto StringConversionTools::toStdWString(const U32StringEditor &str, const EncodingErrorMode errorMode)
    -> std::wstring {
#ifdef ERBSLAND_WCHAR_16BIT
    const auto u16String = toU16StringEditor(str, errorMode);
    return U16StringReadTools{u16String.dataView()}.toStdWString();
#else
    const auto u32String = toU32StringEditor(str, errorMode);
    return U32StringReadTools{u32String.dataView()}.toStdWString();
#endif
}

auto StringConversionTools::toStdWString(const U32String &str, const EncodingErrorMode errorMode) -> std::wstring {
#ifdef ERBSLAND_WCHAR_16BIT
    const auto u16String = toU16StringEditor(str, errorMode);
    return U16StringReadTools{u16String.dataView()}.toStdWString();
#else
    static_cast<void>(errorMode);
    return U32StringReadTools{str.dataView()}.toStdWString();
#endif
}

auto StringConversionTools::encode(
    const U8StringEditor &str,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) -> mem::ByteBlock {
    return U8StringEncodingTools{str.dataView()}.encode(encoding, bomMode, errorMode);
}

auto StringConversionTools::encode(
    const U8String &str, const StringEncoding encoding, const StringBomMode bomMode, const EncodingErrorMode errorMode)
    -> mem::ByteBlock {
    return U8StringEncodingTools{str.dataView()}.encode(encoding, bomMode, errorMode);
}

auto StringConversionTools::encode(
    const U16StringEditor &str,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) -> mem::ByteBlock {
    return U16StringEncodingTools{str.dataView()}.encode(encoding, bomMode, errorMode);
}

auto StringConversionTools::encode(
    const U16String &str, const StringEncoding encoding, const StringBomMode bomMode, const EncodingErrorMode errorMode)
    -> mem::ByteBlock {
    return U16StringEncodingTools{str.dataView()}.encode(encoding, bomMode, errorMode);
}

auto StringConversionTools::encode(
    const U32StringEditor &str,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) -> mem::ByteBlock {
    return U32StringEncodingTools{str.dataView()}.encode(encoding, bomMode, errorMode);
}

auto StringConversionTools::encode(
    const U32String &str, const StringEncoding encoding, const StringBomMode bomMode, const EncodingErrorMode errorMode)
    -> mem::ByteBlock {
    return U32StringEncodingTools{str.dataView()}.encode(encoding, bomMode, errorMode);
}

#define ERBSLAND_DEFINE_STRING_CONVERSION_LENGTH(TYPE, TOOLS)                                                          \
    auto StringConversionTools::encodedLength(                                                                         \
        const TYPE &str,                                                                                               \
        const StringEncoding encoding,                                                                                 \
        const StringBomMode bomMode,                                                                                   \
        const EncodingErrorMode errorMode) -> unit::ByteLength {                                                       \
        return TOOLS{str.dataView()}.encodedLength(encoding, bomMode, errorMode);                                      \
    }

ERBSLAND_DEFINE_STRING_CONVERSION_LENGTH(U8StringEditor, U8StringEncodingTools);
ERBSLAND_DEFINE_STRING_CONVERSION_LENGTH(U8String, U8StringEncodingTools);
ERBSLAND_DEFINE_STRING_CONVERSION_LENGTH(U16StringEditor, U16StringEncodingTools);
ERBSLAND_DEFINE_STRING_CONVERSION_LENGTH(U16String, U16StringEncodingTools);
ERBSLAND_DEFINE_STRING_CONVERSION_LENGTH(U32StringEditor, U32StringEncodingTools);
ERBSLAND_DEFINE_STRING_CONVERSION_LENGTH(U32String, U32StringEncodingTools);

#undef ERBSLAND_DEFINE_STRING_CONVERSION_LENGTH

#define ERBSLAND_DEFINE_STRING_CONVERSION_ENCODE_TO(TYPE, TOOLS)                                                       \
    auto StringConversionTools::encodeTo(                                                                              \
        const TYPE &str,                                                                                               \
        mem::RingBuffer &buffer,                                                                                       \
        const StringEncoding encoding,                                                                                 \
        const StringBomMode bomMode,                                                                                   \
        const EncodingErrorMode errorMode) -> util::Result {                                                           \
        return TOOLS{str.dataView()}.encodeTo(buffer, encoding, bomMode, errorMode);                                   \
    }

ERBSLAND_DEFINE_STRING_CONVERSION_ENCODE_TO(U8StringEditor, U8StringEncodingTools);
ERBSLAND_DEFINE_STRING_CONVERSION_ENCODE_TO(U8String, U8StringEncodingTools);
ERBSLAND_DEFINE_STRING_CONVERSION_ENCODE_TO(U16StringEditor, U16StringEncodingTools);
ERBSLAND_DEFINE_STRING_CONVERSION_ENCODE_TO(U16String, U16StringEncodingTools);
ERBSLAND_DEFINE_STRING_CONVERSION_ENCODE_TO(U32StringEditor, U32StringEncodingTools);
ERBSLAND_DEFINE_STRING_CONVERSION_ENCODE_TO(U32String, U32StringEncodingTools);

#undef ERBSLAND_DEFINE_STRING_CONVERSION_ENCODE_TO

auto StringConversionTools::decodeU8String(
    const mem::ByteBlock &data,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) -> U8StringEditor {
    return U8StringEncodingTools::decode(data, encoding, bomMode, errorMode);
}

auto StringConversionTools::decodeU16String(
    const mem::ByteBlock &data,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) -> U16StringEditor {
    return U16StringEncodingTools::decode(data, encoding, bomMode, errorMode);
}

auto StringConversionTools::decodeU32String(
    const mem::ByteBlock &data,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) -> U32StringEditor {
    return U32StringEncodingTools::decode(data, encoding, bomMode, errorMode);
}

}
