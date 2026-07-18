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
    auto storage = impl::U32StringSharedStorage{};
    auto appendTools = impl::U32StringAppendTools{storage};
    impl::appendInteger(appendTools, value, format);
    return U32String{impl::U32StringStorage{std::move(storage)}};
}

}
