// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "VersionRange_fwd.hpp"
#include "VersionUnit.hpp"

#include "../text/StringEditor.hpp"
#include "../util/HashHelper.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <compare>
#include <cstdint>
#include <utility>

namespace erbsland::unit {

/// A version consisting of major, minor, revision, and build number.
///
/// The default version is `0.0.0.0`. Version comparison is lexicographic by all four parts unless a precision is
/// supplied. With a precision, all less-significant parts are ignored.
///
/// @tested{VersionTest}
class Version {
public:
    /// The raw unsigned integer type used for each version part.
    using Value = uint16_t;

public:
    /// Create version `0.0.0.0`.
    constexpr Version() noexcept = default;
    /// Create a version from raw values.
    explicit constexpr Version(Value major, Value minor = 0U, Value revision = 0U, Value build = 0U) noexcept :
        _major{major}, _minor{minor}, _revision{revision}, _build{build} {}
    /// Create a version from typed version parts in any order.
    template <impl::VersionComponentArgument... tArguments>
        requires(sizeof...(tArguments) > 0 && impl::UniqueVersionComponentArguments<tArguments...>)
    explicit constexpr Version(tArguments... arguments) noexcept {
        (setPart(arguments), ...);
    }

    // defaults
    ~Version() = default;
    Version(const Version &) noexcept = default;
    auto operator=(const Version &) noexcept -> Version & = default;

public: // operators
    /// Compare this version with another version at full precision.
    constexpr auto operator<=>(const Version &other) const noexcept -> std::strong_ordering {
        return compare(other, VersionPart::Build);
    }
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FROM_SPACESHIP(const Version &other, other);

public: // tests
    /// Test if this version is in the given range using the selected precision.
    [[nodiscard]] constexpr auto inRange(
        const VersionRange &range, VersionPart precision = VersionPart::Build) const noexcept -> bool;

public: // accessors and modifiers
    /// Get the major version number.
    [[nodiscard]] constexpr auto major() const noexcept -> Major { return _major; }
    /// Set the major version number.
    constexpr void setMajor(Major major) noexcept { _major = major; }
    /// Get the minor version number.
    [[nodiscard]] constexpr auto minor() const noexcept -> Minor { return _minor; }
    /// Set the minor version number.
    constexpr void setMinor(Minor minor) noexcept { _minor = minor; }
    /// Get the revision version number.
    [[nodiscard]] constexpr auto revision() const noexcept -> Revision { return _revision; }
    /// Set the revision version number.
    constexpr void setRevision(Revision revision) noexcept { _revision = revision; }
    /// Get the build version number.
    [[nodiscard]] constexpr auto build() const noexcept -> BuildNumber { return _build; }
    /// Set the build version number.
    constexpr void setBuild(BuildNumber build) noexcept { _build = build; }

public:
    /// Compare this version with another version using the given precision.
    [[nodiscard]] constexpr auto compare(const Version &other, VersionPart precision) const noexcept
        -> std::strong_ordering {
        if (const auto result = _major <=> other._major; result != std::strong_ordering::equal) {
            return result;
        }
        if (precision == VersionPart::Major) {
            return std::strong_ordering::equal;
        }
        if (const auto result = _minor <=> other._minor; result != std::strong_ordering::equal) {
            return result;
        }
        if (precision == VersionPart::Minor) {
            return std::strong_ordering::equal;
        }
        if (const auto result = _revision <=> other._revision; result != std::strong_ordering::equal) {
            return result;
        }
        if (precision == VersionPart::Revision) {
            return std::strong_ordering::equal;
        }
        return _build <=> other._build;
    }
    /// Swap two versions.
    friend constexpr void swap(Version &first, Version &second) noexcept {
        std::swap(first._major, second._major);
        std::swap(first._minor, second._minor);
        std::swap(first._revision, second._revision);
        std::swap(first._build, second._build);
    }

public: // conversion
    /// Convert this version into dotted decimal text.
    /// @param precision The least significant version part to include.
    /// @return The version text.
    [[nodiscard]] auto toString(VersionPart precision = VersionPart::Revision) const -> text::String;
    /// Convert this version into a packed 64-bit number.
    [[nodiscard]] constexpr auto toNumber() const noexcept -> uint64_t {
        return (static_cast<uint64_t>(_major.toRawValue()) << 48U) |
            (static_cast<uint64_t>(_minor.toRawValue()) << 32U) |
            (static_cast<uint64_t>(_revision.toRawValue()) << 16U) | static_cast<uint64_t>(_build.toRawValue());
    }
    /// Create a version from a packed 64-bit number.
    [[nodiscard]] constexpr static auto fromNumber(uint64_t value) noexcept -> Version {
        return Version{
            static_cast<Value>((value >> 48U) & 0xffffU),
            static_cast<Value>((value >> 32U) & 0xffffU),
            static_cast<Value>((value >> 16U) & 0xffffU),
            static_cast<Value>(value & 0xffffU)};
    }

private:
    /// Set the major version part.
    constexpr void setPart(Major part) noexcept { _major = part; }
    /// Set the minor version part.
    constexpr void setPart(Minor part) noexcept { _minor = part; }
    /// Set the revision version part.
    constexpr void setPart(Revision part) noexcept { _revision = part; }
    /// Set the build version part.
    constexpr void setPart(BuildNumber part) noexcept { _build = part; }

private:
    Major _major{};       ///< The major version number.
    Minor _minor{};       ///< The minor version number.
    Revision _revision{}; ///< The revision version number.
    BuildNumber _build{}; ///< The build version number.
};

}

namespace std {
/// Hashes a version by its parts.
template <>
struct hash<erbsland::unit::Version> {
    /// Calculate a version hash.
    auto operator()(const erbsland::unit::Version &value) const noexcept -> std::size_t {
        return erbsland::util::createHash(value.major(), value.minor(), value.revision(), value.build());
    }
};
}
