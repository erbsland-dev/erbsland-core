// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <functional>

namespace erbsland::util {

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::compare(const Self &other) const -> bool {
    const auto &left = raw();
    const auto &right = static_cast<const Set &>(other).raw();
    if (left.size() != right.size()) {
        return false;
    }
    const auto compare = left.key_comp();
    auto leftIterator = left.begin();
    auto rightIterator = right.begin();
    while (leftIterator != left.end()) {
        if (compare(*leftIterator, *rightIterator) || compare(*rightIterator, *leftIterator)) {
            return false;
        }
        ++leftIterator;
        ++rightIterator;
    }
    return true;
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::contains(const Key &key) const -> bool {
    return raw().find(key) != raw().end();
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
template <typename Function>
auto Set<tKey, tCompare, tSelf>::allOf(Function function) const -> bool {
    for (const auto &key : raw()) {
        if (!std::invoke(function, key)) {
            return false;
        }
    }
    return true;
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
template <typename Function>
auto Set<tKey, tCompare, tSelf>::anyOf(Function function) const -> bool {
    for (const auto &key : raw()) {
        if (std::invoke(function, key)) {
            return true;
        }
    }
    return false;
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
template <typename Function>
auto Set<tKey, tCompare, tSelf>::noneOf(Function function) const -> bool {
    return !anyOf(function);
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::isSubsetOf(const Self &other) const -> bool {
    for (const auto &key : raw()) {
        if (!other.contains(key)) {
            return false;
        }
    }
    return true;
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::isSupersetOf(const Self &other) const -> bool {
    return other.isSubsetOf(self());
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::isDisjointWith(const Self &other) const -> bool {
    const auto &left = raw();
    const auto &right = static_cast<const Set &>(other).raw();
    const auto *smaller = &left;
    const auto *larger = &right;
    if (right.size() < left.size()) {
        smaller = &right;
        larger = &left;
    }
    for (const auto &key : *smaller) {
        if (larger->find(key) != larger->end()) {
            return false;
        }
    }
    return true;
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::intersects(const Self &other) const -> bool {
    return !isDisjointWith(other);
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::begin() const noexcept -> const_iterator {
    return raw().begin();
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::end() const noexcept -> const_iterator {
    return raw().end();
}

}
