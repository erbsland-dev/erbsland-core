// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../StopToken.hpp"

#include <functional>
#include <type_traits>
#include <utility>

namespace erbsland::util::impl {

/// Invoke work, preferring the cooperative token overload when it exists.
/// @tparam tFunction The callable type.
/// @tparam tArgs The item argument types.
/// @param function The work callable.
/// @param token The pool stop token.
/// @param args The optional item argument.
/// @return The callable result.
/// @tested{WorkerPoolTest WorkerPoolBulkTest}
template <typename tFunction, typename... tArgs>
decltype(auto) invokeWorker(tFunction &&function, StopToken token, tArgs &&...args) {
    if constexpr (std::is_invocable_v<tFunction, tArgs..., StopToken>) {
        return std::invoke(std::forward<tFunction>(function), std::forward<tArgs>(args)..., std::move(token));
    } else {
        return std::invoke(std::forward<tFunction>(function), std::forward<tArgs>(args)...);
    }
}

}
