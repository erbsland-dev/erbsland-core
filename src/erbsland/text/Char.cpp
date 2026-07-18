// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Char.hpp"

#include "u16/impl/U16Encoding.hpp"
#include "u8/impl/U8Encoding.hpp"

namespace erbsland::text {

auto Char::utf8Size() const noexcept -> unit::ByteLength {
    return impl::utf8::encodedLength(*this);
}

auto Char::utf16Size() const noexcept -> unit::U16DataLength {
    return impl::utf16::encodedLength(*this);
}

auto Char::encodedSize(const StringKind stringKind) const noexcept -> std::size_t {
    switch (stringKind) {
    case StringKind::U8:
        return utf8Size().toSizeT();
    case StringKind::U16:
        return utf16Size().toSizeT();
    default:
        return 1U;
    }
}

auto Char::encodedBytes(const StringEncoding encoding) const noexcept -> unit::ByteLength {
    if (!isValidUnicode()) {
        return unit::ByteLength::zero();
    }
    switch (encoding.effectiveEncoding().toRawValue()) {
    case StringEncoding::Utf8:
        return utf8Size();
    case StringEncoding::Utf16LittleEndian:
    case StringEncoding::Utf16BigEndian:
        return unit::ByteLength{static_cast<unit::ByteLength::Value>(utf16Size().toRawValue() * 2U)};
    case StringEncoding::Utf32LittleEndian:
    case StringEncoding::Utf32BigEndian:
        return unit::ByteLength{4U};
    case StringEncoding::Utf16:
    case StringEncoding::Utf32:
        break;
    }
    return unit::ByteLength::zero();
}

}
