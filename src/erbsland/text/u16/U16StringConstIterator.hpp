// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16String_fwd.hpp"
#include "U16StringConstIterator_fwd.hpp"
#include "U16StringView_fwd.hpp"

#include "../Char.hpp"

#include "../../unit/U16DataIndex.hpp"

#include <cstddef>
#include <memory>

namespace erbsland::text {

/// A minimal const iterator for UTF-16 encoded strings.
/// @tested{U16StringTest}
class U16StringConstIterator final {
    struct Private;
    friend class U16String;
    friend class U16StringView;

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
    U16StringConstIterator();
    /// Create a copy of another iterator.
    U16StringConstIterator(const U16StringConstIterator &other);
    /// Move another iterator into this iterator.
    U16StringConstIterator(U16StringConstIterator &&other) noexcept;
    /// Assign a copy of another iterator.
    auto operator=(const U16StringConstIterator &other) -> U16StringConstIterator &;
    /// Move another iterator into this iterator.
    auto operator=(U16StringConstIterator &&other) noexcept -> U16StringConstIterator &;

    // defaults
    /// Destroy the iterator.
    ~U16StringConstIterator();

public:
    /// Test if this iterator points to the same position as another iterator.
    auto operator==(const U16StringConstIterator &other) const noexcept -> bool;
    /// Test if this iterator points to the same position as another iterator.
    auto operator!=(const U16StringConstIterator &other) const noexcept -> bool;

public:
    /// Test if this iterator is valid.
    [[nodiscard]] auto isValid() const noexcept -> bool;
    /// Access the character at the current position.
    /// This returns `Char::null()` if the iterator is invalid.
    auto operator*() const -> Char;
    /// Increment this iterator to the next position
    auto operator++() -> U16StringConstIterator &;
    /// Post-increment this iterator to the next position
    auto operator++(int) -> U16StringConstIterator;
    /// Access the character at the current position through pointer semantics.
    /// This returns `nullptr` if the iterator is invalid.
    auto operator->() const -> const Char *;

private:
    /// Create an iterator pointing to the given position in the given view.
    U16StringConstIterator(const U16StringView &view, unit::U16DataIndex index);

private:
    std::unique_ptr<Private> _p; ///< Private implementation
    mutable Char _currentChar;   ///< Current character cache for pointer semantics
};

}
