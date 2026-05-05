// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/U8StringCharReadTool.hpp"
#include "impl/U8StringDataView.hpp"
#include "impl/U8StringReadTools.hpp"

namespace erbsland::text {

template <typename tChar>
auto U8StringLiteral<tChar>::isEmpty() const noexcept -> bool {
    return impl::U8StringReadTools{dataView()}.isEmpty();
}

template <typename tChar>
auto U8StringLiteral<tChar>::isValidUtf8() const noexcept -> bool {
    return impl::U8StringReadTools{dataView()}.isValidUtf8();
}

template <typename tChar>
auto U8StringLiteral<tChar>::length() const noexcept -> unit::ByteLength {
    return impl::U8StringReadTools{dataView()}.byteLength();
}

template <typename tChar>
auto U8StringLiteral<tChar>::characterLength() const noexcept -> unit::CpLength {
    return impl::U8StringCharReadTool{dataView()}.charLength();
}

template <typename tChar>
auto U8StringLiteral<tChar>::dataView() const noexcept -> impl::U8StringDataView {
    if constexpr (std::is_same_v<tChar, char8_t>) {
        return impl::U8StringDataView{
            std::span{reinterpret_cast<const char *>(_charPtr), _size}, unit::ByteRange::fromSizeT(_size)};
    } else {
        return impl::U8StringDataView{std::span{_charPtr, _size}, unit::ByteRange::fromSizeT(_size)};
    }
}

}
