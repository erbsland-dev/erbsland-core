// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerTraits.hpp"

namespace erbsland::math {

template <NativeInteger tValue>
template <AnyIntegerType tOther>
constexpr SaturatingInteger<tValue>::SaturatingInteger(tOther value) noexcept :
    _value{saturatingCast<NativeValue>(convertToNativeInt(value))} {
}

template <NativeInteger tValue>
auto SaturatingInteger<tValue>::operator++() noexcept -> SaturatingInteger & {
    saturatingIncrement(_value);
    return *this;
}

template <NativeInteger tValue>
auto SaturatingInteger<tValue>::operator++(int) noexcept -> SaturatingInteger {
    auto tmp = *this;
    saturatingIncrement(_value);
    return tmp;
}

template <NativeInteger tValue>
auto SaturatingInteger<tValue>::operator--() noexcept -> SaturatingInteger & {
    saturatingDecrement(_value);
    return *this;
}

template <NativeInteger tValue>
auto SaturatingInteger<tValue>::operator--(int) noexcept -> SaturatingInteger {
    auto tmp = *this;
    saturatingDecrement(_value);
    return tmp;
}

}
