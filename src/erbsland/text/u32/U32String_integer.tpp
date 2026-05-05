// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::text {

template <math::AnyIntegerType T>
auto U32String::toInteger(T defaultValue, IntegerParseOptions options) const noexcept -> T {
    return impl::parseIntegerOrDefault<T>(StringCharReader{*this}, defaultValue, options);
}

template <math::AnyIntegerType T>
auto U32String::toIntegerOrThrow(IntegerParseOptions options) const -> T {
    return impl::parseInteger<T>(StringCharReader{*this}, options);
}

template <math::AnyIntegerType T>
auto U32String::fromInteger(T value, IntegerFormat format) -> U32String {
    auto builder = StringBuilder{StringKind::U32};
    builder.appendInteger(value, format);
    return builder.takeU32String();
}

}
