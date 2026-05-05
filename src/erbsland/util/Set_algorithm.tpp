// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/LoopControl.hpp"

#include <functional>

namespace erbsland::util {

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
template <typename Function>
auto Set<tKey, tCompare, tSelf>::forEach(Function function) const -> LoopResult {
    for (const auto &key : raw()) {
        const auto status = impl::invokeLoopFunction(function, key);
        if (status != LoopStatus::Continue) {
            return impl::loopStatusToResult(status);
        }
    }
    return LoopResult::Success;
}

template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
template <typename Function>
auto Set<tKey, tCompare, tSelf>::forEachReverse(Function function) const -> LoopResult {
    const auto &data = raw();
    for (auto iterator = data.rbegin(); iterator != data.rend(); ++iterator) {
        const auto status = impl::invokeLoopFunction(function, *iterator);
        if (status != LoopStatus::Continue) {
            return impl::loopStatusToResult(status);
        }
    }
    return LoopResult::Success;
}

}
