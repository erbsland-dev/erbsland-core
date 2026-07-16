// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Character.hpp"

#include "../../../util/HashHelper.hpp"

#include <memory>
#include <vector>

namespace erbsland::re::impl {

/// A character sequence optimized for later use in the compiler.
class CharSequence {
    using SequencePtr = std::shared_ptr<std::vector<text::Char>>;

public:
    /// Create an empty sequence.
    CharSequence();

    // defaults
    ~CharSequence() = default;
    CharSequence(const CharSequence &other) noexcept = default;
    auto operator=(const CharSequence &other) noexcept -> CharSequence & = default;

public: // operators
    auto operator==(const CharSequence &other) const noexcept -> bool;
    auto operator!=(const CharSequence &other) const noexcept -> bool;

public: // accessors
    /// Get the size of this character sequence.
    [[nodiscard]] auto size() const noexcept -> std::size_t;
    /// Access the sequence.
    [[nodiscard]] auto sequence() const noexcept -> const SequencePtr &;
    /// Access the hash.
    [[nodiscard]] auto hash() const noexcept -> std::size_t;
    /// Access the sequence.
    [[nodiscard]] auto begin() const noexcept -> std::vector<text::Char>::const_iterator;
    /// Access the sequence.
    [[nodiscard]] auto end() const noexcept -> std::vector<text::Char>::const_iterator;

public: // modifiers.
    /// Append a new character to this sequence.
    void append(text::Char character);

private:
    /// Create a new instance of the sequence if necessary.
    void conditionalDetach();

private:
    SequencePtr _sequence; ///< The shared character sequence.
    std::size_t _hash = 0; ///< The hash for this sequence.
};

}

namespace std {
template <>
struct hash<erbsland::re::impl::CharSequence> {
    auto operator()(const erbsland::re::impl::CharSequence &charSequence) const noexcept -> std::size_t {
        return charSequence.hash();
    }
};
}
