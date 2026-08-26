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

}
