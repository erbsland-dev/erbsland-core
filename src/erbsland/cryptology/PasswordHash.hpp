// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PasswordHashAlgorithm.hpp"
#include "PasswordHasher_fwd.hpp"

#include "impl/PasswordHashData_fwd.hpp"

#include "../text/String.hpp"

#include <optional>

namespace erbsland::cryptology {

/// An immutable canonical password-hash record suitable for database or configuration storage.
/// The default value is an invalid sentinel and is safe to use for malformed records and unknown users.
/// @seedoc{/reference/cryptology/password_hashing}
/// @tested{PasswordHasherTest}
class PasswordHash final {
    friend class PasswordHasher;

public:
    // defaults
    PasswordHash() = default;
    ~PasswordHash() = default;
    PasswordHash(const PasswordHash &) = default;
    PasswordHash(PasswordHash &&) noexcept = default;
    auto operator=(const PasswordHash &) -> PasswordHash & = default;
    auto operator=(PasswordHash &&) noexcept -> PasswordHash & = default;

public: // tests
    /// Test whether this value contains a valid parsed record.
    [[nodiscard]] auto isValid() const noexcept -> bool { return _data != nullptr; }
    /// Test whether the record uses an application key.
    [[nodiscard]] auto isKeyed() const noexcept -> bool;

public: // accessors
    /// Get the stored algorithm.
    /// @throws err::LogicError If this is the invalid sentinel.
    [[nodiscard]] auto algorithm() const -> PasswordHashAlgorithm;
    /// Get the public key identifier, if the keyed record has one.
    [[nodiscard]] auto keyIdentifier() const noexcept -> std::optional<text::String>;

public: // conversion
    /// Return the canonical storage representation, or an empty string for the invalid sentinel.
    [[nodiscard]] auto toString() const -> text::String;
    /// Parse a strict canonical record, returning the invalid sentinel on any error.
    /// @param text The storage record.
    /// @return The parsed record, or the invalid sentinel.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> PasswordHash;
    /// Parse a strict canonical record.
    /// @param text The storage record.
    /// @return The parsed record.
    /// @throws err::ParseError If the record is malformed, noncanonical, unsupported, or exceeds resource limits.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> PasswordHash;

private:
    /// Create a password hash from immutable record data.
    /// @param data The immutable record data to retain.
    explicit PasswordHash(impl::PasswordHashDataPtr data) noexcept;

private:
    impl::PasswordHashDataPtr _data; ///< The immutable parsed record, or null for an invalid value.
};

}
