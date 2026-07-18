// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String.hpp"
#include "../text/StringEditor.hpp"

#include <cstddef>

namespace erbsland::system {

/// A platform user identifier.
/// POSIX stores the numeric UID as text, Windows stores the SID string.
/// @tested{UserLookupTest}
class UserId final {
public:
    /// Create an empty user identifier.
    UserId() = default;
    /// Create a user identifier from its platform representation.
    explicit UserId(const text::String &value) : _value{value} {}

    // defaults
    ~UserId() = default;
    UserId(const UserId &) = default;
    UserId(UserId &&) noexcept = default;
    auto operator=(const UserId &) -> UserId & = default;
    auto operator=(UserId &&) noexcept -> UserId & = default;

public: // operators
    /// Compare two identifiers.
    auto operator==(const UserId &other) const noexcept -> bool = default;

public: // accessors
    /// Test if this identifier is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _value.isEmpty(); }
    /// Access the platform representation.
    [[nodiscard]] auto value() const noexcept -> const text::String & { return _value; }

public: // conversion
    /// Convert this identifier to text.
    [[nodiscard]] auto toString() const -> text::String { return _value; }
    /// Get a stable hash for this identifier.
    [[nodiscard]] auto hash() const noexcept -> std::size_t { return _value.toHash(); }

private:
    text::String _value; ///< Platform representation of the identifier.
};

}
