// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/LoopControl.hpp"

#include <functional>

namespace erbsland::util {

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::forEach(Function function) const -> LoopResult {
    for (const auto &[key, value] : raw()) {
        const auto status = impl::invokeLoopFunction(function, key, value);
        if (status != LoopStatus::Continue) {
            return impl::loopStatusToResult(status);
        }
    }
    return LoopResult::Success;
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::forEachKey(Function function) const -> LoopResult {
    return forEach(
        [&](const Key &key, const Value &) -> LoopStatus { return impl::invokeLoopFunction(function, key); });
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::forEachValue(Function function) const -> LoopResult {
    return forEach(
        [&](const Key &, const Value &value) -> LoopStatus { return impl::invokeLoopFunction(function, value); });
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::mapValue(Function function) -> Self & {
    auto &data = mutableRaw();
    for (auto &[key, value] : data) {
        static_assert(
            std::same_as<std::remove_cvref_t<std::invoke_result_t<Function, const Key &, const Value &>>, Value>);
        value = std::invoke(function, key, value);
    }
    return self();
}

template <typename tKey, typename tValue, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
template <typename Function>
auto HashMap<tKey, tValue, tHash, tEqual, tSelf>::mappedValues(Function function) const -> Self {
    auto result = self();
    result.mapValue(function);
    return result;
}

}
