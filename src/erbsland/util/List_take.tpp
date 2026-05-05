// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <functional>

namespace erbsland::util {

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::take(const Index index) -> Element {
    auto &data = mutableRaw();
    if (!validIndex(index, data.size())) {
        return {};
    }
    const auto position = data.begin() + static_cast<typename Raw::difference_type>(index.toSizeT());
    auto result = std::move(*position);
    data.erase(position);
    return result;
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::take(const Range range) -> Self {
    auto &data = mutableRaw();
    const auto boundedRange = clampedRange(range, Count::fromSizeT(data.size()));
    if (!boundedRange.isValid() || boundedRange.isEmpty()) {
        return {};
    }
    const auto begin = boundedRange.index().toSizeT();
    const auto end = boundedRange.endIndex().toSizeT();
    auto result =
        Raw{data.begin() + static_cast<typename Raw::difference_type>(begin),
            data.begin() + static_cast<typename Raw::difference_type>(end)};
    data.erase(
        data.begin() + static_cast<typename Raw::difference_type>(begin),
        data.begin() + static_cast<typename Raw::difference_type>(end));
    return makeSelf(std::move(result));
}

template <typename tElement, typename tSelf>
template <typename Function>
auto List<tElement, tSelf>::takeIf(Function function) -> Self {
    auto &data = mutableRaw();
    auto taken = Raw{};
    auto kept = Raw{};
    taken.reserve(data.size());
    kept.reserve(data.size());
    for (auto &value : data) {
        if (std::invoke(function, value)) {
            taken.push_back(std::move(value));
        } else {
            kept.push_back(std::move(value));
        }
    }
    data = std::move(kept);
    return makeSelf(std::move(taken));
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::takeFirst() -> Element {
    return take(Index::zero());
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::takeLast() -> Element {
    if (raw().empty()) {
        return {};
    }
    return take(Index::fromSizeT(raw().size() - 1U));
}

}
