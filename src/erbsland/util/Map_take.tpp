// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <functional>

namespace erbsland::util {

template <typename tKey, typename tValue, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto Map<tKey, tValue, tCompare, tSelf>::take(const Key &key) -> Value {
    auto &data = mutableRaw();
    const auto iterator = data.find(key);
    if (iterator == data.end()) {
        return {};
    }
    auto result = std::move(iterator->second);
    data.erase(iterator);
    return result;
}

template <typename tKey, typename tValue, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto Map<tKey, tValue, tCompare, tSelf>::takeIf(Function function) -> Self {
    auto &data = mutableRaw();
    auto taken = Raw{};
    for (auto iterator = data.begin(); iterator != data.end();) {
        if (std::invoke(function, iterator->first, iterator->second)) {
            taken.emplace(iterator->first, std::move(iterator->second));
            iterator = data.erase(iterator);
        } else {
            ++iterator;
        }
    }
    return makeSelf(std::move(taken));
}

template <typename tKey, typename tValue, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto Map<tKey, tValue, tCompare, tSelf>::takeIfKey(Function function) -> Self {
    return takeIf([&](const Key &key, const Value &) -> bool { return std::invoke(function, key); });
}

template <typename tKey, typename tValue, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto Map<tKey, tValue, tCompare, tSelf>::takeIfValue(Function function) -> Self {
    return takeIf([&](const Key &, const Value &value) -> bool { return std::invoke(function, value); });
}

}
