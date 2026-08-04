// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PasswordHash.hpp"
#include "PasswordHasher_fwd.hpp"

#include <optional>

namespace erbsland::cryptology {

/// The explicit result of password verification.
/// This type deliberately has no boolean conversion, so callers must name the accepted/rejected state. An accepted
/// result can carry a replacement hash that must be persisted after a successful migration.
/// @seedoc{/reference/cryptology/password_hashing}
/// @tested{PasswordHasherTest}
class PasswordVerification final {
    friend class PasswordHasher;

public: // tests
    /// Test whether the password was accepted.
    [[nodiscard]] auto isAccepted() const noexcept -> bool { return _accepted; }
    /// Test whether the password was rejected.
    [[nodiscard]] auto isRejected() const noexcept -> bool { return !_accepted; }

public: // accessors
    /// Get the replacement hash required after an accepted migration.
    [[nodiscard]] auto replacementHash() const noexcept -> const std::optional<PasswordHash> & { return _replacement; }

private:
    /// Create a verification result with an optional replacement hash.
    explicit PasswordVerification(bool accepted, std::optional<PasswordHash> replacement = {}) noexcept;

private:
    bool _accepted{};                         ///< Whether the password was accepted.
    std::optional<PasswordHash> _replacement; ///< The optional replacement record after migration.
};

}
