// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::util {

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::resize(const Count count) -> Self & {
    mutableRaw().resize(countToSize(count));
    return self();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::resize(const Count count, const Element &value) -> Self & {
    mutableRaw().resize(countToSize(count), value);
    return self();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::reserve(const Count count) -> Self & {
    mutableRaw().reserve(countToSize(count));
    return self();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::capacity() const noexcept -> Count {
    return Count::fromSizeT(raw().capacity());
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::shrinkToFit() -> Self & {
    mutableRaw().shrink_to_fit();
    return self();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::clear() -> Self & {
    mutableRaw().clear();
    return self();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::swap(Self &other) noexcept -> Self & {
    _storage.swap(static_cast<List &>(other)._storage);
    return self();
}

}
