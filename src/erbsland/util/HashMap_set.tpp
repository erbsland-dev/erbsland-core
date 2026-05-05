// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::util {

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::toKeySet() const -> Set<Key> {
    return Set<Key>::fromList(toKeyList());
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::toKeyHashSet() const -> HashSet<Key, Hash, Equal> {
    auto result = typename HashSet<Key, Hash, Equal>::Raw{raw().bucket_count(), raw().hash_function(), raw().key_eq()};
    for (const auto &[key, value] : raw()) {
        static_cast<void>(value);
        result.insert(key);
    }
    return HashSet<Key, Hash, Equal>{std::move(result)};
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::toValueSet() const -> Set<Value> {
    return Set<Value>::fromList(toValueList());
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::toValueHashSet() const -> HashSet<Value> {
    return HashSet<Value>::fromList(toValueList());
}

}
