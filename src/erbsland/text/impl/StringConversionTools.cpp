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
#include "../../mem/impl/UnsafeByteBlockBuffer.hpp"
#include "../../mem/impl/UnsafeRingBufferAccess.hpp"
#include "../../mem/RingBuffer.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <span>

namespace erbsland::text::impl {

template <typename T>
[[nodiscard]] auto StringConversionTools::isMatchingRepresentation(const StringEncoding encoding) noexcept -> bool {
    if constexpr (std::same_as<T, char>) {
        return encoding.isUtf8();
    } else if constexpr (std::same_as<T, char16_t>) {
        const auto nativeEndianness =
            std::endian::native == std::endian::little ? mem::Endianness::Little : mem::Endianness::Big;
        return encoding.isUtf16() && encoding.endianness() == nativeEndianness;
    } else if constexpr (std::same_as<T, char32_t>) {
        const auto nativeEndianness =
            std::endian::native == std::endian::little ? mem::Endianness::Little : mem::Endianness::Big;
        return encoding.isUtf32() && encoding.endianness() == nativeEndianness;
    } else {
        return false;
    }
}

template <typename T>
[[nodiscard]] auto StringConversionTools::asByteSpan(const std::span<const T> source) noexcept -> mem::ConstByteSpan {
    return {reinterpret_cast<const mem::Byte *>(source.data()), source.size_bytes()};
}

template <typename T>
[[nodiscard]] auto StringConversionTools::rawEncodedLength(
    const std::span<const T> source, const StringEncoding encoding, const StringBomMode bomMode) -> unit::ByteLength {
    auto result = encoding.bomLength(bomMode);
    result.addOrThrow(unit::ByteLength::fromSizeT(source.size_bytes()));
    return result;
}

void StringConversionTools::copyBytes(
    const mem::ByteSpan destination, std::size_t &offset, const mem::ConstByteSpan source) noexcept {
    if (!source.empty()) {
        std::memcpy(destination.data() + offset, source.data(), source.size());
        offset += source.size();
    }
}

template <typename T>
[[nodiscard]] auto StringConversionTools::encodeRaw(
    const std::span<const T> source, const StringEncoding encoding, const StringBomMode bomMode) -> mem::ByteBlock {
    const auto length = rawEncodedLength(source, encoding, bomMode);
    auto buffer = mem::impl::UnsafeByteBlockBuffer{length};
    auto offset = std::size_t{0U};
    copyBytes(buffer.data(), offset, encoding.bomBytes(bomMode));
    copyBytes(buffer.data(), offset, asByteSpan(source));
    return buffer.take(length);
}

template <typename T>
[[nodiscard]] auto StringConversionTools::encodeRawTo(
    const std::span<const T> source,
    mem::RingBuffer &buffer,
    const StringEncoding encoding,
    const StringBomMode bomMode) -> util::Result {
    const auto length = rawEncodedLength(source, encoding, bomMode);
    if (isFailure(buffer.reserveAdditional(length))) {
        return util::Result::Failure;
    }
    auto access = mem::impl::UnsafeRingBufferAccess{buffer};
    const auto writable = access.writableSpans();
    auto spanIndex = std::size_t{0U};
    auto spanOffset = std::size_t{0U};
    auto write = [&](const mem::ConstByteSpan bytes) -> void {
        auto sourceOffset = std::size_t{0U};
        while (sourceOffset < bytes.size()) {
            if (spanOffset == writable[spanIndex].size()) {
                ++spanIndex;
                spanOffset = 0U;
            }
            const auto count = std::min(bytes.size() - sourceOffset, writable[spanIndex].size() - spanOffset);
            std::memcpy(writable[spanIndex].data() + spanOffset, bytes.data() + sourceOffset, count);
            sourceOffset += count;
            spanOffset += count;
        }
    };
    write(encoding.bomBytes(bomMode));
    write(asByteSpan(source));
    access.commitWritten(length);
    return util::Result::Success;
}

template <typename tData, typename tTools, typename tDataView>
[[nodiscard]] auto StringConversionTools::encodeString(
    const tDataView dataView, const StringEncoding encoding, const StringBomMode bomMode) -> mem::ByteBlock {
    if (isMatchingRepresentation<tData>(encoding)) {
        return encodeRaw<tData>(dataView.dataSpan(), encoding, bomMode);
    }
    return tTools{dataView}.encode(encoding, bomMode);
}

template <typename tData, typename tTools, typename tDataView>
[[nodiscard]] auto StringConversionTools::encodedStringLength(
    const tDataView dataView, const StringEncoding encoding, const StringBomMode bomMode) -> unit::ByteLength {
    if (isMatchingRepresentation<tData>(encoding)) {
        return rawEncodedLength<tData>(dataView.dataSpan(), encoding, bomMode);
    }
    return tTools{dataView}.encodedLength(encoding, bomMode);
}

template <typename tData, typename tTools, typename tDataView>
[[nodiscard]] auto StringConversionTools::encodeStringTo(
    const tDataView dataView, mem::RingBuffer &buffer, const StringEncoding encoding, const StringBomMode bomMode)
    -> util::Result {
    if (isMatchingRepresentation<tData>(encoding)) {
        return encodeRawTo<tData>(dataView.dataSpan(), buffer, encoding, bomMode);
    }
    return tTools{dataView}.encodeTo(buffer, encoding, bomMode);
}

auto StringConversionTools::toU8StringEditor(const std::string_view str, const EncodingMode mode) -> U8StringEditor {
    return U8StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf8::forEachDecodedCharacter(str, mode, function); });
}

