// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CoTaskPromise_fwd.hpp"

#include <coroutine>

namespace erbsland::util::impl {

/// Completes state after a CoTask coroutine reaches its final suspension point.
/// @tparam tValue The task result type.
/// @tested{CoTaskTest}
template <typename tValue>
class CoTaskFinalAwaiter final {
public:
    /// Test whether final suspension can proceed without suspension.
    [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }
    /// Destroy the coroutine frame and complete its shared task state.
    void await_suspend(const std::coroutine_handle<CoTaskPromise<tValue>> handle) const noexcept;
    /// Resume after final coroutine completion.
    void await_resume() const noexcept {}
};

}
