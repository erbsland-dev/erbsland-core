// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CoGeneratorPromise.hpp"

#include "../../err/LogicError.hpp"

#include <coroutine>
#include <cstddef>
#include <exception>
#include <iterator>

namespace erbsland::util::impl {

/// Provide single-pass range iteration over one generator coroutine.
template <typename tValue>
class CoGeneratorIterator {
public:
    using Value = tValue;
    using Promise = CoGeneratorPromise<tValue>;
    using Handle = std::coroutine_handle<Promise>;
    using iterator_category = std::input_iterator_tag;
    using value_type = Value;
    using difference_type = std::ptrdiff_t;
    using reference = const Value &;
    using pointer = const Value *;

    /// Create an end iterator.
    CoGeneratorIterator() = default;
    /// Create an iterator for a coroutine handle.
    explicit CoGeneratorIterator(const Handle handle) noexcept : _handle{handle} {}

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
    auto operator++() -> CoGeneratorIterator & {
        resume();
        return *this;
    }
    /// Advance to the next yielded value.
    /// @throws Any exception that escaped from the coroutine body.
    void operator++(int) { resume(); }
    /// Test if two iterators point to the same coroutine state.
    /// @param other The iterator to compare with.
    /// @return `true` if both iterators have the same handle.
    [[nodiscard]] auto operator==(const CoGeneratorIterator &other) const noexcept -> bool {
        return _handle == other._handle;
    }
    /// Test if two iterators point to different coroutine states.
    /// @param other The iterator to compare with.
    /// @return `true` if both iterators have different handles.
    [[nodiscard]] auto operator!=(const CoGeneratorIterator &other) const noexcept -> bool { return !(*this == other); }

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
    Handle _handle{};
};

}
