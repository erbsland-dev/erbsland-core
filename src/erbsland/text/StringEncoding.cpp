// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringEncoding.hpp"

#include "../mem/ByteArray.hpp"

namespace erbsland::text {

auto StringEncoding::bomLength(const StringBomMode mode) const noexcept -> unit::ByteLength {
    return unit::ByteLength::fromSizeT(bomBytes(mode).size());
}

auto StringEncoding::bomBytes(const StringBomMode mode) const noexcept -> mem::ConstByteSpan {
    static constexpr auto cUtf8Bom = mem::ByteArray{mem::Byte{0xefU}, mem::Byte{0xbbU}, mem::Byte{0xbfU}};
    static constexpr auto cUtf16LittleEndianBom = mem::ByteArray{mem::Byte{0xffU}, mem::Byte{0xfeU}};
    static constexpr auto cUtf16BigEndianBom = mem::ByteArray{mem::Byte{0xfeU}, mem::Byte{0xffU}};
    static constexpr auto cUtf32LittleEndianBom =
        mem::ByteArray{mem::Byte{0xffU}, mem::Byte{0xfeU}, mem::Byte{0x00U}, mem::Byte{0x00U}};
    static constexpr auto cUtf32BigEndianBom =
        mem::ByteArray{mem::Byte{0x00U}, mem::Byte{0x00U}, mem::Byte{0xfeU}, mem::Byte{0xffU}};

    if (!writesBom(mode)) {
        return {};
    }
    switch (effectiveEncoding().toRawValue()) {
    case Utf8:
        return cUtf8Bom.span();
    case Utf16LittleEndian:
        return cUtf16LittleEndianBom.span();
    case Utf16BigEndian:
        return cUtf16BigEndianBom.span();
    case Utf32LittleEndian:
        return cUtf32LittleEndianBom.span();
    case Utf32BigEndian:
        return cUtf32BigEndianBom.span();
    case Utf16:
    case Utf32:
        break;
    }
    return {};
}

}
