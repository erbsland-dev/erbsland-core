// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String.hpp"
#include "../text/StringEditor.hpp"

#include <cstddef>

namespace erbsland::system {

/// A platform group identifier.
/// POSIX stores the numeric GID as text, Windows stores the SID string.
/// @tested{UserLookupTest}
class GroupId final {
public:
    /// Create an empty group identifier.
    GroupId() = default;
    /// Create a group identifier from its platform representation.
    explicit GroupId(const text::String &value) : _value{value} {}

    // defaults
    ~GroupId() = default;
    GroupId(const GroupId &) = default;
    GroupId(GroupId &&) noexcept = default;
    auto operator=(const GroupId &) -> GroupId & = default;
    auto operator=(GroupId &&) noexcept -> GroupId & = default;

public: // operators
    /// Compare two identifiers.
    auto operator==(const GroupId &other) const noexcept -> bool = default;

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
