// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <functional>

namespace erbsland::util {

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::compare(const Self &other) const -> bool {
    const auto &left = raw();
    const auto &right = static_cast<const HashSet &>(other).raw();
    if (left.size() != right.size()) {
        return false;
    }
    for (const auto &key : left) {
        if (right.find(key) == right.end()) {
            return false;
        }
    }
    return true;
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::contains(const Key &key) const -> bool {
    return raw().find(key) != raw().end();
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
template <typename Function>
auto HashSet<tKey, tHash, tEqual, tSelf>::allOf(Function function) const -> bool {
    for (const auto &key : raw()) {
        if (!std::invoke(function, key)) {
            return false;
        }
    }
    return true;
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
template <typename Function>
auto HashSet<tKey, tHash, tEqual, tSelf>::anyOf(Function function) const -> bool {
    for (const auto &key : raw()) {
        if (std::invoke(function, key)) {
            return true;
        }
    }
    return false;
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
template <typename Function>
auto HashSet<tKey, tHash, tEqual, tSelf>::noneOf(Function function) const -> bool {
    return !anyOf(function);
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::isSubsetOf(const Self &other) const -> bool {
    for (const auto &key : raw()) {
        if (!other.contains(key)) {
            return false;
        }
    }
    return true;
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::isSupersetOf(const Self &other) const -> bool {
    return other.isSubsetOf(self());
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::isDisjointWith(const Self &other) const -> bool {
    const auto &left = raw();
    const auto &right = static_cast<const HashSet &>(other).raw();
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

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::intersects(const Self &other) const -> bool {
    return !isDisjointWith(other);
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::begin() const noexcept -> const_iterator {
    return raw().begin();
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::end() const noexcept -> const_iterator {
    return raw().end();
}

}
