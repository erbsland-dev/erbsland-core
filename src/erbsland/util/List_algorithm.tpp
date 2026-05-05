// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/LoopControl.hpp"

#include <algorithm>
#include <functional>

namespace erbsland::util {

template <typename tElement, typename tSelf>
template <typename Function>
auto List<tElement, tSelf>::forEach(Function function) const -> LoopResult {
    for (const auto &value : raw()) {
        const auto status = impl::invokeLoopFunction(function, value);
        if (status != LoopStatus::Continue) {
            return impl::loopStatusToResult(status);
        }
    }
    return LoopResult::Success;
}

template <typename tElement, typename tSelf>
template <typename Function>
auto List<tElement, tSelf>::forEachReverse(Function function) const -> LoopResult {
    const auto &data = raw();
    for (auto iterator = data.rbegin(); iterator != data.rend(); ++iterator) {
        const auto status = impl::invokeLoopFunction(function, *iterator);
        if (status != LoopStatus::Continue) {
            return impl::loopStatusToResult(status);
        }
    }
    return LoopResult::Success;
}

template <typename tElement, typename tSelf>
template <typename Function>
auto List<tElement, tSelf>::map(Function function) -> Self & {
    auto &data = mutableRaw();
    for (auto &value : data) {
        static_assert(std::same_as<std::remove_cvref_t<std::invoke_result_t<Function, const Element &>>, Element>);
        value = std::invoke(function, value);
    }
    return self();
}

template <typename tElement, typename tSelf>
template <typename Function>
auto List<tElement, tSelf>::mapped(Function function) const -> Self {
    auto result = self();
    result.map(function);
    return result;
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::reverse() -> Self & {
    auto &data = mutableRaw();
    std::reverse(data.begin(), data.end());
    return self();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::reversed() const -> Self {
    auto result = self();
    result.reverse();
    return result;
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::collapse() -> Self & {
    auto &data = mutableRaw();
    const auto newEnd = std::unique(data.begin(), data.end());
    data.erase(newEnd, data.end());
    return self();
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::collapsed() const -> Self {
    auto result = self();
    result.collapse();
    return result;
}

}
