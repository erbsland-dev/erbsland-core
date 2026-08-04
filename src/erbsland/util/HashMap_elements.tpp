// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <functional>

namespace erbsland::util {

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::count() const noexcept -> Count {
    return Count::fromSizeT(raw().size());
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::countIf(Function function) const -> Count {
    auto result = std::size_t{0};
    for (const auto &[key, value] : raw()) {
        if (std::invoke(function, key, value)) {
            ++result;
        }
    }
    return Count::fromSizeT(result);
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::countIfKey(Function function) const -> Count {
    return countIf([&](const Key &key, const Value &) -> bool { return std::invoke(function, key); });
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::countIfValue(Function function) const -> Count {
    return countIf([&](const Key &, const Value &value) -> bool { return std::invoke(function, value); });
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::get(const Key &key) const -> std::optional<Value> {
    const auto iterator = raw().find(key);
    if (iterator == raw().end()) {
        return {};
    }
    return iterator->second;
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::get(const Key &key, const Value &defaultValue) const -> Value {
    const auto iterator = raw().find(key);
    if (iterator == raw().end()) {
        return defaultValue;
    }
    return iterator->second;
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::first() const -> Entry {
    const auto &data = raw();
    if (data.empty()) {
        return {};
    }
    const auto iterator = data.begin();
    return {iterator->first, iterator->second};
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::last() const -> Entry {
    const auto &data = raw();
    if (data.empty()) {
        return {};
    }
    auto iterator = data.begin();
    auto lastIterator = iterator;
    while (iterator != data.end()) {
        lastIterator = iterator;
        ++iterator;
    }
    return {lastIterator->first, lastIterator->second};
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::toKeyList() const -> List<Key> {
    auto result = List<Key>{};
    result.reserve(count());
    for (const auto &entry : raw()) {
        result.append(entry.first);
    }
    return result;
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::toValueList() const -> List<Value> {
    auto result = List<Value>{};
    result.reserve(count());
    for (const auto &entry : raw()) {
        result.append(entry.second);
    }
    return result;
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::toList() const -> List<Entry> {
    auto result = List<Entry>{};
    result.reserve(count());
    for (const auto &[key, value] : raw()) {
        result.append({key, value});
    }
    return result;
}

}