auto StringConversionTools::toU8StringEditor(const std::u8string_view str, const EncodingMode mode) -> U8StringEditor {
    return toU8StringEditor(std::string_view{reinterpret_cast<const char *>(str.data()), str.size()}, mode);
}

auto StringConversionTools::toU8StringEditor(const std::u16string_view str, const EncodingMode mode) -> U8StringEditor {
    return U8StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf16::forEachDecodedCharacter(str, mode, function); });
}

auto StringConversionTools::toU8StringEditor(const std::u32string_view str, const EncodingMode mode) -> U8StringEditor {
    return U8StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf32::forEachValidatedCharacter(str, mode, function); });
}

auto StringConversionTools::toU8StringEditor(const std::wstring_view str, const EncodingMode mode) -> U8StringEditor {
#ifdef ERBSLAND_WCHAR_16BIT
    return toU8StringEditor(std::u16string_view{reinterpret_cast<const char16_t *>(str.data()), str.size()}, mode);
#else
    return toU8StringEditor(std::u32string_view{reinterpret_cast<const char32_t *>(str.data()), str.size()}, mode);
#endif
}

auto StringConversionTools::toU8StringEditor(const U8StringEditor &str, const EncodingMode mode) -> U8StringEditor {
    if (mode == EncodingMode::Strict) {
        const auto data = str.dataView().dataSpan();
        return toU8StringEditor(std::string_view{data.data(), data.size()}, mode);
    }
    return str;
}

auto StringConversionTools::toU8StringEditor(const U8String &str, const EncodingMode mode) -> U8String {
    if (mode == EncodingMode::Strict) {
        const auto data = str.dataView().dataSpan();
        return toU8StringEditor(std::string_view{data.data(), data.size()}, mode);
    }
    return str;
}

auto StringConversionTools::toU8StringEditor(const U16StringEditor &str, const EncodingMode mode) -> U8StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU8StringEditor(std::u16string_view{data.data(), data.size()}, mode);
}

auto StringConversionTools::toU8StringEditor(const U16String &str, const EncodingMode mode) -> U8StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU8StringEditor(std::u16string_view{data.data(), data.size()}, mode);
}

auto StringConversionTools::toU8StringEditor(const U32StringEditor &str, const EncodingMode mode) -> U8StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU8StringEditor(std::u32string_view{data.data(), data.size()}, mode);
}

auto StringConversionTools::toU8StringEditor(const U32String &str, const EncodingMode mode) -> U8StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU8StringEditor(std::u32string_view{data.data(), data.size()}, mode);
}

