// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CryptographicSecurity.hpp"

#include "../../text/String_fwd.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <optional>
#include <span>

namespace erbsland::cryptology {

/// A supported asymmetric key-agreement algorithm.
/// @seedoc{/reference/cryptology/key_management}
/// @tested{KeyAgreementTest}
class KeyAgreementAlgorithm final {
public:
    /// Raw key-agreement algorithm value.
    enum Value : uint8_t {
        X25519, ///< X25519 over Curve25519.
    };

public:
    /// Create the default X25519 algorithm.
    constexpr KeyAgreementAlgorithm() noexcept = default;
    /// Create an algorithm from its raw value.
    constexpr KeyAgreementAlgorithm(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    ~KeyAgreementAlgorithm() = default;
    KeyAgreementAlgorithm(const KeyAgreementAlgorithm &) = default;
    KeyAgreementAlgorithm(KeyAgreementAlgorithm &&) = default;
    auto operator=(const KeyAgreementAlgorithm &) -> KeyAgreementAlgorithm & = default;
    auto operator=(KeyAgreementAlgorithm &&) -> KeyAgreementAlgorithm & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const KeyAgreementAlgorithm &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const KeyAgreementAlgorithm &other, value, other._value);

public: // accessors
    /// Get the raw algorithm value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }
    /// Get the encoded public-key length.
    [[nodiscard]] auto publicKeySize() const noexcept -> unit::ByteLength;
    /// Get the encoded private-key length.
    [[nodiscard]] auto privateKeySize() const noexcept -> unit::ByteLength;
    /// Get the shared-secret length.
    [[nodiscard]] auto sharedSecretSize() const noexcept -> unit::ByteLength;
    /// Get the coarse security level.
    [[nodiscard]] auto security() const noexcept -> CryptographicSecurity;

public: // conversion
    /// Convert the algorithm to its stable lowercase identifier.
    [[nodiscard]] auto toString() const -> text::String;

public: // factories
    /// Parse an exact lowercase algorithm identifier.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<KeyAgreementAlgorithm>;
    /// Parse an exact lowercase algorithm identifier or throw.
    /// @throws err::ParseError If the identifier is unsupported.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> KeyAgreementAlgorithm;
    /// Get all supported key-agreement algorithms.
    [[nodiscard]] static auto all() noexcept -> std::span<const KeyAgreementAlgorithm>;

private:
    Value _value{X25519}; ///< Raw algorithm value.
};

}
