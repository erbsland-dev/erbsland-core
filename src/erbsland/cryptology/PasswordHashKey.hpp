// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/PasswordHashData_fwd.hpp"

#include "../mem/ByteBlock.hpp"
#include "../text/String.hpp"

#include <optional>

namespace erbsland::cryptology {

/// A system/application key used as a password-hash pepper.
/// Key material must contain at least 32 bytes. Keep it outside the password database, preferably in a secret manager
/// or operating-system protected key store.
/// @seedoc{/reference/cryptology/password_hashing}
/// @tested{PasswordHasherTest}
class PasswordHashKey final {
    friend class impl::PasswordHashData;

public:
    /// Create the normal unnamed application key.
    /// The key's shared allocation is marked as sensitive by this constructor.
    /// @param key The application-key material.
    /// @throws err::ParameterError If the key contains fewer than 32 bytes.
    explicit PasswordHashKey(mem::ByteBlock key);

    // defaults
    PasswordHashKey(const PasswordHashKey &) = default;
    PasswordHashKey(PasswordHashKey &&) noexcept = default;
    auto operator=(const PasswordHashKey &) -> PasswordHashKey & = default;
    auto operator=(PasswordHashKey &&) noexcept -> PasswordHashKey & = default;

public: // tests
    /// Test whether this key has a public rotation identifier.
    [[nodiscard]] auto isIdentified() const noexcept -> bool { return _identifier.has_value(); }

public: // accessors
    /// Get the public rotation identifier, if present.
    [[nodiscard]] auto identifier() const noexcept -> const std::optional<text::String> & { return _identifier; }

public: // factory
    /// Create an identified key for rotation.
    /// Identifiers contain 1-32 characters from ASCII letters, digits, `.`, `_`, and `-`.
    /// @param identifier The public rotation identifier.
    /// @param key The application-key material, marked as sensitive by the resulting key.
    /// @return The identified key.
    /// @throws err::ParameterError If the key or identifier is invalid.
    [[nodiscard]] static auto identified(text::String identifier, mem::ByteBlock key) -> PasswordHashKey;

private:
    /// Create a validated password-hash key with its optional identifier.
    PasswordHashKey(std::optional<text::String> identifier, mem::ByteBlock key);
    /// Test whether an identifier meets the public rotation-identifier requirements.
    [[nodiscard]] static auto isValidIdentifier(const text::String &identifier) noexcept -> bool;

private:
    std::optional<text::String> _identifier; ///< The optional public rotation identifier.
    mem::ByteBlock _key;                     ///< The marked application-key material.
};

}
