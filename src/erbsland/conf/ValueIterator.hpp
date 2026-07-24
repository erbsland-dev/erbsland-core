// Copyright (c) 2024-2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Value_fwd.hpp"

#include "../core/Definitions.hpp"

#include <iterator>
#include <memory>
#include <vector>

namespace erbsland::conf {

namespace impl {
class Value;
using ValuePtr = std::shared_ptr<Value>;
}

/// Const iterator for the `Value` class.
/// This is a simple wrapper around the iterator of the internally used container.
class ValueIterator final {
    /// @private
    using WrappedIterator = std::vector<impl::ValuePtr>::const_iterator;

public:
    /// @private
    using iterator_category = std::bidirectional_iterator_tag;
    /// @private
    using value_type = const ValuePtr;
    /// @private
    using difference_type = std::ptrdiff_t;
    /// @private
    using pointer = const ValuePtr;
    /// @private
    using reference = const ValuePtr;

public:
    /// @name Construction and Assignment
    /// @{

    /// Create an iterator wrapping the given internal iterator.
    /// @param it The iterator to wrap.
    constexpr explicit ValueIterator(WrappedIterator it) : _current{it} {}

    /// Default constructor.
    ValueIterator() = default;
    /// Default destructor.
    ~ValueIterator() = default;
    /// Default copy constructor.
    ValueIterator(const ValueIterator &) = default;
    /// Default move constructor.
    ValueIterator(ValueIterator &&) noexcept = default;
    /// Default copy assignment.
    auto operator=(const ValueIterator &) -> ValueIterator & = default;
    /// Default move assignment.
    auto operator=(ValueIterator &&) noexcept -> ValueIterator & = default;
    /// @}

public:
    /// @name Operators
    /// @{

    /// Dereference operator.
    /// @return A shared pointer to the current value.
    [[nodiscard]] auto operator*() const noexcept -> reference;
    /// Member access operator.
    /// @return A pointer to the current value.
    [[nodiscard]] auto operator->() const noexcept -> pointer;
    /// Prefix increment. Advances the iterator to the next element.
    auto operator++() noexcept -> ValueIterator &;
    /// Postfix increment. Advances the iterator and returns the previous state.
    auto operator++(int) noexcept -> ValueIterator;
    /// Prefix decrement. Moves the iterator to the previous element.
    auto operator--() noexcept -> ValueIterator &;
    /// Postfix decrement. Moves the iterator to the previous element and returns the previous state.
    auto operator--(int) noexcept -> ValueIterator;
    /// Equality comparison.
    /// @param other The other iterator for comparison.
    [[nodiscard]] auto operator==(const ValueIterator &other) const noexcept -> bool;
    /// Inequality comparison.
    /// @param other The other iterator for comparison.
    [[nodiscard]] auto operator!=(const ValueIterator &other) const noexcept -> bool;

    /// @}

private:
    WrappedIterator _current;
};

}
