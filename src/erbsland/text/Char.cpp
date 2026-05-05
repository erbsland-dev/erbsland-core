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

}
