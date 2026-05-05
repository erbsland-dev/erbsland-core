// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Tag.hpp"

#include <bit>
#include <cstdint>
#include <functional>
#include <initializer_list>

namespace erbsland::cterm::theme {

/// A compact set of theme tags.
class Tags final {
public:
    /// Create an empty tag set.
    constexpr Tags() noexcept = default;
    /// Create a tag set containing one tag.
    /// @param tag The initial tag.
    constexpr Tags(const Tag tag) noexcept : _bits{tag.mask()} {}
    /// Create a tag set from raw bits.
    /// @param bits The raw tag bits.
    explicit constexpr Tags(const uint64_t bits) noexcept : _bits{bits} {}
    /// Create a tag set from an initializer list.
    /// @param tags The tags to add.
    constexpr Tags(std::initializer_list<Tag> tags) noexcept {
        for (const auto tag : tags) {
            add(tag);
        }
    }

public: // operators
    /// Compare two tag sets.
    auto operator==(const Tags &other) const noexcept -> bool = default;
    /// Compare two tag sets.
    auto operator!=(const Tags &other) const noexcept -> bool = default;
    /// Combine two tag sets.
    [[nodiscard]] constexpr auto operator|(const Tags &other) const noexcept -> Tags {
        return Tags{_bits | other._bits};
    }
    /// Add all tags from another tag set.
    constexpr auto operator|=(const Tags &other) noexcept -> Tags & {
        _bits |= other._bits;
        return *this;
    }

public:
    /// Add one tag.
    /// @param tag The tag to add.
    constexpr void add(const Tag tag) noexcept { _bits |= tag.mask(); }
    /// Add all tags from another tag set.
    /// @param tags The tags to add.
    constexpr void add(const Tags tags) noexcept { _bits |= tags._bits; }
    /// Remove one tag.
    /// @param tag The tag to remove.
    constexpr void remove(const Tag tag) noexcept { _bits &= ~tag.mask(); }
    /// Test if one tag is present.
    /// @param tag The tag to test.
    [[nodiscard]] constexpr auto contains(const Tag tag) const noexcept -> bool { return (_bits & tag.mask()) != 0; }
    /// Test if all tags from another set are present.
    /// @param tags The required tags.
    [[nodiscard]] constexpr auto containsAll(const Tags tags) const noexcept -> bool {
        return (_bits & tags._bits) == tags._bits;
    }
    /// Test if the set is empty.
    [[nodiscard]] constexpr auto empty() const noexcept -> bool { return _bits == 0; }
    /// Get the number of tags in the set.
    [[nodiscard]] auto size() const noexcept -> int { return std::popcount(_bits); }
    /// Get the raw tag bits.
    [[nodiscard]] constexpr auto bits() const noexcept -> uint64_t { return _bits; }
    /// Get a stable hash for this tag set.
    [[nodiscard]] constexpr auto hash() const noexcept -> std::size_t { return static_cast<std::size_t>(_bits); }

private:
    uint64_t _bits{0}; ///< The raw tag bits.
};

}

template <>
struct std::hash<erbsland::cterm::theme::Tags> {
    auto operator()(const erbsland::cterm::theme::Tags tags) const noexcept -> std::size_t { return tags.hash(); }
};
