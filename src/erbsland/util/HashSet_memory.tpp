// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cmath>

namespace erbsland::util {

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::reserve(const Count count) -> Self & {
    mutableRaw().reserve(countToSize(count));
    return self();
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::capacity() const noexcept -> Count {
    const auto &data = raw();
    const auto capacity = std::floor(static_cast<double>(data.bucket_count()) * data.max_load_factor());
    return Count::fromSizeT(static_cast<std::size_t>(capacity));
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::shrinkToFit() -> Self & {
    mutableRaw().rehash(0);
    return self();
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::clear() -> Self & {
    mutableRaw().clear();
    return self();
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::swap(Self &other) noexcept -> Self & {
    _storage.swap(static_cast<HashSet &>(other)._storage);
    return self();
}

}