auto StringConversionTools::toU16StringEditor(const std::string_view str, const EncodingMode mode) -> U16StringEditor {
    return U16StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf8::forEachDecodedCharacter(str, mode, function); });
}

auto StringConversionTools::toU16StringEditor(const std::u8string_view str, const EncodingMode mode)
    -> U16StringEditor {
    return toU16StringEditor(std::string_view{reinterpret_cast<const char *>(str.data()), str.size()}, mode);
}

auto StringConversionTools::toU16StringEditor(const std::u16string_view str, const EncodingMode mode)
    -> U16StringEditor {
    return U16StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf16::forEachDecodedCharacter(str, mode, function); });
}

auto StringConversionTools::toU16StringEditor(const std::u32string_view str, const EncodingMode mode)
    -> U16StringEditor {
    return U16StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf32::forEachValidatedCharacter(str, mode, function); });
}

auto StringConversionTools::toU16StringEditor(const std::wstring_view str, const EncodingMode mode) -> U16StringEditor {
#ifdef ERBSLAND_WCHAR_16BIT
    return toU16StringEditor(std::u16string_view{reinterpret_cast<const char16_t *>(str.data()), str.size()}, mode);
#else
    return toU16StringEditor(std::u32string_view{reinterpret_cast<const char32_t *>(str.data()), str.size()}, mode);
#endif
}

auto StringConversionTools::toU16StringEditor(const U8StringEditor &str, const EncodingMode mode) -> U16StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU16StringEditor(std::string_view{data.data(), data.size()}, mode);
}

auto StringConversionTools::toU16StringEditor(const U8String &str, const EncodingMode mode) -> U16StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU16StringEditor(std::string_view{data.data(), data.size()}, mode);
}

auto StringConversionTools::toU16StringEditor(const U16StringEditor &str, const EncodingMode mode) -> U16StringEditor {
    if (mode == EncodingMode::Strict) {
        const auto data = str.dataView().dataSpan();
        return toU16StringEditor(std::u16string_view{data.data(), data.size()}, mode);
    }
    return str;
}

auto StringConversionTools::toU16StringEditor(const U16String &str, const EncodingMode mode) -> U16String {
    if (mode == EncodingMode::Strict) {
        const auto data = str.dataView().dataSpan();
        return toU16StringEditor(std::u16string_view{data.data(), data.size()}, mode);
    }
    return str;
}

auto StringConversionTools::toU16StringEditor(const U32StringEditor &str, const EncodingMode mode) -> U16StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU16StringEditor(std::u32string_view{data.data(), data.size()}, mode);
}

auto StringConversionTools::toU16StringEditor(const U32String &str, const EncodingMode mode) -> U16StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU16StringEditor(std::u32string_view{data.data(), data.size()}, mode);
}

auto StringConversionTools::toU32StringEditor(const std::string_view str, const EncodingMode mode) -> U32StringEditor {
    return U32StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf8::forEachDecodedCharacter(str, mode, function); });
}

auto StringConversionTools::toU32StringEditor(const std::u8string_view str, const EncodingMode mode)
    -> U32StringEditor {
    return toU32StringEditor(std::string_view{reinterpret_cast<const char *>(str.data()), str.size()}, mode);
}

auto StringConversionTools::toU32StringEditor(const std::u16string_view str, const EncodingMode mode)
    -> U32StringEditor {
    return U32StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf16::forEachDecodedCharacter(str, mode, function); });
}

auto StringConversionTools::toU32StringEditor(const std::u32string_view str, const EncodingMode mode)
    -> U32StringEditor {
    return U32StringEncodingTools::decodeFromCharacters(
        [&](auto function) -> void { utf32::forEachValidatedCharacter(str, mode, function); });
}

auto StringConversionTools::toU32StringEditor(const std::wstring_view str, const EncodingMode mode) -> U32StringEditor {
#ifdef ERBSLAND_WCHAR_16BIT
    return toU32StringEditor(std::u16string_view{reinterpret_cast<const char16_t *>(str.data()), str.size()}, mode);
#else
    return toU32StringEditor(std::u32string_view{reinterpret_cast<const char32_t *>(str.data()), str.size()}, mode);
#endif
}

