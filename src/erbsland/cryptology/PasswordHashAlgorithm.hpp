// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String_fwd.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::cryptology {

/// A password hashing algorithm supported by `PasswordHasher`.
/// @seedoc{/reference/cryptology/password_hashing}
/// @tested{PasswordHasherTest}
class PasswordHashAlgorithm final {
public:
    /// The raw password-hashing algorithm value.
    enum Value : uint8_t {
        Argon2id, ///< Argon2id version 1.3.
        Scrypt,   ///< scrypt as specified in RFC 7914.
    };

public:
    /// Create the default Argon2id algorithm.
    constexpr PasswordHashAlgorithm() noexcept = default;
    /// Create an algorithm from its raw value.
    /// @param value The raw algorithm value.
    constexpr PasswordHashAlgorithm(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    ~PasswordHashAlgorithm() = default;
    PasswordHashAlgorithm(const PasswordHashAlgorithm &) = default;
    PasswordHashAlgorithm(PasswordHashAlgorithm &&) = default;
    auto operator=(const PasswordHashAlgorithm &) -> PasswordHashAlgorithm & = default;
    auto operator=(PasswordHashAlgorithm &&) -> PasswordHashAlgorithm & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const PasswordHashAlgorithm &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const PasswordHashAlgorithm &other, value, other._value);

public: // accessors
    /// Get the raw algorithm value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // conversion
    /// Convert the algorithm to its stable lowercase identifier.
    [[nodiscard]] auto toString() const -> text::String;
    /// Parse an exact lowercase algorithm identifier.
    /// @param text The identifier to parse.
    /// @return The matching algorithm, or no value for unsupported text.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<PasswordHashAlgorithm>;

private:
    Value _value{Argon2id}; ///< The raw algorithm value.
};

}
