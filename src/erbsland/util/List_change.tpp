// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::util {

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::insert(const Index index, const Element &value) -> Self & {
    auto &data = mutableRaw();
    const auto position = validIndex(index, data.size()) ? index.toSizeT() : data.size();
    data.insert(data.begin() + static_cast<typename Raw::difference_type>(position), value);
    return self();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::insert(const Index index, Element &&value) -> Self & {
    auto &data = mutableRaw();
    const auto position = validIndex(index, data.size()) ? index.toSizeT() : data.size();
    data.insert(data.begin() + static_cast<typename Raw::difference_type>(position), std::move(value));
    return self();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::insert(const Index index, const Self &other) -> Self & {
    auto values = other.toStdVector();
    auto &data = mutableRaw();
    const auto position = validIndex(index, data.size()) ? index.toSizeT() : data.size();
    data.insert(data.begin() + static_cast<typename Raw::difference_type>(position), values.begin(), values.end());
    return self();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::append(const Element &value) -> Self & {
    mutableRaw().push_back(value);
    return self();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::append(Element &&value) -> Self & {
    mutableRaw().push_back(std::move(value));
    return self();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::append(const Self &other) -> Self & {
    auto values = other.toStdVector();
    auto &data = mutableRaw();
    data.insert(data.end(), values.begin(), values.end());
    return self();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::prepend(const Element &value) -> Self & {
    auto &data = mutableRaw();
    data.insert(data.begin(), value);
    return self();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::prepend(Element &&value) -> Self & {
    auto &data = mutableRaw();
    data.insert(data.begin(), std::move(value));
    return self();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::prepend(const Self &other) -> Self & {
    return insert(Index::zero(), other);
}

}
