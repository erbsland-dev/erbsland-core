// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <algorithm>
#include <functional>

namespace erbsland::util {

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::remove(const Index index) -> Self & {
    auto &data = mutableRaw();
    if (validIndex(index, data.size())) {
        data.erase(data.begin() + static_cast<typename Raw::difference_type>(index.toSizeT()));
    }
    return self();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::remove(const Range range) -> Self & {
    auto &data = mutableRaw();
    const auto boundedRange = clampedRange(range, Count::fromSizeT(data.size()));
    if (!boundedRange.isValid() || boundedRange.isEmpty()) {
        return self();
    }
    const auto begin = boundedRange.index().toSizeT();
    const auto end = boundedRange.endIndex().toSizeT();
    data.erase(
        data.begin() + static_cast<typename Raw::difference_type>(begin),
        data.begin() + static_cast<typename Raw::difference_type>(end));
    return self();
}

template <typename tElement, typename tSelf>
template <typename Function>
auto List<tElement, tSelf>::removeIf(Function function) -> Self & {
    auto &data = mutableRaw();
    const auto newEnd = std::remove_if(
        data.begin(), data.end(), [&](const Element &value) -> bool { return std::invoke(function, value); });
    data.erase(newEnd, data.end());
    return self();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::removed(const Index index) const -> Self {
    auto result = self();
    result.remove(index);
    return result;
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::removed(const Range range) const -> Self {
    auto result = self();
    result.remove(range);
    return result;
}

template <typename tElement, typename tSelf>
template <typename Function>
auto List<tElement, tSelf>::removedIf(Function function) const -> Self {
    auto result = self();
    result.removeIf(function);
    return result;
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::removeFirst() -> Self & {
    return remove(Index::zero());
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::removeLast() -> Self & {
    if (!raw().empty()) {
        remove(Index::fromSizeT(raw().size() - 1U));
    }
    return self();
}

}
