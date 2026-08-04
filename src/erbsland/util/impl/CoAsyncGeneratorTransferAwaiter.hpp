// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CoAsyncGeneratorPromise_fwd.hpp"

#include <coroutine>
#include <mutex>
#include <utility>

namespace erbsland::util::impl {

/// Transfer execution from an asynchronous generator producer to its consumer.
template <typename tValue>
class CoAsyncGeneratorTransferAwaiter final {
public:
    /// Always suspend the producer before transferring control.
    [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }
    /// Resume the registered consumer coroutine.
    [[nodiscard]] auto await_suspend(const std::coroutine_handle<CoAsyncGeneratorPromise<tValue>> handle) const noexcept
        -> std::coroutine_handle<> {
        auto continuation = std::coroutine_handle<>{};
        {
            const auto lock = std::scoped_lock{handle.promise()._mutex};
            handle.promise()._nextPending = false;
            continuation = std::exchange(handle.promise()._continuation, {});
        }
        return continuation ? continuation : std::noop_coroutine();
    }
    /// Complete the producer's await operation.
    void await_resume() const noexcept {}
};

}