auto StringConversionTools::toU32StringEditor(const U8StringEditor &str, const EncodingMode mode) -> U32StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU32StringEditor(std::string_view{data.data(), data.size()}, mode);
}

auto StringConversionTools::toU32StringEditor(const U8String &str, const EncodingMode mode) -> U32StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU32StringEditor(std::string_view{data.data(), data.size()}, mode);
}

auto StringConversionTools::toU32StringEditor(const U16StringEditor &str, const EncodingMode mode) -> U32StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU32StringEditor(std::u16string_view{data.data(), data.size()}, mode);
}

auto StringConversionTools::toU32StringEditor(const U16String &str, const EncodingMode mode) -> U32StringEditor {
    const auto data = str.dataView().dataSpan();
    return toU32StringEditor(std::u16string_view{data.data(), data.size()}, mode);
}

auto StringConversionTools::toU32StringEditor(const U32StringEditor &str, const EncodingMode mode) -> U32StringEditor {
    if (mode == EncodingMode::Strict) {
        const auto data = str.dataView().dataSpan();
        return toU32StringEditor(std::u32string_view{data.data(), data.size()}, mode);
    }
    return str;
}

auto StringConversionTools::toU32StringEditor(const U32String &str, const EncodingMode mode) -> U32String {
    if (mode == EncodingMode::Strict) {
        const auto data = str.dataView().dataSpan();
        return toU32StringEditor(std::u32string_view{data.data(), data.size()}, mode);
    }
    return str;
}

auto StringConversionTools::toStdString(const U8StringEditor &str) -> std::string {
    const auto u8String = toU8StringEditor(str);
    return U8StringReadTools{u8String.dataView()}.toStdString();
}

auto StringConversionTools::toStdString(const U8String &str) -> std::string {
    const auto u8String = toU8StringEditor(str);
    return U8StringReadTools{u8String.dataView()}.toStdString();
}

auto StringConversionTools::toStdString(const U16StringEditor &str) -> std::string {
    const auto u8String = toU8StringEditor(str);
    return U8StringReadTools{u8String.dataView()}.toStdString();
}

auto StringConversionTools::toStdString(const U16String &str) -> std::string {
    const auto u8String = toU8StringEditor(str);
    return U8StringReadTools{u8String.dataView()}.toStdString();
}

auto StringConversionTools::toStdString(const U32StringEditor &str) -> std::string {
    const auto u8String = toU8StringEditor(str);
    return U8StringReadTools{u8String.dataView()}.toStdString();
}

auto StringConversionTools::toStdString(const U32String &str) -> std::string {
    const auto u8String = toU8StringEditor(str);
    return U8StringReadTools{u8String.dataView()}.toStdString();
}

auto StringConversionTools::toStdU8String(const U8StringEditor &str) -> std::u8string {
    const auto u8String = toU8StringEditor(str);
    return U8StringReadTools{u8String.dataView()}.toStdU8String();
}

auto StringConversionTools::toStdU8String(const U8String &str) -> std::u8string {
    const auto u8String = toU8StringEditor(str);
    return U8StringReadTools{u8String.dataView()}.toStdU8String();
}

auto StringConversionTools::toStdU8String(const U16StringEditor &str) -> std::u8string {
    const auto u8String = toU8StringEditor(str);
    return U8StringReadTools{u8String.dataView()}.toStdU8String();
}

auto StringConversionTools::toStdU8String(const U16String &str) -> std::u8string {
    const auto u8String = toU8StringEditor(str);
    return U8StringReadTools{u8String.dataView()}.toStdU8String();
}

auto StringConversionTools::toStdU8String(const U32StringEditor &str) -> std::u8string {
    const auto u8String = toU8StringEditor(str);
    return U8StringReadTools{u8String.dataView()}.toStdU8String();
}

