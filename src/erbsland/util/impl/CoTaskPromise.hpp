// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CoTaskCompletion.hpp"
#include "CoTaskFinalAwaiter.hpp"
#include "CoTaskPromise_fwd.hpp"
#include "CoTaskState.hpp"

#include <concepts>
#include <coroutine>
#include <exception>
#include <memory>
#include <utility>

namespace erbsland::util::impl {

/// Provides the coroutine protocol and shared state for a CoTask with a result.
/// @tparam tValue The task result type.
/// @tested{CoTaskTest}
template <typename tValue>
class CoTaskPromise final {
public:
    /// Create promise state for a new task.
    CoTaskPromise() : _state{std::make_shared<CoTaskState<tValue>>()} {}
    /// Create the public task connected to this promise.
    /// @return The eagerly started task.
    [[nodiscard]] auto get_return_object() -> CoTask<tValue>;
    /// Start executing the coroutine immediately.
    /// @return An awaiter that does not suspend.
    [[nodiscard]] auto initial_suspend() noexcept -> std::suspend_never { return {}; }
    /// Complete shared task state at the final suspension point.
    /// @return The final completion awaiter.
    [[nodiscard]] auto final_suspend() noexcept -> CoTaskFinalAwaiter<tValue> { return {}; }
    /// Store the value returned by the coroutine.
    /// @tparam tResult The returned expression type.
    /// @param value The value used to construct the task result.
    template <typename tResult>
        requires std::constructible_from<tValue, tResult>
    void return_value(tResult &&value) {
        _state->value.emplace(std::forward<tResult>(value));
    }
    /// Store an exception that escaped the coroutine body.
    void unhandled_exception() noexcept { _state->exception = std::current_exception(); }
    /// Access shared task state for final coroutine completion.
    /// @return The task state owned by this promise.
    [[nodiscard]] auto state() const noexcept -> std::shared_ptr<CoTaskState<tValue>> { return _state; }
    /// Access task cancellation state for Erbsland coroutine awaiters.
    /// @return Shared cancellation state for this task.
    [[nodiscard]] auto cancellationState() const noexcept -> std::shared_ptr<CoTaskStateBase> { return _state; }

private:
    std::shared_ptr<CoTaskState<tValue>> _state;
};

/// Provides the coroutine protocol and shared state for a void CoTask.
/// @tested{CoTaskTest}
template <>
class CoTaskPromise<void> final {
public:
    /// Create promise state for a new task.
    CoTaskPromise() : _state{std::make_shared<CoTaskState<void>>()} {}
    /// Create the public task connected to this promise.
    /// @return The eagerly started task.
    [[nodiscard]] auto get_return_object() -> CoTask<void>;
    /// Start executing the coroutine immediately.
    /// @return An awaiter that does not suspend.
    [[nodiscard]] auto initial_suspend() noexcept -> std::suspend_never { return {}; }
    /// Complete shared task state at the final suspension point.
    /// @return The final completion awaiter.
    [[nodiscard]] auto final_suspend() noexcept -> CoTaskFinalAwaiter<void> { return {}; }
    /// Complete the coroutine without storing a value.
    void return_void() noexcept {}
    /// Store an exception that escaped the coroutine body.
    void unhandled_exception() noexcept { _state->exception = std::current_exception(); }
    /// Access shared task state for final coroutine completion.
    /// @return The task state owned by this promise.
    [[nodiscard]] auto state() const noexcept -> std::shared_ptr<CoTaskState<void>> { return _state; }
    /// Access task cancellation state for Erbsland coroutine awaiters.
    /// @return Shared cancellation state for this task.
    [[nodiscard]] auto cancellationState() const noexcept -> std::shared_ptr<CoTaskStateBase> { return _state; }

private:
    std::shared_ptr<CoTaskState<void>> _state;
};

template <typename tValue>
/// Complete this task and destroy its coroutine frame.
/// @param handle The completed coroutine handle.
void CoTaskFinalAwaiter<tValue>::await_suspend(
    const std::coroutine_handle<CoTaskPromise<tValue>> handle) const noexcept {
    auto state = handle.promise().state();
    handle.destroy();
    completeCoTask(state);
}

}
