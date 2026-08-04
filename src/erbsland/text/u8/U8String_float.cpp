// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8String.hpp"

#include "../impl/FloatConversion.hpp"

namespace erbsland::text {

template <impl::AnyFloatType T>
auto U8String::toFloat(T defaultValue, FloatParseOptions options) const noexcept -> T {
    if constexpr (std::same_as<T, float>) {
        return impl::parseFloatOrDefault(StringCharReader{*this}, defaultValue, options);
    } else {
        return impl::parseDoubleOrDefault(StringCharReader{*this}, defaultValue, options);
    }
}

template <impl::AnyFloatType T>
auto U8String::toFloatOrThrow(FloatParseOptions options) const -> T {
    if constexpr (std::same_as<T, float>) {
        return impl::parseFloatOrThrow(StringCharReader{*this}, options);
    } else {
        return impl::parseDoubleOrThrow(StringCharReader{*this}, options);
    }
}

template auto U8String::toFloat<float>(float defaultValue, FloatParseOptions options) const noexcept -> float;
template auto U8String::toFloat<double>(double defaultValue, FloatParseOptions options) const noexcept -> double;
template auto U8String::toFloatOrThrow<float>(FloatParseOptions options) const -> float;
template auto U8String::toFloatOrThrow<double>(FloatParseOptions options) const -> double;

}
