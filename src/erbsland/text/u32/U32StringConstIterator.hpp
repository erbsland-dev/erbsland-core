// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32String.hpp"
#include "U32StringConstIterator_fwd.hpp"
#include "U32StringEditor_fwd.hpp"

#include "../Char.hpp"

#include "../../unit/CpIndex.hpp"

#include <cstddef>

namespace erbsland::text {

/// A minimal const iterator for UTF-32 encoded strings.
/// @tested{U32StringTest}
class U32StringConstIterator final {
    friend class U32StringEditor;
    friend class U32String;

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
    U32StringConstIterator();
    // defaults
    ~U32StringConstIterator() = default;
    U32StringConstIterator(const U32StringConstIterator &) = default;
    U32StringConstIterator(U32StringConstIterator &&) noexcept = default;
    auto operator=(const U32StringConstIterator &) -> U32StringConstIterator & = default;
    auto operator=(U32StringConstIterator &&) noexcept -> U32StringConstIterator & = default;

public:
    /// Test if this iterator points to the same position as another iterator.
    auto operator==(const U32StringConstIterator &other) const noexcept -> bool;
    /// Test if this iterator points to the same position as another iterator.
    auto operator!=(const U32StringConstIterator &other) const noexcept -> bool;

public:
    /// Test if this iterator is valid.
    [[nodiscard]] auto isValid() const noexcept -> bool;
    /// Access the character at the current position.
    /// This returns `Char::null()` if the iterator is invalid.
    auto operator*() const -> Char;
    /// Increment this iterator to the next position
    auto operator++() -> U32StringConstIterator &;
    /// Post-increment this iterator to the next position
    auto operator++(int) -> U32StringConstIterator;
    /// Access the character at the current position through pointer semantics.
    /// This returns `nullptr` if the iterator is invalid.
    auto operator->() const -> const Char *;

private:
    /// Create an iterator pointing to the given position in the given view.
    U32StringConstIterator(const U32String &view, unit::CpIndex index);

private:
    U32String _string;                              ///< The string accessed by this iterator.
    unit::CpIndex _index{unit::CpIndex::noIndex()}; ///< The current code-point index within the storage.
    mutable Char _currentChar;                      ///< Current character cache for pointer semantics.
};

}
