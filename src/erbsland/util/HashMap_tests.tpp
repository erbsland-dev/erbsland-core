// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <functional>

namespace erbsland::util {

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::compare(const Self &other) const -> bool {
    const auto &left = raw();
    const auto &right = static_cast<const HashMap &>(other).raw();
    if (left.size() != right.size()) {
        return false;
    }
    for (const auto &[key, value] : left) {
        const auto iterator = right.find(key);
        if (iterator == right.end() || iterator->second != value) {
            return false;
        }
    }
    return true;
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::compareKeys(const Self &other) const -> bool {
    const auto &left = raw();
    const auto &right = static_cast<const HashMap &>(other).raw();
    if (left.size() != right.size()) {
        return false;
    }
    for (const auto &[key, value] : left) {
        static_cast<void>(value);
        if (right.find(key) == right.end()) {
            return false;
        }
    }
    return true;
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::contains(const Key &key) const -> bool {
    return raw().find(key) != raw().end();
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::allOf(Function function) const -> bool {
    for (const auto &[key, value] : raw()) {
        if (!std::invoke(function, key, value)) {
            return false;
        }
    }
    return true;
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::anyOf(Function function) const -> bool {
    for (const auto &[key, value] : raw()) {
        if (std::invoke(function, key, value)) {
            return true;
        }
    }
    return false;
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::noneOf(Function function) const -> bool {
    return !anyOf(function);
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::begin() const noexcept -> const_iterator {
    return raw().begin();
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::end() const noexcept -> const_iterator {
    return raw().end();
}

}
