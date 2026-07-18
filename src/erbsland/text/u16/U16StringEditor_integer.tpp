// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::text {

template <math::AnyIntegerType T>
auto U16StringEditor::toInteger(T defaultValue, IntegerParseOptions options) const noexcept -> T {
    return impl::parseIntegerOrDefault<T>(StringCharReader{*this}, defaultValue, options);
}

template <math::AnyIntegerType T>
auto U16StringEditor::toIntegerOrThrow(IntegerParseOptions options) const -> T {
    return impl::parseInteger<T>(StringCharReader{*this}, options);
}

template <math::AnyIntegerType T>
auto U16StringEditor::fromInteger(T value, IntegerFormat format) -> U16StringEditor {
    auto builder = AnyStringBuilder{StringKind::U16};
    builder.appendInteger(value, format);
    return builder.takeU16StringEditor();
}

}