auto StringConversionTools::toStdU8String(const U32String &str) -> std::u8string {
    const auto u8String = toU8StringEditor(str);
    return U8StringReadTools{u8String.dataView()}.toStdU8String();
}

auto StringConversionTools::toStdU16String(const U8StringEditor &str) -> std::u16string {
    const auto u16String = toU16StringEditor(str);
    return U16StringReadTools{u16String.dataView()}.toStdU16String();
}

auto StringConversionTools::toStdU16String(const U8String &str) -> std::u16string {
    const auto u16String = toU16StringEditor(str);
    return U16StringReadTools{u16String.dataView()}.toStdU16String();
}

auto StringConversionTools::toStdU16String(const U16StringEditor &str) -> std::u16string {
    const auto u16String = toU16StringEditor(str);
    return U16StringReadTools{u16String.dataView()}.toStdU16String();
}

auto StringConversionTools::toStdU16String(const U16String &str) -> std::u16string {
    const auto u16String = toU16StringEditor(str);
    return U16StringReadTools{u16String.dataView()}.toStdU16String();
}

auto StringConversionTools::toStdU16String(const U32StringEditor &str) -> std::u16string {
    const auto u16String = toU16StringEditor(str);
    return U16StringReadTools{u16String.dataView()}.toStdU16String();
}

auto StringConversionTools::toStdU16String(const U32String &str) -> std::u16string {
    const auto u16String = toU16StringEditor(str);
    return U16StringReadTools{u16String.dataView()}.toStdU16String();
}

auto StringConversionTools::toStdU32String(const U8StringEditor &str) -> std::u32string {
    const auto u32String = toU32StringEditor(str);
    return U32StringReadTools{u32String.dataView()}.toStdU32String();
}

auto StringConversionTools::toStdU32String(const U8String &str) -> std::u32string {
    const auto u32String = toU32StringEditor(str);
    return U32StringReadTools{u32String.dataView()}.toStdU32String();
}

auto StringConversionTools::toStdU32String(const U16StringEditor &str) -> std::u32string {
    const auto u32String = toU32StringEditor(str);
    return U32StringReadTools{u32String.dataView()}.toStdU32String();
}

auto StringConversionTools::toStdU32String(const U16String &str) -> std::u32string {
    const auto u32String = toU32StringEditor(str);
    return U32StringReadTools{u32String.dataView()}.toStdU32String();
}

auto StringConversionTools::toStdU32String(const U32StringEditor &str) -> std::u32string {
    const auto u32String = toU32StringEditor(str);
    return U32StringReadTools{u32String.dataView()}.toStdU32String();
}

auto StringConversionTools::toStdU32String(const U32String &str) -> std::u32string {
    const auto u32String = toU32StringEditor(str);
    return U32StringReadTools{u32String.dataView()}.toStdU32String();
}

auto StringConversionTools::toStdWString(const U8StringEditor &str) -> std::wstring {
#ifdef ERBSLAND_WCHAR_16BIT
    const auto u16String = toU16StringEditor(str);
    return U16StringReadTools{u16String.dataView()}.toStdWString();
#else
    const auto u32String = toU32StringEditor(str);
    return U32StringReadTools{u32String.dataView()}.toStdWString();
#endif
}

auto StringConversionTools::toStdWString(const U8String &str) -> std::wstring {
#ifdef ERBSLAND_WCHAR_16BIT
    const auto u16String = toU16StringEditor(str);
    return U16StringReadTools{u16String.dataView()}.toStdWString();
#else
    const auto u32String = toU32StringEditor(str);
    return U32StringReadTools{u32String.dataView()}.toStdWString();
#endif
}

auto StringConversionTools::toStdWString(const U16StringEditor &str) -> std::wstring {
#ifdef ERBSLAND_WCHAR_16BIT
    const auto u16String = toU16StringEditor(str);
    return U16StringReadTools{u16String.dataView()}.toStdWString();
#else
    const auto u32String = toU32StringEditor(str);
    return U32StringReadTools{u32String.dataView()}.toStdWString();
#endif
}

