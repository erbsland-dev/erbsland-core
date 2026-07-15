// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CoTask_fwd.hpp"

#include "impl/CoTaskAwaiter.hpp"
#include "impl/CoTaskCompletion.hpp"
#include "impl/CoWorkAwaiter.hpp"

#include "../err/LogicError.hpp"

#include <concepts>
#include <coroutine>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

namespace erbsland::util {

/// An eagerly started, single-consumer coroutine task.
///
/// `CoTask` stores its result independently from the coroutine frame. Destroying an incomplete task requests
/// cancellation; Erbsland coroutine awaiters observe that request at their next completion and unwind the coroutine.
/// Continuations run on the thread that completes the awaited operation and have no caller-thread affinity.
/// @tparam tValue The task result type.
/// @tested{CoTaskTest}
template <typename tValue>
class CoTask {
    static_assert(!std::is_void_v<tValue>, "Use the CoTask<void> specialization for void tasks.");
    static_assert(std::move_constructible<tValue>, "CoTask requires a move constructible result type.");

private:
    using State = impl::CoTaskState<tValue>;

public:
    /// Coroutine promise that owns the shared state of a task with a result.
    /// @tested{CoTaskTest}
    struct promise_type {
    private:
        struct FinalAwaiter final {
            [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }
            void await_suspend(const std::coroutine_handle<promise_type> handle) const noexcept {
                auto state = handle.promise()._state;
                handle.destroy();
                impl::completeCoTask(state);
            }
            void await_resume() const noexcept {}
        };

    public:
        /// Create promise state for a new task.
        promise_type() : _state{std::make_shared<State>()} {}
        /// Create the public task connected to this promise.
        /// @return The eagerly started task.
        auto get_return_object() -> CoTask { return CoTask{_state}; }
        /// Start executing the coroutine immediately.
        /// @return An awaiter that does not suspend.
        auto initial_suspend() noexcept -> std::suspend_never { return {}; }
        /// Complete shared task state at the final suspension point.
        /// @return The final completion awaiter.
        auto final_suspend() noexcept -> FinalAwaiter { return {}; }
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
        /// Access task cancellation state for Erbsland coroutine awaiters.
        /// @return Shared cancellation state for this task.
        [[nodiscard]] auto cancellationState() const noexcept -> std::shared_ptr<impl::CoTaskStateBase> {
            return _state;
        }

    private:
        std::shared_ptr<State> _state;
    };

public:
    /// Create an empty task.
    CoTask() = default;
    /// Request cancellation when this task is still incomplete.
    ~CoTask() { cancel(); }

    CoTask(const CoTask &) = delete;
    auto operator=(const CoTask &) -> CoTask & = delete;
    /// Move ownership from another task.
    /// @param other The task whose state is transferred.
    CoTask(CoTask &&other) noexcept : _state{std::move(other._state)} {}
    auto operator=(CoTask &&other) noexcept -> CoTask & {
        if (this != &other) {
            cancel();
            _state = std::move(other._state);
        }
        return *this;
    }

public:
    /// Run a callable on the coroutine worker service.
    /// @tparam tFunction The callable type.
    /// @param function The callable to execute.
    /// @return An eagerly started task for the callable result.
    /// Exceptions from the callable are stored in the task and rethrown when its result is observed.
    template <typename tFunction>
        requires std::same_as<std::invoke_result_t<tFunction>, tValue>
    [[nodiscard]] static auto run(tFunction function) -> CoTask {
        co_return co_await impl::CoWorkAwaiter<tValue, tFunction>{std::move(function)};
    }
    /// Test if this task completed.
    /// @return `true` if the task is empty or its coroutine completed.
    [[nodiscard]] auto isComplete() const noexcept -> bool {
        if (!_state) {
            return true;
        }
        const auto lock = std::scoped_lock{_state->mutex};
        return _state->complete;
    }
    /// Access the completed result.
    /// @return A reference to the stored result without consuming it.
    /// @throws err::LogicError If the task is empty, incomplete, or its result was consumed.
    /// @throws Any exception produced by the coroutine.
    [[nodiscard]] auto result() const -> const tValue & {
        if (!_state) {
            throw err::LogicError{"An empty CoTask has no result."};
        }
        const auto lock = std::scoped_lock{_state->mutex};
        if (!_state->complete) {
            throw err::LogicError{"The CoTask result is not ready."};
        }
        if (_state->exception) {
            std::rethrow_exception(_state->exception);
        }
        if (_state->consumed) {
            throw err::LogicError{"The CoTask result was already consumed."};
        }
        return *_state->value;
    }
    /// Take the completed result.
    /// @return The moved task result.
    /// @throws err::LogicError If the task is empty, incomplete, or its result was consumed.
    /// @throws Any exception produced by the coroutine.
    [[nodiscard]] auto takeResult() -> tValue {
        if (!_state) {
            throw err::LogicError{"An empty CoTask has no result."};
        }
        const auto lock = std::scoped_lock{_state->mutex};
        if (!_state->complete) {
            throw err::LogicError{"The CoTask result is not ready."};
        }
        if (_state->exception) {
            std::rethrow_exception(_state->exception);
        }
        if (_state->consumed) {
            throw err::LogicError{"The CoTask result was already consumed."};
        }
        _state->consumed = true;
        return std::move(*_state->value);
    }
    /// Request cancellation of this task.
    /// Running bounded work is allowed to finish; the coroutine unwinds instead of resuming user code afterward.
    void cancel() noexcept {
        if (_state) {
            _state->cancelled.store(true);
            _state.reset();
        }
    }
    /// Await and consume this task result.
    /// @return An awaiter for the result.
    /// @throws err::LogicError If this task is empty.
    [[nodiscard]] auto operator co_await() && -> impl::CoTaskAwaiter<tValue> {
        if (!_state) {
            throw err::LogicError{"An empty CoTask cannot be awaited."};
        }
        return impl::CoTaskAwaiter<tValue>{std::exchange(_state, {})};
    }

private:
    explicit CoTask(std::shared_ptr<State> state) : _state{std::move(state)} {}

private:
    std::shared_ptr<State> _state;
};

/**
 * @brief An eagerly started, single-consumer coroutine task without a result value.
 * @tested{CoTaskTest}
 */
template <>
class CoTask<void> {
private:
    using State = impl::CoTaskState<void>;

public:
    /// Coroutine promise that owns the shared state of a task without a result.
    /// @tested{CoTaskTest}
    struct promise_type {
    private:
        struct FinalAwaiter final {
            [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }
            void await_suspend(const std::coroutine_handle<promise_type> handle) const noexcept {
                auto state = handle.promise()._state;
                handle.destroy();
                impl::completeCoTask(state);
            }
            void await_resume() const noexcept {}
        };

