// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Version.hpp"

#include "../util/HashHelper.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <compare>
#include <optional>
#include <utility>

namespace erbsland::unit {

/// An inclusive range of versions.
///
/// A version range may have no minimum, no maximum, or neither bound. Existing bounds are inclusive. Precision-limited
/// containment uses the same comparison rules as Version::compare().
///
/// @tested{VersionTest}
class VersionRange {
public:
    /// Create an open-ended range.
    constexpr VersionRange() noexcept = default;
    /// Create a range from optional inclusive bounds.
    constexpr VersionRange(std::optional<Version> minimum, std::optional<Version> maximum) noexcept :
        _minimum{minimum}, _maximum{maximum} {}

    /// Destroy this version range.
    ~VersionRange() = default;
    /// Copy a version range.
    VersionRange(const VersionRange &) noexcept = default;
    /// Copy another version range into this range.
    auto operator=(const VersionRange &) noexcept -> VersionRange & = default;

public: // operators
    /// Compare this range with another range.
    constexpr auto operator<=>(const VersionRange &other) const noexcept -> std::strong_ordering {
        if (const auto result = _minimum.has_value() <=> other._minimum.has_value();
            result != std::strong_ordering::equal) {
            return result;
        }
        if (_minimum.has_value()) {
            if (const auto result = *_minimum <=> *other._minimum; result != std::strong_ordering::equal) {
                return result;
            }
        }
        if (const auto result = _maximum.has_value() <=> other._maximum.has_value();
            result != std::strong_ordering::equal) {
            return result;
        }
        if (_maximum.has_value()) {
            return *_maximum <=> *other._maximum;
        }
        return std::strong_ordering::equal;
    }
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FROM_SPACESHIP(const VersionRange &other, other);

public: // tests
    /// Test if this range has a minimum bound.
    [[nodiscard]] constexpr auto hasMinimum() const noexcept -> bool { return _minimum.has_value(); }
    /// Test if this range has a maximum bound.
    [[nodiscard]] constexpr auto hasMaximum() const noexcept -> bool { return _maximum.has_value(); }
    /// Test if this range is empty using the selected precision.
    [[nodiscard]] constexpr auto isEmpty(VersionPart precision = VersionPart::Build) const noexcept -> bool {
        return _minimum.has_value() && _maximum.has_value() &&
            _minimum->compare(*_maximum, precision) == std::strong_ordering::greater;
    }
    /// Test if this range contains the given version using the selected precision.
    [[nodiscard]] constexpr auto contains(
        const Version &version, VersionPart precision = VersionPart::Build) const noexcept -> bool {
        if (isEmpty(precision)) {
            return false;
        }
        if (_minimum.has_value() && version.compare(*_minimum, precision) == std::strong_ordering::less) {
            return false;
        }
        if (_maximum.has_value() && version.compare(*_maximum, precision) == std::strong_ordering::greater) {
            return false;
        }
        return true;
    }

public: // accessors and modifiers
    /// Get the inclusive minimum bound.
    [[nodiscard]] constexpr auto minimum() const noexcept -> std::optional<Version> { return _minimum; }
    /// Set the inclusive minimum bound.
    constexpr void setMinimum(const std::optional<Version> minimum) noexcept { _minimum = minimum; }
    /// Get the inclusive maximum bound.
    [[nodiscard]] constexpr auto maximum() const noexcept -> std::optional<Version> { return _maximum; }
    /// Set the inclusive maximum bound.
    constexpr void setMaximum(const std::optional<Version> maximum) noexcept { _maximum = maximum; }
    /// Swap two version ranges.
    friend constexpr void swap(VersionRange &first, VersionRange &second) noexcept {
        std::swap(first._minimum, second._minimum);
        std::swap(first._maximum, second._maximum);
    }

public: // factory methods
    /// Return a range without bounds.
    [[nodiscard]] constexpr static auto all() noexcept -> VersionRange { return VersionRange{}; }
    /// Return a range with only an inclusive minimum bound.
    [[nodiscard]] constexpr static auto atLeast(Version minimum) noexcept -> VersionRange {
        return VersionRange{minimum, std::nullopt};
    }
    /// Return a range with only an inclusive maximum bound.
    [[nodiscard]] constexpr static auto atMost(Version maximum) noexcept -> VersionRange {
        return VersionRange{std::nullopt, maximum};
    }
    /// Return a range with inclusive minimum and maximum bounds.
    [[nodiscard]] constexpr static auto between(Version minimum, Version maximum) noexcept -> VersionRange {
        return VersionRange{minimum, maximum};
    }
    /// Return a range that contains exactly one version.
    [[nodiscard]] constexpr static auto exact(Version version) noexcept -> VersionRange {
        return VersionRange{version, version};
    }

private:
    std::optional<Version> _minimum; ///< The optional inclusive minimum bound.
    std::optional<Version> _maximum; ///< The optional inclusive maximum bound.
};

constexpr auto Version::inRange(const VersionRange &range, VersionPart precision) const noexcept -> bool {
    return range.contains(*this, precision);
}

}

namespace std {
template <>
struct hash<erbsland::unit::VersionRange> {
    auto operator()(const erbsland::unit::VersionRange &value) const noexcept -> std::size_t {
        return erbsland::util::createHash(value.hasMinimum(), value.minimum(), value.hasMaximum(), value.maximum());
    }
};
}
