// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CoAsyncGeneratorPromise.hpp"

#include "../../err/LogicError.hpp"

#include <coroutine>
#include <exception>
#include <mutex>
#include <optional>
#include <utility>

namespace erbsland::util::impl {

/// Await the next value from an asynchronous generator.
template <typename tValue>
class CoAsyncGeneratorNextOperation final {
public:
    using Value = tValue;
    using Promise = CoAsyncGeneratorPromise<tValue>;
    using Handle = std::coroutine_handle<Promise>;

    /// Create an operation for a generator coroutine.
    /// @param handle The producer coroutine handle, or an empty handle for an empty generator.
    explicit CoAsyncGeneratorNextOperation(const Handle handle) noexcept : _handle{handle} {}

    /// Move an outstanding operation.
    CoAsyncGeneratorNextOperation(CoAsyncGeneratorNextOperation &&other) noexcept :
        _handle{std::exchange(other._handle, {})} {}
    /// Move an outstanding operation from another instance.
    auto operator=(CoAsyncGeneratorNextOperation &&other) noexcept -> CoAsyncGeneratorNextOperation & {
        _handle = std::exchange(other._handle, {});
        return *this;
    }

    // defaults/deletions
    CoAsyncGeneratorNextOperation(const CoAsyncGeneratorNextOperation &) = delete;
    auto operator=(const CoAsyncGeneratorNextOperation &) -> CoAsyncGeneratorNextOperation & = delete;

public:
    /// Test if no producer resumption is required.
    [[nodiscard]] auto await_ready() const noexcept -> bool { return !_handle || _handle.done(); }
    /// Register the consumer and resume the producer.
    [[nodiscard]] auto await_suspend(const std::coroutine_handle<> continuation) -> Handle {
        const auto lock = std::scoped_lock{_handle.promise()._mutex};
        if (_handle.promise()._nextPending) {
            throw err::LogicError{"A CoAsyncGenerator can only process one next operation at a time."};
        }
        _handle.promise()._nextPending = true;
        _handle.promise()._continuation = continuation;
        return _handle;
    }
    /// Consume the yielded value or observe generator completion.
    [[nodiscard]] auto await_resume() -> std::optional<Value> {
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
    Handle _handle;
};

}
