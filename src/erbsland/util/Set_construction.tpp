// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::util {

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
Set<tKey, tCompare, tSelf>::Set() : _storage{} {
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::fromList(const List<Key> &values) -> Self {
    auto data = Raw{};
    for (const auto &value : values) {
        data.insert(value);
    }
    return makeSelf(std::move(data));
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
Set<tKey, tCompare, tSelf>::Set(std::initializer_list<Key> values) : _storage{Storage::from(Raw{values})} {
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
Set<tKey, tCompare, tSelf>::Set(const Raw &raw) : _storage{Storage::from(raw)} {
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
Set<tKey, tCompare, tSelf>::Set(Raw &&raw) : _storage{Storage::from(std::move(raw))} {
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::raw() const noexcept -> const Raw & {
    return _storage.data();
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::toRawValue() const noexcept -> const Raw & {
    return raw();
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::toList() const -> List<Key> {
    return List<Key>{toStdVector()};
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::toStdVector() const -> std::vector<Key> {
    const auto &data = raw();
    return {data.begin(), data.end()};
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::toStdSet() const -> Raw {
    return raw();
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::toStdUnorderedSet() const -> std::unordered_set<Key> {
    auto result = std::unordered_set<Key>{};
    result.reserve(raw().size());
    for (const auto &key : raw()) {
        result.insert(key);
    }
    return result;
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::mutableRaw() -> Raw & {
    return _storage.detachedData();
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::self() noexcept -> Self & {
    return static_cast<Self &>(*this);
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::self() const noexcept -> const Self & {
    return static_cast<const Self &>(*this);
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::makeSelf(Raw raw) -> Self {
    return Self{std::move(raw)};
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
auto Set<tKey, tCompare, tSelf>::countToSize(const Count count) -> std::size_t {
    return count.toSizeTOrThrow();
}

}
