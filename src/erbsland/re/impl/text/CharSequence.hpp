// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Character.hpp"

#include "../../../util/HashHelper.hpp"

#include <array>
#include <memory>
#include <span>
#include <vector>

namespace erbsland::re::impl {

/// A character sequence optimized for later use in the compiler.
class CharSequence {
    static constexpr std::size_t cInlineCapacity = 8U;
    using SequencePtr = std::shared_ptr<std::vector<text::Char>>;
    using ConstIterator = const text::Char *;

public:
    /// Create an empty sequence.
    CharSequence() = default;

    // defaults
    ~CharSequence() = default;
    CharSequence(const CharSequence &other) noexcept = default;
    auto operator=(const CharSequence &other) noexcept -> CharSequence & = default;

public: // operators
    /// Test whether two character sequences have identical contents.
    auto operator==(const CharSequence &other) const noexcept -> bool;
    /// Test whether two character sequences have different contents.
    auto operator!=(const CharSequence &other) const noexcept -> bool;

public: // accessors
    /// Get the size of this character sequence.
    [[nodiscard]] auto size() const noexcept -> std::size_t;
    /// Access the sequence.
    [[nodiscard]] auto sequence() const noexcept -> std::span<const text::Char>;
    /// Access the hash.
    [[nodiscard]] auto hash() const noexcept -> std::size_t;
    /// Access the sequence.
    [[nodiscard]] auto begin() const noexcept -> ConstIterator;
    /// Access the sequence.
    [[nodiscard]] auto end() const noexcept -> ConstIterator;

public: // modifiers.
    /// Append a new character to this sequence.
    void append(text::Char character);

private:
    /// Create a new instance of the sequence if necessary.
    /// Detach shared extended storage before modifying the sequence.
    void conditionalDetach();

private:
    std::array<text::Char, cInlineCapacity> _inlineSequence{}; ///< Allocation-free storage for short sequences.
    std::size_t _inlineSize{};                                 ///< Used characters in the inline storage.
    SequencePtr _extendedSequence;                             ///< Copy-on-write storage for longer sequences.
    std::size_t _hash{};                                       ///< The hash for this sequence.
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
