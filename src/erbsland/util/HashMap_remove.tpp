// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <functional>

namespace erbsland::util {

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::remove(const Key &key) -> Self & {
    mutableRaw().erase(key);
    return self();
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::removeIf(Function function) -> Self & {
    auto &data = mutableRaw();
    for (auto iterator = data.begin(); iterator != data.end();) {
        if (std::invoke(function, iterator->first, iterator->second)) {
            iterator = data.erase(iterator);
        } else {
            ++iterator;
        }
    }
    return self();
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::removeIfKey(Function function) -> Self & {
    return removeIf([&](const Key &key, const Value &) -> bool { return std::invoke(function, key); });
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::removeIfValue(Function function) -> Self & {
    return removeIf([&](const Key &, const Value &value) -> bool { return std::invoke(function, value); });
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::removed(const Key &key) const -> Self {
    auto result = self();
    result.remove(key);
    return result;
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::removedIf(Function function) const -> Self {
    auto result = self();
    result.removeIf(function);
    return result;
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::removedIfKey(Function function) const -> Self {
    auto result = self();
    result.removeIfKey(function);
    return result;
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::removedIfValue(Function function) const -> Self {
    auto result = self();
    result.removeIfValue(function);
    return result;
}

}
