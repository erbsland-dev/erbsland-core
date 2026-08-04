// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::util {

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
HashSet<tKey, tHash, tEqual, tSelf>::HashSet() : _storage{defaultStorage()} {
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::fromList(const List<Key> &values) -> Self {
    auto data = Raw{};
    data.reserve(values.count().toSizeTOrThrow());
    for (const auto &value : values) {
        data.insert(value);
    }
    return makeSelf(std::move(data));
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
HashSet<tKey, tHash, tEqual, tSelf>::HashSet(std::initializer_list<Key> values) : _storage{defaultStorage()} {
    auto data = Raw{};
    data.reserve(values.size());
    for (const auto &value : values) {
        data.insert(value);
    }
    _storage.setData(std::move(data));
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
HashSet<tKey, tHash, tEqual, tSelf>::HashSet(const Raw &raw) : _storage{Storage::from(raw)} {
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
HashSet<tKey, tHash, tEqual, tSelf>::HashSet(Raw &&raw) : _storage{Storage::from(std::move(raw))} {
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::raw() const noexcept -> const Raw & {
    return _storage.data();
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::toRawValue() const noexcept -> const Raw & {
    return raw();
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::toList() const -> List<Key> {
    return List<Key>{toStdVector()};
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::toStdVector() const -> std::vector<Key> {
    const auto &data = raw();
    return {data.begin(), data.end()};
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::toStdSet() const -> std::set<Key> {
    const auto &data = raw();
    return {data.begin(), data.end()};
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::toStdUnorderedSet() const -> Raw {
    return raw();
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::mutableRaw() -> Raw & {
    return _storage.detachedData();
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::self() noexcept -> Self & {
    return static_cast<Self &>(*this);
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::self() const noexcept -> const Self & {
    return static_cast<const Self &>(*this);
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::makeSelf(Raw raw) -> Self {
    return Self{std::move(raw)};
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::defaultStorage() -> Storage {
    if constexpr (std::is_empty_v<Hash> && std::is_empty_v<Equal>) {
        return Storage::sharedDefault();
    } else {
        return Storage{};
    }
}

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto HashSet<tKey, tHash, tEqual, tSelf>::countToSize(const Count count) -> std::size_t {
    return count.toSizeTOrThrow();
}

}
