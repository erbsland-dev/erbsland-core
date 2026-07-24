// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TimeAmounts.hpp"

namespace erbsland::time::literals {

/// Literal for nanoseconds.
inline auto operator""_ns(const unsigned long long value) -> Nanoseconds {
    return Nanoseconds{math::saturatingCast<Nanoseconds::NativeValue>(value)};
}
/// Literal for microseconds.
inline auto operator""_us(const unsigned long long value) -> Microseconds {
    return Microseconds{math::saturatingCast<Microseconds::NativeValue>(value)};
}
/// Literal for milliseconds.
inline auto operator""_ms(const unsigned long long value) -> Milliseconds {
    return Milliseconds{math::saturatingCast<Milliseconds::NativeValue>(value)};
}
/// Literal for seconds.
inline auto operator""_s(const unsigned long long value) -> Seconds {
    return Seconds{math::saturatingCast<Seconds::NativeValue>(value)};
}
/// Literal for minutes.
inline auto operator""_m(const unsigned long long value) -> Minutes {
    return Minutes{math::saturatingCast<Minutes::NativeValue>(value)};
}
/// Literal for hours.
inline auto operator""_h(const unsigned long long value) -> Hours {
    return Hours{math::saturatingCast<Hours::NativeValue>(value)};
}

}