    public:
        /// Create promise state for a new task.
        promise_type() : _state{std::make_shared<State>()} {}
        /// Create the public task connected to this promise.
        /// @return The eagerly started task.
        auto get_return_object() -> CoTask { return CoTask{_state}; }
        /// Start executing the coroutine immediately.
        /// @return An awaiter that does not suspend.
        auto initial_suspend() noexcept -> std::suspend_never { return {}; }
        /// Complete shared task state at the final suspension point.
        /// @return The final completion awaiter.
        auto final_suspend() noexcept -> FinalAwaiter { return {}; }
        /// Complete the coroutine without storing a value.
        void return_void() noexcept {}
        /// Store an exception that escaped the coroutine body.
        void unhandled_exception() noexcept { _state->exception = std::current_exception(); }
        /// Access task cancellation state for Erbsland coroutine awaiters.
        /// @return Shared cancellation state for this task.
        [[nodiscard]] auto cancellationState() const noexcept -> std::shared_ptr<impl::CoTaskStateBase> {
            return _state;
        }

    private:
        std::shared_ptr<State> _state;
    };

public:
    /// Create an empty task.
    CoTask() = default;
    /// Request cancellation when this task is still incomplete.
    ~CoTask() { cancel(); }

    CoTask(const CoTask &) = delete;
    auto operator=(const CoTask &) -> CoTask & = delete;
    /// Move ownership from another task.
    /// @param other The task whose state is transferred.
    CoTask(CoTask &&other) noexcept : _state{std::move(other._state)} {}
    auto operator=(CoTask &&other) noexcept -> CoTask & {
        if (this != &other) {
            cancel();
            _state = std::move(other._state);
        }
        return *this;
    }

public:
    /// Run a void callable on the coroutine worker service.
    /// @tparam tFunction The callable type.
    /// @param function The callable to execute.
    /// @return An eagerly started task for its completion.
    /// Exceptions from the callable are stored in the task and rethrown when completion is observed.
    template <typename tFunction>
        requires std::same_as<std::invoke_result_t<tFunction>, void>
    [[nodiscard]] static auto run(tFunction function) -> CoTask {
        co_await impl::CoWorkAwaiter<void, tFunction>{std::move(function)};
    }
    /// Test if this task completed.
    /// @return `true` if the task is empty or its coroutine completed.
    [[nodiscard]] auto isComplete() const noexcept -> bool {
        if (!_state) {
            return true;
        }
        const auto lock = std::scoped_lock{_state->mutex};
        return _state->complete;
    }
    /// Observe completion and rethrow a stored exception.
    /// @throws err::LogicError If the task is empty or incomplete.
    /// @throws Any exception produced by the coroutine.
    void result() const {
        if (!_state) {
            throw err::LogicError{"An empty CoTask has no result."};
        }
        const auto lock = std::scoped_lock{_state->mutex};
        if (!_state->complete) {
            throw err::LogicError{"The CoTask result is not ready."};
        }
        if (_state->exception) {
            std::rethrow_exception(_state->exception);
        }
    }
    /// Request cancellation of this task.
    /// Running bounded work is allowed to finish; the coroutine unwinds instead of resuming user code afterward.
    void cancel() noexcept {
        if (_state) {
            _state->cancelled.store(true);
            _state.reset();
        }
    }
    /// Await this task's completion.
    /// @return An awaiter for completion.
    /// @throws err::LogicError If this task is empty.
    [[nodiscard]] auto operator co_await() && -> impl::CoTaskAwaiter<void> {
        if (!_state) {
            throw err::LogicError{"An empty CoTask cannot be awaited."};
        }
        return impl::CoTaskAwaiter<void>{std::exchange(_state, {})};
    }

private:
    void takeResult() = delete;

private:
    explicit CoTask(std::shared_ptr<State> state) : _state{std::move(state)} {}

private:
    std::shared_ptr<State> _state;
};

}
