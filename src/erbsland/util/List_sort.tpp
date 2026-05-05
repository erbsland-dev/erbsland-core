// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <algorithm>

namespace erbsland::util {

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::sort() -> Self & {
    auto &data = mutableRaw();
    std::sort(data.begin(), data.end());
    return self();
}

template <typename tElement, typename tSelf>
template <typename Function>
auto List<tElement, tSelf>::sort(Function function) -> Self & {
    auto &data = mutableRaw();
    std::sort(data.begin(), data.end(), function);
    return self();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::sorted() const -> Self {
    auto result = self();
    result.sort();
    return result;
}

template <typename tElement, typename tSelf>
template <typename Function>
auto List<tElement, tSelf>::sorted(Function function) const -> Self {
    auto result = self();
    result.sort(function);
    return result;
}

}
