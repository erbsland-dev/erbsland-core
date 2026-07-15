// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <coroutine>
#include <memory>
#include <mutex>
#include <utility>

namespace erbsland::util::impl {

/// Mark a coroutine task complete and resume its registered continuation.
/// @param state The task state to complete.
/// @tested{CoTaskTest}
template <typename tState>
inline void completeCoTask(const std::shared_ptr<tState> &state) noexcept {
    auto continuation = std::coroutine_handle<>{};
    {
        const auto lock = std::scoped_lock{state->mutex};
        state->complete = true;
        continuation = std::exchange(state->continuation, {});
    }
    if (continuation) {
        continuation.resume();
    }
}

}
