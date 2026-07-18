// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::text {

template <math::AnyIntegerType T>
auto U16String::toInteger(T defaultValue, IntegerParseOptions options) const noexcept -> T {
    return impl::parseIntegerOrDefault<T>(StringCharReader{*this}, defaultValue, options);
}

template <math::AnyIntegerType T>
auto U16String::toIntegerOrThrow(IntegerParseOptions options) const -> T {
    return impl::parseInteger<T>(StringCharReader{*this}, options);
}

template <math::AnyIntegerType T>
auto U16String::fromInteger(T value, IntegerFormat format) -> U16String {
    auto storage = impl::U16StringSharedStorage{};
    auto appendTools = impl::U16StringAppendTools{storage};
    impl::appendInteger(appendTools, value, format);
    return U16String{impl::U16StringStorage{std::move(storage)}};
}

}
