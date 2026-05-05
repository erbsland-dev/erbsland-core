// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>
#include <functional>
#include <limits>

namespace erbsland::cterm::theme {

/// A theme tag bit assigned by a theme builder.
class Tag final {
public:
    /// Create an empty tag.
    constexpr Tag() noexcept = default;
    /// Create a tag from a zero-based bit index.
    /// @param bit The zero-based tag bit index.
    explicit constexpr Tag(const uint8_t bit) noexcept : _bit{bit} {}

public: // operators
    /// Compare two tags.
    auto operator==(const Tag &other) const noexcept -> bool = default;
    /// Compare two tags.
    auto operator!=(const Tag &other) const noexcept -> bool = default;

public:
    /// Get the one-based numeric tag value, or zero for an empty tag.
    [[nodiscard]] constexpr auto value() const noexcept -> uint8_t {
        return isValid() ? static_cast<uint8_t>(_bit + 1) : 0;
    }
    /// Get the zero-based tag bit index.
    [[nodiscard]] constexpr auto bit() const noexcept -> uint8_t { return _bit; }
    /// Get the mask for this tag.
    [[nodiscard]] constexpr auto mask() const noexcept -> uint64_t {
        return isValid() ? (uint64_t{1} << _bit) : uint64_t{0};
    }
    /// Test if this tag is non-empty.
    /// @return `true` if this tag references a valid bit.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool { return _bit < cEmptyBit; }
    /// Get a stable hash for this tag.
    /// @return The hash value.
    [[nodiscard]] constexpr auto hash() const noexcept -> std::size_t { return static_cast<std::size_t>(_bit); }

private:
    constexpr static auto cEmptyBit = std::numeric_limits<uint8_t>::max();

private:
    uint8_t _bit{cEmptyBit}; ///< The zero-based tag bit index.
};

}

template <>
struct std::hash<erbsland::cterm::theme::Tag> {
    auto operator()(const erbsland::cterm::theme::Tag &tag) const noexcept -> std::size_t { return tag.hash(); }
};