auto StringConversionTools::toStdWString(const U16String &str) -> std::wstring {
#ifdef ERBSLAND_WCHAR_16BIT
    return U16StringReadTools{str.dataView()}.toStdWString();
#else
    const auto u32String = toU32StringEditor(str);
    return U32StringReadTools{u32String.dataView()}.toStdWString();
#endif
}

auto StringConversionTools::toStdWString(const U32StringEditor &str) -> std::wstring {
#ifdef ERBSLAND_WCHAR_16BIT
    const auto u16String = toU16StringEditor(str);
    return U16StringReadTools{u16String.dataView()}.toStdWString();
#else
    const auto u32String = toU32StringEditor(str);
    return U32StringReadTools{u32String.dataView()}.toStdWString();
#endif
}

auto StringConversionTools::toStdWString(const U32String &str) -> std::wstring {
#ifdef ERBSLAND_WCHAR_16BIT
    const auto u16String = toU16StringEditor(str);
    return U16StringReadTools{u16String.dataView()}.toStdWString();
#else
    return U32StringReadTools{str.dataView()}.toStdWString();
#endif
}

#define ERBSLAND_DEFINE_STRING_CONVERSION(TYPE, DATA_TYPE, TOOLS)                                                      \
    auto StringConversionTools::encode(const TYPE &str, const StringEncoding encoding, const StringBomMode bomMode)    \
        -> mem::ByteBlock {                                                                                            \
        return encodeString<DATA_TYPE, TOOLS>(str.dataView(), encoding, bomMode);                                      \
    }                                                                                                                  \
    auto StringConversionTools::encodedLength(                                                                         \
        const TYPE &str, const StringEncoding encoding, const StringBomMode bomMode) -> unit::ByteLength {             \
        return encodedStringLength<DATA_TYPE, TOOLS>(str.dataView(), encoding, bomMode);                               \
    }                                                                                                                  \
    auto StringConversionTools::encodeTo(                                                                              \
        const TYPE &str, mem::RingBuffer &buffer, const StringEncoding encoding, const StringBomMode bomMode)          \
        -> util::Result {                                                                                              \
        return encodeStringTo<DATA_TYPE, TOOLS>(str.dataView(), buffer, encoding, bomMode);                            \
    }

ERBSLAND_DEFINE_STRING_CONVERSION(U8StringEditor, char, U8StringEncodingTools);
ERBSLAND_DEFINE_STRING_CONVERSION(U8String, char, U8StringEncodingTools);
ERBSLAND_DEFINE_STRING_CONVERSION(U16StringEditor, char16_t, U16StringEncodingTools);
ERBSLAND_DEFINE_STRING_CONVERSION(U16String, char16_t, U16StringEncodingTools);
ERBSLAND_DEFINE_STRING_CONVERSION(U32StringEditor, char32_t, U32StringEncodingTools);
ERBSLAND_DEFINE_STRING_CONVERSION(U32String, char32_t, U32StringEncodingTools);

#undef ERBSLAND_DEFINE_STRING_CONVERSION

auto StringConversionTools::decodeU8String(
    const mem::ByteBlock &data, const StringEncoding encoding, const StringBomMode bomMode, const EncodingMode mode)
    -> U8StringEditor {
    return U8StringEncodingTools::decode(data, encoding, bomMode, mode);
}

auto StringConversionTools::decodeU16String(
    const mem::ByteBlock &data, const StringEncoding encoding, const StringBomMode bomMode, const EncodingMode mode)
    -> U16StringEditor {
    return U16StringEncodingTools::decode(data, encoding, bomMode, mode);
}

auto StringConversionTools::decodeU32String(
    const mem::ByteBlock &data, const StringEncoding encoding, const StringBomMode bomMode, const EncodingMode mode)
    -> U32StringEditor {
    return U32StringEncodingTools::decode(data, encoding, bomMode, mode);
}

void StringConversionTools::validateEncodedData(
    const mem::ByteBlock &data, const StringEncoding encoding, const StringBomMode bomMode) {
    U8StringEncodingTools::validate(data, encoding, bomMode);
}

}
