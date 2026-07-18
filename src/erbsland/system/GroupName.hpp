// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String.hpp"
#include "../text/StringEditor.hpp"

#include <cstddef>

namespace erbsland::system {

/// A platform group name with an optional domain.
/// @tested{UserLookupTest}
class GroupName final {
public:
    /// Create an empty group name.
    GroupName() = default;
    /// Create a group name without a domain.
    explicit GroupName(const text::String &name) : _name{name} {}
    /// Create a group name with an optional domain.
    GroupName(const text::String &name, const text::String &domain) : _name{name}, _domain{domain} {}

    // defaults
    ~GroupName() = default;
    GroupName(const GroupName &) = default;
    GroupName(GroupName &&) noexcept = default;
    auto operator=(const GroupName &) -> GroupName & = default;
    auto operator=(GroupName &&) noexcept -> GroupName & = default;

public: // operators
    /// Compare two names.
    auto operator==(const GroupName &other) const noexcept -> bool = default;

public: // accessors
    /// Test if this group name is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _name.isEmpty(); }
    /// Access the group name without its domain.
    [[nodiscard]] auto name() const noexcept -> const text::String & { return _name; }
    /// Access the optional domain.
    [[nodiscard]] auto domain() const noexcept -> const text::String & { return _domain; }

public: // conversion
    /// Convert this name to display text.
    [[nodiscard]] auto toString() const -> text::String;
    /// Create a name from display text, splitting a Windows-style domain if present.
    [[nodiscard]] static auto fromString(const text::String &text) -> GroupName;
    /// Get a stable hash for this name.
    [[nodiscard]] auto hash() const noexcept -> std::size_t;

private:
    text::String _name;   ///< The group name.
    text::String _domain; ///< The optional domain.
};

}
