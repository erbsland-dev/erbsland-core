// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConfVersionRange.hpp"
#include "VersionMask_fwd.hpp"

#include "../../../text/String.hpp"

#include <initializer_list>
#include <type_traits>
#include <utility>
#include <vector>

namespace erbsland::conf::impl {

using namespace text::literals;

/// A set-like mask over non-negative integer versions.
/// Semantics and invariants:
/// - A mask represents a finite union of closed, inclusive ranges.
/// - Public constructors/factories keep the internal representation normalized:
///   ranges are sorted by start, coalesced (overlapping or adjacent are merged),
///   and minimal. An empty mask has no ranges and matches nothing. The default
///   mask matches all versions (>= 0).
class VersionMask final {
public:
    /// Creates a mask that matches all possible versions (>= 0).
    VersionMask() = default;

    /// Creates a mask consisting of a single range.
    /// This constructor does not coalesce with other ranges (by design),
    /// but the provided `ConfVersionRange` is already normalized on construction.
    constexpr VersionMask(const ConfVersionRange &range) noexcept :
        _ranges{{range}} {} // NOLINT(*-explicit-constructor)

private:
    /// Internal constructor to create a mask from ranges.
    template <typename Fwd>
        requires(std::is_same_v<std::remove_cvref_t<Fwd>, std::vector<ConfVersionRange>>)
    constexpr explicit VersionMask(Fwd &&ranges) noexcept : _ranges{std::forward<Fwd>(ranges)} {}

public:
    /// Create an empty mask that matches nothing.
    [[nodiscard]] static auto empty() noexcept -> VersionMask;

    /// Construct a mask from a vector of ranges.
    /// This will normalize the ranges before using them in the mask.
    template <typename Fwd>
        requires(std::is_same_v<std::remove_cvref_t<Fwd>, std::vector<ConfVersionRange>>)
    [[nodiscard]] static auto fromRanges(Fwd &&values) noexcept -> VersionMask {
        return VersionMask{normalize(std::forward<Fwd>(values))};
    }
    /// Construct a mask from an initializer list of ranges.
    [[nodiscard]] static auto fromRanges(const std::initializer_list<ConfVersionRange> values) noexcept -> VersionMask;

    /// Construct a mask that matches any of the given version integers (OR semantics).
    /// Notes:
    /// - Negative values are clamped to 0 by `ConfVersionRange`.
    /// - Duplicates are removed, and adjacent numbers are merged into ranges.
    /// - Empty input creates an empty mask (matches nothing).
    [[nodiscard]] static auto fromIntegers(const std::vector<Integer> &values) noexcept -> VersionMask;
    /// Construct a mask from an initializer list of version integers.
    [[nodiscard]] static auto fromIntegers(const std::initializer_list<Integer> values) noexcept -> VersionMask;

    /// Merge this mask with another one, using OR semantics.
    /// The result is a new, merged, and normalized mask.
    /// @param other The other mask for the merge.
    [[nodiscard]] auto unionWith(const VersionMask &other) const noexcept -> VersionMask;
    /// Merge this mask with another mask one, using OR semantics.
    [[nodiscard]] auto operator|(const VersionMask &other) const noexcept -> VersionMask { return unionWith(other); }
    /// Merge this mask with another mask one, using OR semantics.
    auto operator|=(const VersionMask &other) noexcept -> VersionMask & {
        *this = unionWith(other);
        return *this;
    }

    /// Merge this mask with another one, using AND semantics.
    /// The result is a new, merged, and normalized mask.
    /// @param other The other mask for the merge.
    [[nodiscard]] auto intersectionWith(const VersionMask &other) const noexcept -> VersionMask;
    /// Merge this mask with another one, using AND semantics.
    [[nodiscard]] auto operator&(const VersionMask &other) const noexcept -> VersionMask {
        return intersectionWith(other);
    }
    /// Merge this mask with another one, using AND semantics.
    auto operator&=(const VersionMask &other) noexcept -> VersionMask & {
        *this = intersectionWith(other);
        return *this;
    }

    /// Complement this mask within the universe [0, maxInt()].
    /// Example: !(1-3, 7-10) == (0-0, 4-6, 11-max)
    [[nodiscard]] auto complement() const noexcept -> VersionMask;

    /// Logical NOT operator returning the complement of this mask in the universe [0, max].
    [[nodiscard]] auto operator!() const noexcept -> VersionMask { return complement(); }

public: // accessors
    /// Access all coalesced ranges of this mask (sorted, minimal).
    [[nodiscard]] auto ranges() const noexcept -> const std::vector<ConfVersionRange> & { return _ranges; }

public: // tests
    /// Test if this mask is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _ranges.empty(); }
    /// Test if this mask matches all versions.
    [[nodiscard]] auto isAny() const noexcept -> bool;
    /// Test if a version matches this mask.
    [[nodiscard]] auto matches(const Integer version) const noexcept -> bool;

public: // conversion
    /// Create a compact human-readable text representation.
    /// Examples: "5-6, 10, 14, 17-20", "<=10", ">=40", ">=0" (all), "<none>" (empty)
    [[nodiscard]] auto toText() const noexcept -> text::String;

private: // helper methods
    /// Get the maximum integer for a version.
    [[nodiscard]] static constexpr auto maxInt() noexcept -> Integer { return std::numeric_limits<Integer>::max(); }

    /// Comparison operator to sort the ranges.
    [[nodiscard]] static constexpr auto lessStartThenEnd(const ConfVersionRange &a, const ConfVersionRange &b) noexcept
        -> bool {
        return (a.first < b.first) || (a.first == b.first && a.last < b.last);
    }

    /// Normalize a list of ranges.
    [[nodiscard]] static auto normalize(std::vector<ConfVersionRange> ranges) noexcept -> std::vector<ConfVersionRange>;

private:
    std::vector<ConfVersionRange> _ranges{ConfVersionRange::all()};
};

}
