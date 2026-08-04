// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringEditor.hpp"

#include "../impl/FloatConversion.hpp"
#include "../StringConverter.hpp"

namespace erbsland::text {

auto U32StringEditor::fromFloat(const double value, const FloatFormat format) -> U32StringEditor {
    const auto text = impl::formatFloat(value, format);
    return U32StringEditor{StringConverter{text}.toU32String()};
}

template <impl::AnyFloatType T>
auto U32StringEditor::toFloat(T defaultValue, FloatParseOptions options) const noexcept -> T {
    if constexpr (std::same_as<T, float>) {
        return impl::parseFloatOrDefault(StringCharReader{*this}, defaultValue, options);
    } else {
        return impl::parseDoubleOrDefault(StringCharReader{*this}, defaultValue, options);
    }
}

template <impl::AnyFloatType T>
auto U32StringEditor::toFloatOrThrow(FloatParseOptions options) const -> T {
    if constexpr (std::same_as<T, float>) {
        return impl::parseFloatOrThrow(StringCharReader{*this}, options);
    } else {
        return impl::parseDoubleOrThrow(StringCharReader{*this}, options);
    }
}

template auto U32StringEditor::toFloat<float>(float defaultValue, FloatParseOptions options) const noexcept -> float;
template auto U32StringEditor::toFloat<double>(double defaultValue, FloatParseOptions options) const noexcept -> double;
template auto U32StringEditor::toFloatOrThrow<float>(FloatParseOptions options) const -> float;
template auto U32StringEditor::toFloatOrThrow<double>(FloatParseOptions options) const -> double;

}
