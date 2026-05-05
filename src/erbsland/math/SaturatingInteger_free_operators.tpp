// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::math {

template <NativeInteger tValue>
    requires std::signed_integral<tValue>
constexpr auto operator-(SaturatingInteger<tValue> value) noexcept -> SaturatingInteger<tValue> {
    return value.negated();
}

template <NativeInteger tFirst, NativeInteger tSecond>
constexpr auto operator==(tFirst first, SaturatingInteger<tSecond> second) noexcept -> bool {
    return SaturatingInteger<tFirst>{first} == second;
}

template <NativeInteger tFirst, NativeInteger tSecond>
constexpr auto operator!=(tFirst first, SaturatingInteger<tSecond> second) noexcept -> bool {
    return SaturatingInteger<tFirst>{first} != second;
}

template <NativeInteger tFirst, NativeInteger tSecond>
constexpr auto operator<(tFirst first, SaturatingInteger<tSecond> second) noexcept -> bool {
    return SaturatingInteger<tFirst>{first} < second;
}

template <NativeInteger tFirst, NativeInteger tSecond>
constexpr auto operator<=(tFirst first, SaturatingInteger<tSecond> second) noexcept -> bool {
    return SaturatingInteger<tFirst>{first} <= second;
}

template <NativeInteger tFirst, NativeInteger tSecond>
constexpr auto operator>(tFirst first, SaturatingInteger<tSecond> second) noexcept -> bool {
    return SaturatingInteger<tFirst>{first} > second;
}

template <NativeInteger tFirst, NativeInteger tSecond>
constexpr auto operator>=(tFirst first, SaturatingInteger<tSecond> second) noexcept -> bool {
    return SaturatingInteger<tFirst>{first} >= second;
}

template <NativeInteger tFirst, NativeInteger tSecond>
    requires SameSignednessNativeIntegers<tFirst, tSecond>
auto operator+(tFirst first, SaturatingInteger<tSecond> second) noexcept
    -> decltype(SaturatingInteger<tFirst>{first} + second) {
    return SaturatingInteger<tFirst>{first} + second;
}

template <NativeInteger tFirst, NativeInteger tSecond>
    requires SameSignednessNativeIntegers<tFirst, tSecond>
auto operator-(tFirst first, SaturatingInteger<tSecond> second) noexcept
    -> decltype(SaturatingInteger<tFirst>{first} - second) {
    return SaturatingInteger<tFirst>{first} - second;
}

template <NativeInteger tFirst, NativeInteger tSecond>
    requires SameSignednessNativeIntegers<tFirst, tSecond>
auto operator*(tFirst first, SaturatingInteger<tSecond> second) noexcept
    -> decltype(SaturatingInteger<tFirst>{first} * second) {
    return SaturatingInteger<tFirst>{first} * second;
}

template <NativeInteger tFirst, NativeInteger tSecond>
    requires SameSignednessNativeIntegers<tFirst, tSecond>
auto operator/(tFirst first, SaturatingInteger<tSecond> second) noexcept
    -> decltype(SaturatingInteger<tFirst>{first} / second) {
    return SaturatingInteger<tFirst>{first} / second;
}

template <NativeInteger tFirst, NativeInteger tSecond>
    requires SameSignednessNativeIntegers<tFirst, tSecond>
auto operator%(tFirst first, SaturatingInteger<tSecond> second) noexcept
    -> decltype(SaturatingInteger<tFirst>{first} % second) {
    return SaturatingInteger<tFirst>{first} % second;
}

}
