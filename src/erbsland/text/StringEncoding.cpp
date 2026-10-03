// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringEncoding.hpp"

#include "../mem/ByteArray.hpp"

namespace erbsland::text {

using namespace mem;

auto StringEncoding::bomLength(const StringBomMode mode) const noexcept -> unit::ByteCount {
    return unit::ByteCount::fromSizeT(bomBytes(mode).size());
}

auto StringEncoding::bomBytes(const StringBomMode mode) const noexcept -> ConstByteSpan {
    static constexpr auto cUtf8Bom = ByteArray{Byte{0xefU}, Byte{0xbbU}, Byte{0xbfU}};
    static constexpr auto cUtf16LittleEndianBom = ByteArray{Byte{0xffU}, Byte{0xfeU}};
    static constexpr auto cUtf16BigEndianBom = ByteArray{Byte{0xfeU}, Byte{0xffU}};
    static constexpr auto cUtf32LittleEndianBom = ByteArray{Byte{0xffU}, Byte{0xfeU}, Byte{0x00U}, Byte{0x00U}};
    static constexpr auto cUtf32BigEndianBom = ByteArray{Byte{0x00U}, Byte{0x00U}, Byte{0xfeU}, Byte{0xffU}};

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
