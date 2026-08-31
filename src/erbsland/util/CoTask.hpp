// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/CoTaskAwaiter.hpp"
#include "impl/CoTaskCompletion.hpp"
#include "impl/CoTaskPromise.hpp"
#include "impl/CoTaskPromise_fwd.hpp"
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
/// cancellation; Erbsland Core coroutine awaiters observe that request at their next completion and unwind the
/// coroutine. Continuations run on the thread that completes the awaited operation and have no caller-thread affinity.
/// @tparam tValue The task result type.
/// @tested{CoTaskTest}
template <typename tValue>
class CoTask {
    static_assert(!std::is_void_v<tValue>, "Use the CoTask<void> specialization for void tasks.");
    static_assert(std::move_constructible<tValue>, "CoTask requires a move constructible result type.");

private:
    using State = impl::CoTaskState<tValue>;

    friend class impl::CoTaskPromise<tValue>;

public:
    /// Coroutine promise that owns the shared state of a task with a result.
    /// @tested{CoTaskTest}
    using promise_type = impl::CoTaskPromise<tValue>;

public:
    /// Create an empty task.
    CoTask() = default;
    /// Move ownership from another task.
    /// @param other The task whose state is transferred.
    CoTask(CoTask &&other) noexcept : _state{std::move(other._state)} {}
    /// Request cancellation when this task is still incomplete.
    ~CoTask() { cancel(); }
    /// Move task ownership from another task.
    auto operator=(CoTask &&other) noexcept -> CoTask & {
        if (this != &other) {
            cancel();
            _state = std::move(other._state);
        }
        return *this;
    }

    // defaults/deletions
    CoTask(const CoTask &) = delete;
    auto operator=(const CoTask &) -> CoTask & = delete;

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
    /// Create a task retaining shared coroutine state.
    /// @param state The shared coroutine state.
    explicit CoTask(std::shared_ptr<State> state) : _state{std::move(state)} {}

private:
    std::shared_ptr<State> _state;
};

/// An eagerly started, single-consumer coroutine task without a result value.
/// @tested{CoTaskTest}
template <>
class CoTask<void> {
private:
    using State = impl::CoTaskState<void>;

    friend class impl::CoTaskPromise<void>;

public:
    /// Coroutine promise that owns the shared state of a task without a result.
    /// @tested{CoTaskTest}
    using promise_type = impl::CoTaskPromise<void>;

public:
    /// Create an empty task.
    CoTask() = default;
    /// Request cancellation when this task is still incomplete.
    ~CoTask() { cancel(); }

    // defaults/deletions
    CoTask(const CoTask &) = delete;
    auto operator=(const CoTask &) -> CoTask & = delete;
    /// Move ownership from another task.
    /// @param other The task whose state is transferred.
    CoTask(CoTask &&other) noexcept : _state{std::move(other._state)} {}
    /// Move task ownership from another task.
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
    /// Prevent consuming a result from a void task.
    void takeResult() = delete;

private:
    /// Create a task from its shared coroutine state.
    explicit CoTask(std::shared_ptr<State> state) : _state{std::move(state)} {}

private:
    std::shared_ptr<State> _state;
};

template <typename tValue>
/// Create the public task connected to this promise.
/// @return The eagerly started task.
auto impl::CoTaskPromise<tValue>::get_return_object() -> CoTask<tValue> {
    return CoTask<tValue>{_state};
}

/// Create the public task connected to this void promise.
/// @return The eagerly started task.
inline auto impl::CoTaskPromise<void>::get_return_object() -> CoTask<void> {
    return CoTask<void>{_state};
}

}
