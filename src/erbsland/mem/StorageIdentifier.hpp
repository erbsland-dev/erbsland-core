// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/StorageIdentifierImpl.hpp"

#include "../core/Definitions.hpp"

#include <array>
#include <compare>
#include <cstdint>

namespace erbsland::mem {

/// A stable identifier for a backend storage range.
///
/// This identifier is meant for identity checks, not for ordering by content. It identifies a backend storage range
/// without exposing the memory addresses used to derive it.
/// @seedoc{/reference/mem/cow_storage}
/// @tested{StringReaderTest}
class StorageIdentifier final {
public:
    /// Create an empty identifier.
    constexpr StorageIdentifier() noexcept = default;

public: // operators
    /// Compare two identifiers.
    [[nodiscard]] constexpr auto operator<=>(const StorageIdentifier &other) const noexcept -> std::strong_ordering {
        return _values <=> other._values;
    }
    /// Test if two identifiers are equal.
    [[nodiscard]] constexpr auto operator==(const StorageIdentifier &other) const noexcept -> bool {
        return _values == other._values;
    }
    /// Test if two identifiers differ.
    [[nodiscard]] constexpr auto operator!=(const StorageIdentifier &other) const noexcept -> bool {
        return !operator==(other);
    }

public: // tests
    /// Test if this identifier is empty.
    [[nodiscard]] constexpr auto isEmpty() const noexcept -> bool { return _values[0] == 0 && _values[1] == 0; }

public: // conversion
    /// Return the mixed identity values.
    [[nodiscard]] constexpr auto toRawValues() const noexcept -> std::array<uint64_t, 2> { return _values; }

public:
    /// Create an identifier from the first and one-past-last address of a visible storage range.
    [[nodiscard]] static auto fromMemoryRange(const void *begin, const void *end) noexcept -> StorageIdentifier {
        if (begin == nullptr && end == nullptr) {
            return {};
        }
        return StorageIdentifier{impl::StorageIdentifierMix{begin, end}.toValues()};
    }

private:
    /// Create an identifier from its mixed storage values.
    constexpr explicit StorageIdentifier(const std::array<uint64_t, 2> values) noexcept : _values{values} {}

private:
    std::array<uint64_t, 2> _values{0, 0}; ///< The identity values.
};

}
