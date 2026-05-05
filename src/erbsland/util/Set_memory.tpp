// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::util {

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::reserve(Count) -> Self & {
    return self();
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::capacity() const noexcept -> Count {
    return count();
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::shrinkToFit() -> Self & {
    return self();
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::clear() -> Self & {
    mutableRaw().clear();
    return self();
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::swap(Self &other) noexcept -> Self & {
    _storage.swap(static_cast<Set &>(other)._storage);
    return self();
}

}
