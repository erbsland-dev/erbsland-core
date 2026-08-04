// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CoAsyncGenerator_fwd.hpp"

#include "impl/CoAsyncGeneratorNextOperation.hpp"
#include "impl/CoAsyncGeneratorPromise.hpp"

#include <concepts>
#include <coroutine>
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
    using Value = tValue;                                    ///< The yielded value type.
    using promise_type = impl::CoAsyncGeneratorPromise<tValue>;
    using handle_type = std::coroutine_handle<promise_type>; ///< The producer coroutine handle type.
    using NextOperation = impl::CoAsyncGeneratorNextOperation<tValue>;

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

    /// Move ownership from another generator.
    /// @param other The generator whose coroutine is transferred.
    CoAsyncGenerator(CoAsyncGenerator &&other) noexcept : _handle{std::exchange(other._handle, {})} {}
    /// Replace this generator's coroutine with another generator's coroutine.
    auto operator=(CoAsyncGenerator &&other) noexcept -> CoAsyncGenerator & {
        if (this != &other) {
            if (_handle) {
                _handle.destroy();
            }
            _handle = std::exchange(other._handle, {});
        }
        return *this;
    }

    // defaults/deletions
    CoAsyncGenerator(const CoAsyncGenerator &) = delete;
    auto operator=(const CoAsyncGenerator &) -> CoAsyncGenerator & = delete;

public:
    /// Request the next generated value.
    /// @return An awaitable operation that produces the next value, or an empty optional at completion.
    /// Awaiting the returned operation rethrows exceptions that escaped from the producer coroutine.
    [[nodiscard]] auto next() noexcept -> NextOperation { return NextOperation{_handle}; }

private:
    handle_type _handle;
};

/// Create the public generator connected to this promise.
/// @return The lazy generator.
template <typename tValue>
auto impl::CoAsyncGeneratorPromise<tValue>::get_return_object() -> CoAsyncGenerator<tValue> {
    return CoAsyncGenerator<tValue>{std::coroutine_handle<CoAsyncGeneratorPromise<tValue>>::from_promise(*this)};
}

}
