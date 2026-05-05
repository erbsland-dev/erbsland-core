// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../LoopResult.hpp"
#include "../LoopStatus.hpp"

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

namespace erbsland::util::impl {

/// Convert a callback status to the matching loop result.
[[nodiscard]] constexpr auto loopStatusToResult(const LoopStatus status) noexcept -> LoopResult {
    switch (status) {
    case LoopStatus::Continue:
        return LoopResult::Success;
    case LoopStatus::Stop:
        return LoopResult::Stopped;
    case LoopStatus::Error:
        return LoopResult::Error;
    }
    return LoopResult::Error;
}

/// Invoke a loop callback and normalize its result.
template <typename Function, typename... Arguments>
[[nodiscard]] auto invokeLoopFunction(Function &function, Arguments &&...arguments) -> LoopStatus {
    using Result = std::invoke_result_t<Function &, Arguments...>;
    if constexpr (std::is_void_v<Result>) {
        std::invoke(function, std::forward<Arguments>(arguments)...);
        return LoopStatus::Continue;
    } else {
        static_assert(
            std::same_as<std::remove_cvref_t<Result>, LoopStatus>,
            "Loop callback must return void or erbsland::util::LoopStatus.");
        return std::invoke(function, std::forward<Arguments>(arguments)...);
    }
}

}
