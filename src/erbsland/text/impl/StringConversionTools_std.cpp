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
}
