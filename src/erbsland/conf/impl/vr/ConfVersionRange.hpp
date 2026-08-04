// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../Integer.hpp"

#include <algorithm>
#include <limits>

namespace erbsland::conf::impl {

/// A closed, inclusive range of non-negative versions.
/// @tested{VersionMaskTest}
class ConfVersionRange final {
public:
    /// Create a zero version range 0-0.
    ConfVersionRange() = default;
    /// Create a version range with a single value n-n.
    /// @param value The single version value (>=0).
    constexpr explicit ConfVersionRange(const Integer value) noexcept :
        first{clampVersion(value)}, last{clampVersion(value)} {}
    /// Create a version range from two endpoints.
    /// @param first The first version of the range (>=0).
    /// @param last The last version of the range (>=0).
    constexpr ConfVersionRange(const Integer first, const Integer last) noexcept :
        first{lowerVersion(first, last)}, last{upperVersion(first, last)} {}

    /// Create a version range that covers all valid versions.
    [[nodiscard]] constexpr static auto all() noexcept -> ConfVersionRange {
        return ConfVersionRange{0, std::numeric_limits<Integer>::max()};
    }

public: // tests
    /// Test if this range contains a version number.
    [[nodiscard]] auto matches(const Integer version) const noexcept -> bool {
        return version >= first && version <= last;
    }

private:
    /// Clamp a version number to the supported range.
    [[nodiscard]] constexpr static auto clampVersion(const Integer value) noexcept -> Integer {
        return std::max(value, Integer{0});
    }
    /// Get the normalized lower bound of two versions.
    [[nodiscard]] constexpr static auto lowerVersion(const Integer first, const Integer last) noexcept -> Integer {
        return std::min(clampVersion(first), clampVersion(last));
    }
    /// Get the normalized upper bound of two versions.
    [[nodiscard]] constexpr static auto upperVersion(const Integer first, const Integer last) noexcept -> Integer {
        return std::max(clampVersion(first), clampVersion(last));
    }

public:
    Integer first{};
    Integer last{};
};

}
