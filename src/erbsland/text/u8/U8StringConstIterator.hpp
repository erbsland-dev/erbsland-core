// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8String.hpp"
#include "U8StringConstIterator_fwd.hpp"
#include "U8StringEditor_fwd.hpp"

#include "../Char.hpp"

#include "../../unit/ByteIndex.hpp"

#include <cstddef>

namespace erbsland::text {

/// A minimal const iterator for UTF-8 encoded strings.
/// @tested{U8StringConstIteratorTest}
class U8StringConstIterator final {
    friend class U8StringEditor;
    friend class U8String;

public: // iterator traits
    /// Standard iterator category for this iterator.
    using iterator_category = std::forward_iterator_tag;
    /// Value type returned by dereferencing this iterator.
    using value_type = Char;
    /// Difference type required by the iterator interface.
    using difference_type = std::ptrdiff_t;
    /// Pointer type used by `operator->`.
    using pointer = const Char *;
    /// Reference type used by `operator*`.
    using reference = Char;

public:
    /// Create an invalid iterator that does not point to any string.
    U8StringConstIterator();
    // defaults
    ~U8StringConstIterator() = default;
    U8StringConstIterator(const U8StringConstIterator &) = default;
    U8StringConstIterator(U8StringConstIterator &&) noexcept = default;
    auto operator=(const U8StringConstIterator &) -> U8StringConstIterator & = default;
    auto operator=(U8StringConstIterator &&) noexcept -> U8StringConstIterator & = default;

public:
    /// Test if this iterator points to the same position as another iterator.
    auto operator==(const U8StringConstIterator &other) const noexcept -> bool;
    /// Test if this iterator points to the same position as another iterator.
    auto operator!=(const U8StringConstIterator &other) const noexcept -> bool;

public:
    /// Test if this iterator is valid.
    [[nodiscard]] auto isValid() const noexcept -> bool;
    /// Access the character at the current position.
    /// This returns `Char::null()` if the iterator is invalid.
    auto operator*() const -> Char;
    /// Increment this iterator to the next position
    auto operator++() -> U8StringConstIterator &;
    /// Post-increment this iterator to the next position
    auto operator++(int) -> U8StringConstIterator;
    /// Access the character at the current position through pointer semantics.
    /// This returns `nullptr` if the iterator is invalid.
    auto operator->() const -> const Char *;

private:
    /// Create an iterator pointing to the given position in the given view.
    U8StringConstIterator(const U8String &view, unit::ByteIndex index);

private:
    U8String _string;                                   ///< The string accessed by this iterator.
    unit::ByteIndex _index{unit::ByteIndex::noIndex()}; ///< The current byte index within the storage.
    mutable Char _currentChar;                          ///< Current character cache for pointer semantics.
};

}
