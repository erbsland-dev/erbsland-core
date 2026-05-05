// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::util {

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::unite(const Self &other) -> Self & {
    auto &data = mutableRaw();
    for (const auto &key : static_cast<const HashSet &>(other).raw()) {
        data.insert(key);
    }
    return self();
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::intersect(const Self &other) -> Self & {
    auto &data = mutableRaw();
    for (auto iterator = data.begin(); iterator != data.end();) {
        if (!other.contains(*iterator)) {
            iterator = data.erase(iterator);
        } else {
            ++iterator;
        }
    }
    return self();
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::subtract(const Self &other) -> Self & {
    auto &data = mutableRaw();
    for (const auto &key : static_cast<const HashSet &>(other).raw()) {
        data.erase(key);
    }
    return self();
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::symmetricDifference(const Self &other) -> Self & {
    for (const auto &key : static_cast<const HashSet &>(other).raw()) {
        if (!tryRemove(key)) {
            insert(key);
        }
    }
    return self();
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::unitedWith(const Self &other) const -> Self {
    auto result = self();
    result.unite(other);
    return result;
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::intersectedWith(const Self &other) const -> Self {
    auto result = self();
    result.intersect(other);
    return result;
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::subtractedBy(const Self &other) const -> Self {
    auto result = self();
    result.subtract(other);
    return result;
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::symmetricDifferenceWith(const Self &other) const -> Self {
    auto result = self();
    result.symmetricDifference(other);
    return result;
}

}
