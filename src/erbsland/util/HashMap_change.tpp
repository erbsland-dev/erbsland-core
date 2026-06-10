// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::util {

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename tValueFwd>
    requires std::is_constructible_v<tValue, tValueFwd>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::set(const Key &key, tValueFwd &&value) -> Self & {
    auto &data = mutableRaw();
    if (auto iterator = data.find(key); iterator != data.end()) {
        data.erase(iterator);
    }
    data.emplace(key, std::forward<tValueFwd>(value));
    return self();
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::tryReplace(const Key &key, const Value &value) -> bool {
    auto &data = mutableRaw();
    const auto iterator = data.find(key);
    if (iterator == data.end()) {
        return false;
    }
    data.erase(iterator);
    data.emplace(key, value);
    return true;
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::tryInsert(const Key &key, const Value &value) -> bool {
    return mutableRaw().emplace(key, value).second;
}

}
