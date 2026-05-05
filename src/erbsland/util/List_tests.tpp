// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <algorithm>
#include <functional>

namespace erbsland::util {

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::isEmpty() const -> bool {
    return raw().empty();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::compare(const Self &other) const -> std::strong_ordering {
    const auto &left = raw();
    const auto &right = other.raw();
    const auto sharedSize = std::min(left.size(), right.size());
    for (auto index = std::size_t{0}; index < sharedSize; ++index) {
        const auto order = left[index] <=> right[index];
        if (order < 0) {
            return std::strong_ordering::less;
        }
        if (order > 0) {
            return std::strong_ordering::greater;
        }
    }
    if (left.size() < right.size()) {
        return std::strong_ordering::less;
    }
    if (left.size() > right.size()) {
        return std::strong_ordering::greater;
    }
    return std::strong_ordering::equal;
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::contains(const Element &value) const -> bool {
    return !findFirst(value).isNoIndex();
}

template <typename tElement, typename tSelf>
template <typename Function>
auto List<tElement, tSelf>::allOf(Function function) const -> bool {
    for (const auto &value : raw()) {
        if (!std::invoke(function, value)) {
            return false;
        }
    }
    return true;
}

template <typename tElement, typename tSelf>
template <typename Function>
auto List<tElement, tSelf>::anyOf(Function function) const -> bool {
    for (const auto &value : raw()) {
        if (std::invoke(function, value)) {
            return true;
        }
    }
    return false;
}

template <typename tElement, typename tSelf>
template <typename Function>
auto List<tElement, tSelf>::noneOf(Function function) const -> bool {
    return !anyOf(function);
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::begin() const noexcept -> const_iterator {
    return raw().begin();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::end() const noexcept -> const_iterator {
    return raw().end();
}

}
