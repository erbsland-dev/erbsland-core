// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String.hpp"
#include "../text/StringEditor.hpp"

#include <cstddef>

namespace erbsland::system {

/// A platform user name with an optional domain.
/// @tested{UserLookupTest}
class UserName final {
public:
    /// Create an empty user name.
    UserName() = default;
    /// Create a user name without a domain.
    explicit UserName(const text::String &name) : _name{name} {}
    /// Create a user name with an optional domain.
    UserName(const text::String &name, const text::String &domain) : _name{name}, _domain{domain} {}

    // defaults
    ~UserName() = default;
    UserName(const UserName &) = default;
    UserName(UserName &&) noexcept = default;
    auto operator=(const UserName &) -> UserName & = default;
    auto operator=(UserName &&) noexcept -> UserName & = default;

public: // operators
    /// Compare two names.
    auto operator==(const UserName &other) const noexcept -> bool = default;

public: // accessors
    /// Test if this user name is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _name.isEmpty(); }
    /// Access the account name without its domain.
    [[nodiscard]] auto name() const noexcept -> const text::String & { return _name; }
    /// Access the optional domain.
    [[nodiscard]] auto domain() const noexcept -> const text::String & { return _domain; }

public: // conversion
    /// Convert this name to display text.
    [[nodiscard]] auto toString() const -> text::String;
    /// Create a name from display text, splitting a Windows-style domain if present.
    [[nodiscard]] static auto fromString(const text::String &text) -> UserName;
    /// Get a stable hash for this name.
    [[nodiscard]] auto hash() const noexcept -> std::size_t;

private:
    text::String _name;   ///< The account name.
    text::String _domain; ///< The optional domain.
};

}
