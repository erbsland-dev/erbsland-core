// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <functional>

namespace erbsland::util {

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::remove(const Key &key) -> Self & {
    mutableRaw().erase(key);
    return self();
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::tryRemove(const Key &key) -> bool {
    return mutableRaw().erase(key) > 0;
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
template <typename Function>
auto HashSet<tKey, tHash, tEqual, tSelf>::removeIf(Function function) -> Self & {
    auto &data = mutableRaw();
    for (auto iterator = data.begin(); iterator != data.end();) {
        if (std::invoke(function, *iterator)) {
            iterator = data.erase(iterator);
        } else {
            ++iterator;
        }
    }
    return self();
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::removed(const Key &key) const -> Self {
    auto result = self();
    result.remove(key);
    return result;
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
template <typename Function>
auto HashSet<tKey, tHash, tEqual, tSelf>::removedIf(Function function) const -> Self {
    auto result = self();
    result.removeIf(function);
    return result;
}

}
