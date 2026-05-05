// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <functional>

namespace erbsland::util {

template <typename tKey, typename tValue, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto Map<tKey, tValue, tCompare, tSelf>::remove(const Key &key) -> Self & {
    mutableRaw().erase(key);
    return self();
}

template <typename tKey, typename tValue, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto Map<tKey, tValue, tCompare, tSelf>::removeIf(Function function) -> Self & {
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

template <typename tKey, typename tValue, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto Map<tKey, tValue, tCompare, tSelf>::removeIfKey(Function function) -> Self & {
    return removeIf([&](const Key &key, const Value &) -> bool { return std::invoke(function, key); });
}

template <typename tKey, typename tValue, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto Map<tKey, tValue, tCompare, tSelf>::removeIfValue(Function function) -> Self & {
    return removeIf([&](const Key &, const Value &value) -> bool { return std::invoke(function, value); });
}

template <typename tKey, typename tValue, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto Map<tKey, tValue, tCompare, tSelf>::removed(const Key &key) const -> Self {
    auto result = self();
    result.remove(key);
    return result;
}

template <typename tKey, typename tValue, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto Map<tKey, tValue, tCompare, tSelf>::removedIf(Function function) const -> Self {
    auto result = self();
    result.removeIf(function);
    return result;
}

template <typename tKey, typename tValue, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto Map<tKey, tValue, tCompare, tSelf>::removedIfKey(Function function) const -> Self {
    auto result = self();
    result.removeIfKey(function);
    return result;
}

template <typename tKey, typename tValue, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto Map<tKey, tValue, tCompare, tSelf>::removedIfValue(Function function) const -> Self {
    auto result = self();
    result.removeIfValue(function);
    return result;
}

}
