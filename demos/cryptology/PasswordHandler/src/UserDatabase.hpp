// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/core/MakeOneNamespace.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/path/PathCollisionMode.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringMap.hpp>

namespace demo {

/// The password-hash records loaded from the demo's ELCL user database.
/// Usernames use exact case-sensitive UTF-8 comparison and deterministic ordering.
/// @notest{Compiled and exercised as part of the Password Handler demo.}
class UserDatabase final {
public:
    /// Load and validate a user database.
    /// @param path The ELCL database path.
    /// @return The loaded database.
    [[nodiscard]] static auto load(const el::Path &path) -> UserDatabase;

public: // tests
    /// Test whether a username exists.
    [[nodiscard]] auto contains(const el::String &username) const -> bool;

public: // access
    /// Return the stored hash text, or an empty string for an unknown user.
    [[nodiscard]] auto passwordHash(const el::String &username) const -> el::String;
    /// Access all users in deterministic username order.
    [[nodiscard]] auto users() const noexcept -> const el::StringMap<el::String> & { return _users; }

public: // change
    /// Insert a new user if the exact username does not exist.
    [[nodiscard]] auto tryAdd(const el::String &username, const el::String &passwordHash) -> bool;
    /// Replace an existing user's hash.
    [[nodiscard]] auto trySetPassword(const el::String &username, const el::String &passwordHash) -> bool;
    /// Remove an existing user.
    [[nodiscard]] auto tryRemove(const el::String &username) -> bool;

public: // persistence
    /// Serialize and atomically save this database.
    void save(const el::Path &path, el::PathCollisionMode collisionMode) const;

private:
    el::StringMap<el::String> _users; ///< Canonical hash text by exact username.
};

}
