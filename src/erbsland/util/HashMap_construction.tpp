// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::util {

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
HashMap<tKey, tValue, tHash, tEqual, tSelf>::HashMap() : _storage{} {
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
HashMap<tKey, tValue, tHash, tEqual, tSelf>::HashMap(std::initializer_list<Entry> values) {
    auto data = Raw{};
    for (const auto &[key, value] : values) {
        data[key] = value;
    }
    _storage.setData(std::move(data));
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
HashMap<tKey, tValue, tHash, tEqual, tSelf>::HashMap(const Raw &raw) : _storage{Storage::from(raw)} {
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
HashMap<tKey, tValue, tHash, tEqual, tSelf>::HashMap(Raw &&raw) : _storage{Storage::from(std::move(raw))} {
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::raw() const noexcept -> const Raw & {
    return _storage.data();
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::toRawValue() const noexcept -> const Raw & {
    return raw();
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::toStdMap() const -> std::map<Key, Value> {
    auto result = std::map<Key, Value>{};
    for (const auto &[key, value] : raw()) {
        result.emplace(key, value);
    }
    return result;
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::toStdUnorderedMap() const -> Raw {
    return raw();
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::toStdKeyVector() const -> std::vector<Key> {
    auto result = std::vector<Key>{};
    result.reserve(raw().size());
    for (const auto &[key, value] : raw()) {
        static_cast<void>(value);
        result.push_back(key);
    }
    return result;
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::toStdVector() const -> std::vector<Entry> {
    auto result = std::vector<Entry>{};
    result.reserve(raw().size());
    for (const auto &[key, value] : raw()) {
        result.emplace_back(key, value);
    }
    return result;
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::mutableRaw() -> Raw & {
    return _storage.detachedData();
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::self() noexcept -> Self & {
    return static_cast<Self &>(*this);
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::self() const noexcept -> const Self & {
    return static_cast<const Self &>(*this);
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::makeSelf(Raw raw) -> Self {
    return Self{std::move(raw)};
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::countToSize(const Count count) -> std::size_t {
    return count.toSizeTOrThrow();
}

}
