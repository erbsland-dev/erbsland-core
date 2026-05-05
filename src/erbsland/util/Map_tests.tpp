// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <functional>

namespace erbsland::util {

template <typename tKey, typename tValue, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto Map<tKey, tValue, tCompare, tSelf>::compare(const Self &other) const -> bool {
    const auto &left = raw();
    const auto &right = static_cast<const Map &>(other).raw();
    if (left.size() != right.size()) {
        return false;
    }
    const auto compare = left.key_comp();
    auto leftIterator = left.begin();
    auto rightIterator = right.begin();
    while (leftIterator != left.end()) {
        if (compare(leftIterator->first, rightIterator->first) || compare(rightIterator->first, leftIterator->first) ||
            leftIterator->second != rightIterator->second) {
            return false;
        }
        ++leftIterator;
        ++rightIterator;
    }
    return true;
}

template <typename tKey, typename tValue, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto Map<tKey, tValue, tCompare, tSelf>::compareKeys(const Self &other) const -> bool {
    const auto &left = raw();
    const auto &right = static_cast<const Map &>(other).raw();
    if (left.size() != right.size()) {
        return false;
    }
    const auto compare = left.key_comp();
    auto leftIterator = left.begin();
    auto rightIterator = right.begin();
    while (leftIterator != left.end()) {
        if (compare(leftIterator->first, rightIterator->first) || compare(rightIterator->first, leftIterator->first)) {
            return false;
        }
        ++leftIterator;
        ++rightIterator;
    }
    return true;
}

template <typename tKey, typename tValue, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto Map<tKey, tValue, tCompare, tSelf>::contains(const Key &key) const -> bool {
    return raw().find(key) != raw().end();
}

template <typename tKey, typename tValue, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto Map<tKey, tValue, tCompare, tSelf>::allOf(Function function) const -> bool {
    for (const auto &[key, value] : raw()) {
        if (!std::invoke(function, key, value)) {
            return false;
        }
    }
    return true;
}

template <typename tKey, typename tValue, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto Map<tKey, tValue, tCompare, tSelf>::anyOf(Function function) const -> bool {
    for (const auto &[key, value] : raw()) {
        if (std::invoke(function, key, value)) {
            return true;
        }
    }
    return false;
}

template <typename tKey, typename tValue, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto Map<tKey, tValue, tCompare, tSelf>::noneOf(Function function) const -> bool {
    return !anyOf(function);
}

template <typename tKey, typename tValue, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto Map<tKey, tValue, tCompare, tSelf>::begin() const noexcept -> const_iterator {
    return raw().begin();
}

template <typename tKey, typename tValue, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto Map<tKey, tValue, tCompare, tSelf>::end() const noexcept -> const_iterator {
    return raw().end();
}

}
