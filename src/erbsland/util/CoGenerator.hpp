// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CoGenerator_fwd.hpp"

#include "impl/CoGeneratorIterator.hpp"
#include "impl/CoGeneratorPromise.hpp"

#include <concepts>
#include <coroutine>
#include <optional>
#include <type_traits>
#include <utility>

namespace erbsland::util {

/// A C++20 coroutine generator for yielding a single-pass sequence of values.
///
/// `CoGenerator` owns the coroutine state and destroys it when the generator object is destroyed. Values can be
/// consumed either with `next()` or through range iteration. Do not mix both consumption styles for the same generator.
/// The generator is move-only, and iterators remain valid only while the owning generator object is alive.
/// @tparam tValue The value type yielded by the coroutine.
/// @tested{CoGeneratorTest}
template <typename tValue>
class CoGenerator {
    static_assert(std::is_object_v<tValue>, "CoGenerator requires an object value type.");
    static_assert(!std::is_const_v<tValue>, "CoGenerator requires a non-const value type.");
    static_assert(!std::is_volatile_v<tValue>, "CoGenerator requires a non-volatile value type.");
    static_assert(std::move_constructible<tValue>, "CoGenerator requires a move constructible value type.");

public:
    using Value = tValue;                                    ///< The value type yielded by this generator.
    using promise_type = impl::CoGeneratorPromise<tValue>;
    using handle_type = std::coroutine_handle<promise_type>; ///< The standard coroutine handle type.
    using Iterator = impl::CoGeneratorIterator<tValue>;
    using iterator = Iterator;                               ///< The standard iterator type.

public:
    /// Create an empty generator.
    CoGenerator() = default;
    /// Create a generator from a coroutine handle.
    /// @param handle The coroutine handle this generator takes ownership of.
    explicit CoGenerator(handle_type handle) noexcept : _handle{handle} {}
    /// Destroy the owned coroutine state, if present.
    ~CoGenerator() {
        if (_handle) {
            _handle.destroy();
        }
    }

    // defaults/deletions
    CoGenerator(const CoGenerator &) = delete;
    auto operator=(const CoGenerator &) -> CoGenerator & = delete;
    /// Move a generator, transferring ownership of the coroutine state.
    /// @param other The generator to move from.
    CoGenerator(CoGenerator &&other) noexcept : _handle{std::exchange(other._handle, {})} {}
    /// Move-assign a generator, replacing any owned coroutine state.
    /// @param other The generator to move from.
    /// @return A reference to this generator.
    auto operator=(CoGenerator &&other) noexcept -> CoGenerator & {
        if (this != &other) {
            if (_handle) {
                _handle.destroy();
            }
            _handle = std::exchange(other._handle, {});
        }
        return *this;
    }

public:
    /// Consume the next yielded value.
    /// @return The next value, or an empty optional if the coroutine has finished.
    /// @throws Any exception that escaped from the coroutine body.
    [[nodiscard]] auto next() -> std::optional<Value> {
        if (!resume()) {
            return {};
        }
        auto &promise = _handle.promise();
        auto result = std::optional<Value>{std::move(*promise._currentValue)};
        promise._currentValue.reset();
        return result;
    }
    /// Create an iterator at the first yielded value.
    /// @return An iterator for the first value, or `end()` if the coroutine has finished.
    /// @throws Any exception that escaped from the coroutine body.
    [[nodiscard]] auto begin() -> iterator {
        if (!resume()) {
            return end();
        }
        return iterator{_handle};
    }
    /// Create the end iterator.
    /// @return The end iterator.
    [[nodiscard]] auto end() noexcept -> iterator { return {}; }

private:
    /// Resume the coroutine and prepare the next value.
    /// @return `true` if a value is ready, `false` if the coroutine has finished or no coroutine is present.
    /// @throws Any exception that escaped from the coroutine body.
    auto resume() -> bool {
        if (!_handle) {
            return false;
        }
        auto &promise = _handle.promise();
        if (promise._exception) {
            std::rethrow_exception(promise._exception);
        }
        if (_handle.done()) {
            promise._currentValue.reset();
            return false;
        }
        _handle.resume();
        if (promise._exception) {
            promise._currentValue.reset();
            std::rethrow_exception(promise._exception);
        }
        if (_handle.done()) {
            promise._currentValue.reset();
            return false;
        }
        return true;
    }

private:
    handle_type _handle{};
};

/// Create the coroutine return object.
/// @return The generator that owns this coroutine state.
template <typename tValue>
auto impl::CoGeneratorPromise<tValue>::get_return_object() -> CoGenerator<tValue> {
    return CoGenerator<tValue>{std::coroutine_handle<CoGeneratorPromise<tValue>>::from_promise(*this)};
}

}
