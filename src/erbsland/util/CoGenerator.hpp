// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CoGenerator_fwd.hpp"

#include "../err/LogicError.hpp"

#include <concepts>
#include <coroutine>
#include <cstddef>
#include <exception>
#include <iterator>
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
    struct promise_type;
    class Iterator;

    using Value = tValue;                                    ///< The value type yielded by this generator.
    using handle_type = std::coroutine_handle<promise_type>; ///< The standard coroutine handle type.

    /// The promise type used by the C++ coroutine machinery.
    /// @tested{CoGeneratorTest}
    struct promise_type {
        friend class CoGenerator;
        friend class Iterator;

    public:
        /// Create the coroutine return object.
        /// @return The generator that owns this coroutine state.
        auto get_return_object() -> CoGenerator { return CoGenerator{handle_type::from_promise(*this)}; }
        /// Suspend before the first coroutine statement.
        /// @return A suspension token that keeps the coroutine lazy.
        auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        /// Suspend after the coroutine has finished so the owner can destroy the frame.
        /// @return A suspension token for the final suspend point.
        auto final_suspend() noexcept -> std::suspend_always { return {}; }
        /// Store a yielded value in the coroutine promise.
        /// @tparam tYielded The concrete yielded value type.
        /// @param value The value to store for the next consumer access.
        /// @return A suspension token that pauses the coroutine after the value is stored.
        template <typename tYielded>
            requires std::constructible_from<Value, tYielded>
        auto yield_value(tYielded &&value) -> std::suspend_always {
            _currentValue.emplace(std::forward<tYielded>(value));
            return {};
        }
        /// Finish the coroutine without a final value.
        void return_void() noexcept {}
        /// Store an unhandled coroutine exception until the consumer resumes or accesses the generator.
        void unhandled_exception() noexcept { _exception = std::current_exception(); }

    private:
        std::optional<Value> _currentValue{};
        std::exception_ptr _exception{};
    };

    /// A single-pass input iterator over the yielded values.
    /// @tested{CoGeneratorTest}
    class Iterator {
    public:
        using iterator_category = std::input_iterator_tag; ///< The standard iterator category.
        using value_type = Value;                          ///< The standard iterator value type.
        using difference_type = std::ptrdiff_t;            ///< The standard iterator difference type.
        using reference = const Value &;                   ///< The iterator reference type.
        using pointer = const Value *;                     ///< The iterator pointer type.

    public:
        /// Create an end iterator.
        Iterator() = default;
        /// Create an iterator for a coroutine handle.
        /// @param handle The active coroutine handle.
        explicit Iterator(handle_type handle) noexcept : _handle{handle} {}

    public: // operators
        /// Access the current yielded value.
        /// @return A reference to the current value.
        /// @throws err::LogicError If this iterator does not point to a yielded value.
        [[nodiscard]] auto operator*() const -> reference { return currentValue(); }
        /// Access the current yielded value.
        /// @return A pointer to the current value.
        /// @throws err::LogicError If this iterator does not point to a yielded value.
        [[nodiscard]] auto operator->() const -> pointer { return &currentValue(); }
        /// Advance to the next yielded value.
        /// @return A reference to this iterator.
        /// @throws Any exception that escaped from the coroutine body.
        auto operator++() -> Iterator & {
            resume();
            return *this;
        }
        /// Advance to the next yielded value.
        /// @throws Any exception that escaped from the coroutine body.
        void operator++(int) { resume(); }
        /// Test if two iterators point to the same coroutine state.
        /// @param other The iterator to compare with.
        /// @return `true` if both iterators have the same handle.
        [[nodiscard]] auto operator==(const Iterator &other) const noexcept -> bool { return _handle == other._handle; }
        /// Test if two iterators point to different coroutine states.
        /// @param other The iterator to compare with.
        /// @return `true` if both iterators have different handles.
        [[nodiscard]] auto operator!=(const Iterator &other) const noexcept -> bool { return !(*this == other); }

    private:
        /// Access the current yielded value.
        /// @return A reference to the current value.
        [[nodiscard]] auto currentValue() const -> reference {
            if (!_handle || _handle.done() || !_handle.promise()._currentValue.has_value()) {
                throw err::LogicError{"Dereferencing an invalid CoGenerator iterator."};
            }
            if (_handle.promise()._exception) {
                std::rethrow_exception(_handle.promise()._exception);
            }
            return *_handle.promise()._currentValue;
        }
        /// Resume the coroutine and update this iterator state.
        /// @throws Any exception that escaped from the coroutine body.
        void resume() {
            if (!_handle) {
                throw err::LogicError{"Incrementing an invalid CoGenerator iterator."};
            }
            if (_handle.done()) {
                _handle.promise()._currentValue.reset();
                _handle = {};
                return;
            }
            _handle.resume();
            auto &promise = _handle.promise();
            if (promise._exception) {
                promise._currentValue.reset();
                std::rethrow_exception(promise._exception);
            }
            if (_handle.done()) {
                promise._currentValue.reset();
                _handle = {};
            }
        }

    private:
        handle_type _handle{};
    };

    using iterator = Iterator; ///< The standard iterator type.

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

}
