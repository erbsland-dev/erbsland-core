// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CoAsyncGeneratorPromise_fwd.hpp"
#include "CoAsyncGeneratorTransferAwaiter.hpp"

#include "../CoAsyncGenerator_fwd.hpp"

#include <concepts>
#include <coroutine>
#include <exception>
#include <mutex>
#include <optional>
#include <utility>

namespace erbsland::util::impl {

/// Coordinate an asynchronous generator producer with one consumer.
template <typename tValue>
class CoAsyncGeneratorPromise {
    friend class CoAsyncGenerator<tValue>;
    friend class CoAsyncGeneratorTransferAwaiter<tValue>;
    template <typename>
    friend class CoAsyncGeneratorNextOperation;

public:
    using Value = tValue;
    using TransferAwaiter = CoAsyncGeneratorTransferAwaiter<tValue>;

    /// Create the public generator connected to this promise.
    [[nodiscard]] auto get_return_object() -> CoAsyncGenerator<tValue>;
    /// Keep the producer lazy until the first `next()` operation.
    [[nodiscard]] auto initial_suspend() noexcept -> std::suspend_always { return {}; }
    /// Transfer execution back to the consumer when generation completes.
    [[nodiscard]] auto final_suspend() noexcept -> TransferAwaiter { return {}; }
    /// Store and yield one value to the consumer.
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

}
