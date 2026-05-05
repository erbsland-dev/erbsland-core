// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32String.hpp"

#include "U32StringView.hpp"

#include "../impl/FloatConversion.hpp"
#include "../StringConverter.hpp"

namespace erbsland::text {

auto U32String::fromFloat(const double value, const FloatFormat format) -> U32String {
    const auto text = impl::formatFloat(value, format);
    return StringConverter{text}.toU32String();
}

template <impl::AnyFloatType T>
auto U32String::toFloat(T defaultValue, FloatParseOptions options) const noexcept -> T {
    if constexpr (std::same_as<T, float>) {
        return impl::parseFloatOrDefault(StringCharReader{*this}, defaultValue, options);
    } else {
        return impl::parseDoubleOrDefault(StringCharReader{*this}, defaultValue, options);
    }
}

template <impl::AnyFloatType T>
auto U32String::toFloatOrThrow(FloatParseOptions options) const -> T {
    if constexpr (std::same_as<T, float>) {
        return impl::parseFloatOrThrow(StringCharReader{*this}, options);
    } else {
        return impl::parseDoubleOrThrow(StringCharReader{*this}, options);
    }
}

template <impl::AnyFloatType T>
auto U32StringView::toFloat(T defaultValue, FloatParseOptions options) const noexcept -> T {
    if constexpr (std::same_as<T, float>) {
        return impl::parseFloatOrDefault(StringCharReader{*this}, defaultValue, options);
    } else {
        return impl::parseDoubleOrDefault(StringCharReader{*this}, defaultValue, options);
    }
}

template <impl::AnyFloatType T>
auto U32StringView::toFloatOrThrow(FloatParseOptions options) const -> T {
    if constexpr (std::same_as<T, float>) {
        return impl::parseFloatOrThrow(StringCharReader{*this}, options);
    } else {
        return impl::parseDoubleOrThrow(StringCharReader{*this}, options);
    }
}

template auto U32String::toFloat<float>(float defaultValue, FloatParseOptions options) const noexcept -> float;
template auto U32String::toFloat<double>(double defaultValue, FloatParseOptions options) const noexcept -> double;
template auto U32String::toFloatOrThrow<float>(FloatParseOptions options) const -> float;
template auto U32String::toFloatOrThrow<double>(FloatParseOptions options) const -> double;

template auto U32StringView::toFloat<float>(float defaultValue, FloatParseOptions options) const noexcept -> float;
template auto U32StringView::toFloat<double>(double defaultValue, FloatParseOptions options) const noexcept -> double;
template auto U32StringView::toFloatOrThrow<float>(FloatParseOptions options) const -> float;
template auto U32StringView::toFloatOrThrow<double>(FloatParseOptions options) const -> double;

}
