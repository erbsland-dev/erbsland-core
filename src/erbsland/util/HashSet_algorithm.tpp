// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/LoopControl.hpp"

#include <functional>

namespace erbsland::util {

template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
template <typename Function>
auto HashSet<tKey, tHash, tEqual, tSelf>::forEach(Function function) const -> LoopResult {
    for (const auto &key : raw()) {
        const auto status = impl::invokeLoopFunction(function, key);
        if (status != LoopStatus::Continue) {
            return impl::loopStatusToResult(status);
        }
    }
    return LoopResult::Success;
}

}
