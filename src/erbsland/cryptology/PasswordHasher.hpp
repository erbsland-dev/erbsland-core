// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PasswordHash.hpp"
#include "PasswordHasher_fwd.hpp"
#include "PasswordHashKey.hpp"
#include "PasswordHashPolicy.hpp"
#include "PasswordVerification.hpp"

#include "unsafe/UnsafeNoPasswordHashKey.hpp"

#include "../mem/ByteSpan.hpp"
#include "../text/String.hpp"
#include "../util/List_fwd.hpp"

#include <optional>
#include <vector>

namespace erbsland::cryptology {

/// The safe password hashing and verification API.
/// Normal construction requires an application key. Mark password strings as sensitive whenever practical.
/// @seedoc{/reference/cryptology/password_hashing}
/// @tested{PasswordHasherTest}
class PasswordHasher final {
public:
    /// Create a keyed password hasher.
    /// @param key The active application key.
    /// @param policy The password-hashing policy.
    explicit PasswordHasher(PasswordHashKey key, PasswordHashPolicy policy = PasswordHashPolicy::recommended());
    /// Create an explicitly unkeyed password hasher.
    /// @param acknowledgement The explicit acknowledgement of reduced protection.
    /// @param policy The password-hashing policy.
    explicit PasswordHasher(
        unsafe::UnsafeNoPasswordHashKey acknowledgement, PasswordHashPolicy policy = PasswordHashPolicy::recommended());

    // defaults/deletions
    PasswordHasher() = delete;
    PasswordHasher(const PasswordHasher &) = delete;
    PasswordHasher(PasswordHasher &&) noexcept = default;
    auto operator=(const PasswordHasher &) -> PasswordHasher & = delete;
    auto operator=(PasswordHasher &&) noexcept -> PasswordHasher & = default;

public:
    /// Hash a password with a fresh 16-byte salt from `application().secureRandom()`.
    /// Empty passwords are accepted. Inputs larger than 1 MiB are rejected.
    /// @param password The UTF-8 password. Sensitive marking is recommended but not required.
    /// @return The canonical password-hash record.
    /// @throws err::ParameterError If the password exceeds 1 MiB of UTF-8 data.
    [[nodiscard]] auto hash(const text::String &password) const -> PasswordHash;
    /// Verify a password and report an optional replacement hash.
    /// Malformed/invalid records and unavailable keys run a dummy current-policy derivation before rejection.
    /// @param password The UTF-8 password. Sensitive marking is recommended but not required.
    /// @param storedHash The stored canonical password-hash record.
    /// @return The explicit accepted or rejected result.
    [[nodiscard]] auto verify(const text::String &password, const PasswordHash &storedHash) const
        -> PasswordVerification;

public: // accessors
    /// Get the active password-hashing policy.
    [[nodiscard]] auto policy() const noexcept -> const PasswordHashPolicy & { return _policy; }

public: // factories
    /// Create a keyed hasher with fallback keys for rotation.
    /// The active key must be identified. Fallback identifiers must be unique; at most one fallback may be unnamed.
    /// @param activeKey The identified key used for new records.
    /// @param fallbackKeys The legacy keys accepted during verification.
    /// @param policy The password-hashing policy.
    /// @return The configured password hasher.
    /// @throws err::ParameterError If the rotation set is ambiguous.
    [[nodiscard]] static auto withKeyRotation(
        PasswordHashKey activeKey,
        util::List<PasswordHashKey> fallbackKeys,
        PasswordHashPolicy policy = PasswordHashPolicy::recommended()) -> PasswordHasher;

private:
    constexpr static auto cMaximumPasswordBytes = std::size_t{1024U * 1024U};

    /// Create a hasher from normalized key-rotation state.
    PasswordHasher(
        std::optional<PasswordHashKey> activeKey,
        std::vector<PasswordHashKey> fallbackKeys,
        PasswordHashPolicy policy) noexcept;
    /// Hash a password with supplied salt bytes.
    [[nodiscard]] auto hashWithSalt(const text::String &password, mem::ConstByteSpan salt) const -> PasswordHash;
    /// Derive password-hash output according to a policy.
    [[nodiscard]] static auto derive(
        const text::String &password, mem::ConstByteSpan salt, const PasswordHashPolicy &policy)
        -> mem::ByteBlockEditor;
    /// Find the configured key identified by stored hash data.
    [[nodiscard]] auto keyFor(const impl::PasswordHashData &data) const noexcept -> const PasswordHashKey *;
    /// Mirror derivation, verifier protection, and comparison for verification failures.
    void performDummyVerification(const text::String &password) const;

private:
    std::optional<PasswordHashKey> _activeKey;  ///< The active key, or no key for explicitly unkeyed operation.
    std::vector<PasswordHashKey> _fallbackKeys; ///< The legacy keys accepted during rotation.
    PasswordHashPolicy _policy;                 ///< The active hashing and migration policy.
};

}
