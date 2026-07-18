// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::text {

template <math::AnyIntegerType T>
auto StringTree::append(String label, const T value, const IntegerFormat format) -> StringTree & {
    return append(std::move(label), String::fromInteger(value, format));
}

template <typename T>
    requires requires(const T &value) { value.toRawValue(); } && (!math::AnyIntegerType<T>)
auto StringTree::append(String label, const T &value, const IntegerFormat format) -> StringTree & {
    return append(std::move(label), value.toRawValue(), format);
}

template <std::ranges::input_range Range, typename Convert>
auto StringTree::appendList(String label, Range &&range, Convert convert) -> StringTree & {
    auto listTree = StringTree{};
    auto index = std::size_t{0U};
    for (auto &&value : range) {
        listTree.append(listIndexLabel(index), convert(value));
        ++index;
    }
    return append(std::move(label), std::move(listTree));
}

}
