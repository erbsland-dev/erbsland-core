// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../err/LogicError.hpp"

#include <concepts>
#include <coroutine>
#include <exception>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>

namespace erbsland::util {

/// An asynchronous, single-pass coroutine generator.
///
/// Unlike `CoGenerator`, advancing this generator is itself asynchronous. The producer may use both `co_yield` and
/// `co_await`; consumers request values with `co_await generator.next()`. The generator is move-only and permits one
/// outstanding `next()` operation.
/// @tparam tValue The yielded value type.
/// @tested{CoAsyncGeneratorTest}
template <typename tValue>
class CoAsyncGenerator {
    static_assert(std::is_object_v<tValue>, "CoAsyncGenerator requires an object value type.");
    static_assert(!std::is_const_v<tValue>, "CoAsyncGenerator requires a non-const value type.");
    static_assert(!std::is_volatile_v<tValue>, "CoAsyncGenerator requires a non-volatile value type.");
    static_assert(std::move_constructible<tValue>, "CoAsyncGenerator requires a move constructible value type.");

public:
    struct promise_type;
    class NextOperation;

    using Value = tValue;                                    ///< The yielded value type.
    using handle_type = std::coroutine_handle<promise_type>; ///< The producer coroutine handle type.

    /// Coroutine promise that coordinates the producer with one consumer.
    /// @tested{CoAsyncGeneratorTest}
    struct promise_type {
        friend class CoAsyncGenerator;
        friend class NextOperation;

    private:
        struct TransferAwaiter final {
            [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }
            [[nodiscard]] auto await_suspend(const handle_type handle) const noexcept -> std::coroutine_handle<> {
                auto continuation = std::coroutine_handle<>{};
                {
                    const auto lock = std::scoped_lock{handle.promise()._mutex};
                    handle.promise()._nextPending = false;
                    continuation = std::exchange(handle.promise()._continuation, {});
                }
                return continuation ? continuation : std::noop_coroutine();
            }
            void await_resume() const noexcept {}
        };

    public:
        /// Create the public generator connected to this promise.
        /// @return The lazy generator.
        auto get_return_object() -> CoAsyncGenerator { return CoAsyncGenerator{handle_type::from_promise(*this)}; }
        /// Keep the producer lazy until the first `next()` operation.
        /// @return An awaiter that suspends the producer.
        auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        /// Transfer execution back to the consumer when generation completes.
        /// @return The transfer awaiter.
        auto final_suspend() noexcept -> TransferAwaiter { return {}; }
        /// Store and yield one value to the consumer.
        /// @tparam tYielded The yielded expression type.
        /// @param value The value used to construct the next generator value.
        /// @return An awaiter that transfers execution back to the consumer.
        template <typename tYielded>
            requires std::constructible_from<Value, tYielded>
        auto yield_value(tYielded &&value) -> TransferAwaiter {
            _currentValue.emplace(std::forward<tYielded>(value));
            return {};
        }
        /// Complete the producer without another value.
        void return_void() noexcept {}
        /// Store an exception that escaped the producer coroutine.
        void unhandled_exception() noexcept { _exception = std::current_exception(); }

    private:
        mutable std::mutex _mutex;
        std::optional<Value> _currentValue;
        std::exception_ptr _exception;
        std::coroutine_handle<> _continuation;
        bool _nextPending{false};
    };

    /// One asynchronous request for the next generated value.
    /// @tested{CoAsyncGeneratorTest}
    class NextOperation final {
    public:
        /// Create an operation for a generator coroutine.
        /// @param handle The producer coroutine handle, or an empty handle for an empty generator.
        explicit NextOperation(handle_type handle) noexcept : _handle{handle} {}

        NextOperation(const NextOperation &) = delete;
        auto operator=(const NextOperation &) -> NextOperation & = delete;
        /// Move an outstanding operation.
        /// @param other The operation whose handle is transferred.
        NextOperation(NextOperation &&other) noexcept : _handle{std::exchange(other._handle, {})} {}
        auto operator=(NextOperation &&other) noexcept -> NextOperation & {
            _handle = std::exchange(other._handle, {});
            return *this;
        }

    public:
        /// Test if no producer resumption is required.
        /// @return `true` for an empty or completed generator.
        [[nodiscard]] auto await_ready() const noexcept -> bool { return !_handle || _handle.done(); }
        /// Register the consumer and resume the producer.
        /// @param continuation The consumer coroutine awaiting this operation.
        /// @return The producer coroutine handle to resume.
        /// @throws err::LogicError If another `next()` operation is already outstanding.
        [[nodiscard]] auto await_suspend(const std::coroutine_handle<> continuation) -> handle_type {
            const auto lock = std::scoped_lock{_handle.promise()._mutex};
            if (_handle.promise()._nextPending) {
                throw err::LogicError{"A CoAsyncGenerator can only process one next operation at a time."};
            }
            _handle.promise()._nextPending = true;
            _handle.promise()._continuation = continuation;
            return _handle;
        }
        /// Consume the yielded value or observe generator completion.
        /// @return The yielded value, or an empty optional when generation completed.
        /// @throws Any exception produced by the generator coroutine.
        auto await_resume() -> std::optional<Value> {
            if (!_handle) {
                return {};
            }
            auto &promise = _handle.promise();
            if (promise._exception) {
                std::rethrow_exception(promise._exception);
            }
            if (_handle.done()) {
                return {};
            }
            auto result = std::optional<Value>{std::move(*promise._currentValue)};
            promise._currentValue.reset();
            return result;
        }

    private:
        handle_type _handle;
    };

public:
    /// Create an empty generator.
    CoAsyncGenerator() = default;
    /// Create a generator from its coroutine handle.
    /// @param handle The producer coroutine handle.
    explicit CoAsyncGenerator(handle_type handle) noexcept : _handle{handle} {}
    /// Destroy the producer coroutine.
    ~CoAsyncGenerator() {
        if (_handle) {
            _handle.destroy();
        }
    }

    CoAsyncGenerator(const CoAsyncGenerator &) = delete;
    auto operator=(const CoAsyncGenerator &) -> CoAsyncGenerator & = delete;
    /// Move ownership from another generator.
    /// @param other The generator whose coroutine is transferred.
    CoAsyncGenerator(CoAsyncGenerator &&other) noexcept : _handle{std::exchange(other._handle, {})} {}
    auto operator=(CoAsyncGenerator &&other) noexcept -> CoAsyncGenerator & {
        if (this != &other) {
            if (_handle) {
                _handle.destroy();
            }
            _handle = std::exchange(other._handle, {});
        }
        return *this;
    }

public:
    /// Request the next generated value.
    /// @return An awaitable operation that produces the next value, or an empty optional at completion.
    /// Awaiting the returned operation rethrows exceptions that escaped from the producer coroutine.
    [[nodiscard]] auto next() noexcept -> NextOperation { return NextOperation{_handle}; }

private:
    handle_type _handle;
};

}
