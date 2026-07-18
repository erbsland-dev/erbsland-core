// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::text {

template <math::AnyIntegerType T>
auto U8String::toInteger(T defaultValue, IntegerParseOptions options) const noexcept -> T {
    return impl::parseIntegerOrDefault<T>(StringCharReader{*this}, defaultValue, options);
}

template <math::AnyIntegerType T>
auto U8String::toIntegerOrThrow(IntegerParseOptions options) const -> T {
    return impl::parseInteger<T>(StringCharReader{*this}, options);
}

template <math::AnyIntegerType T>
auto U8String::fromInteger(T value, IntegerFormat format) -> U8String {
    auto storage = impl::U8StringSharedStorage{};
    auto appendTools = impl::U8StringAppendTools{storage};
    impl::appendInteger(appendTools, value, format);
    return U8String{impl::U8StringStorage{std::move(storage)}};
}

}
