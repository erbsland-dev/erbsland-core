// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CoTaskCancelled.hpp"
#include "CoTaskStateBase.hpp"

#include "../../err/LogicError.hpp"

#include <coroutine>
#include <memory>
#include <mutex>
#include <utility>

namespace erbsland::util::impl {

/// Common single-consumer suspension logic for coroutine task awaiters.
/// @tparam tState The concrete shared task state.
/// @tested{CoTaskTest}
template <typename tState>
class CoTaskAwaiterBase {
public:
    /// Create an awaiter for shared task state.
    /// @param state The state retained while the caller is suspended.
    explicit CoTaskAwaiterBase(std::shared_ptr<tState> state) : _state{std::move(state)} {}
    /// Cancel an incomplete child task if its awaiting coroutine abandons the awaiter.
    ~CoTaskAwaiterBase() {
        if (!_state || _resumed) {
            return;
        }
        const auto lock = std::scoped_lock{_state->mutex};
        if (!_state->complete && _state->continuation == _continuation) {
            _state->cancelled.store(true);
            _state->continuation = {};
        }
    }

    // defaults/deletions
    CoTaskAwaiterBase(const CoTaskAwaiterBase &) = delete;
    CoTaskAwaiterBase(CoTaskAwaiterBase &&) = delete;
    auto operator=(const CoTaskAwaiterBase &) -> CoTaskAwaiterBase & = delete;
    auto operator=(CoTaskAwaiterBase &&) -> CoTaskAwaiterBase & = delete;

public:
    /// Test if the task is already complete.
    /// @return `true` if awaiting does not need to suspend the caller.
    [[nodiscard]] auto await_ready() const noexcept -> bool {
        const auto lock = std::scoped_lock{_state->mutex};
        return _state->complete;
    }
    /// Register a single continuation for this task.
    /// @tparam tPromise The awaiting coroutine promise type.
    /// @param continuation The coroutine awaiting the task.
    /// @return `true` if the caller must suspend, or `false` if the task completed meanwhile.
    /// @throws err::LogicError If another coroutine is already awaiting this task.
    template <typename tPromise>
    auto await_suspend(const std::coroutine_handle<tPromise> continuation) -> bool {
        _continuation = continuation;
        if constexpr (requires(tPromise &promise) { promise.cancellationState(); }) {
            _parentCancellation = continuation.promise().cancellationState();
        }
        const auto lock = std::scoped_lock{_state->mutex};
        if (_state->complete) {
            return false;
        }
        if (_state->continuation) {
            throw err::LogicError{"A CoTask can only be awaited once."};
        }
        _state->continuation = continuation;
        return true;
    }

protected:
    /// Mark the awaiter resumed and propagate cancellation from its parent task.
    /// @throws CoTaskCancelled If the parent task was cancelled.
    void beforeResume() {
        _resumed = true;
        const auto parentCancellation = _parentCancellation.lock();
        if (parentCancellation && parentCancellation->isCancelled()) {
            throw CoTaskCancelled{};
        }
    }

protected:
    std::shared_ptr<tState> _state; ///< Shared state retained while the caller is suspended.

private:
    bool _resumed{false};
    std::coroutine_handle<> _continuation;
    std::weak_ptr<CoTaskStateBase> _parentCancellation;
};

}
