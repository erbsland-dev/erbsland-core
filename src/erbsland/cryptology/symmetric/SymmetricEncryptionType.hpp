// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SymmetricCipher.hpp"

#include "../CryptographicSecurity.hpp"

#include "../../text/String_fwd.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../util/impl/ComparisonHelper.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace erbsland::cryptology {

/// A complete symmetric encryption construction supported by the library.
/// Algorithm metadata is library policy and can change between releases. Persisted data and protocols must store
/// `toString()` for the selected type instead of relying on a future recommendation returning the same value.
/// @seedoc{/reference/cryptology/symmetric_encryption}
/// @tested{SymmetricEncryptionTypeTest}
class SymmetricEncryptionType final {
public:
    /// The raw symmetric encryption value.
    enum Value : uint8_t {
        None,                    ///< No encryption type; used for invalid placeholders.
        Aes256Gcm,               ///< AES-256 in Galois/Counter Mode.
        ChaCha20Poly1305,        ///< ChaCha20 with Poly1305 authentication.
        Aes128Gcm,               ///< AES-128 in Galois/Counter Mode.
        Aes256CbcRandomFill,     ///< Legacy AES-256-CBC with external-length random-fill padding.
        Aes256CbcIso9797Method2, ///< Legacy AES-256-CBC with ISO/IEC 9797-1 method 2 padding.
    };

public:
    /// Create an invalid encryption type.
    constexpr SymmetricEncryptionType() noexcept = default;
    /// Create an encryption type from its raw value.
    /// @param value The raw encryption type value.
    constexpr SymmetricEncryptionType(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    ~SymmetricEncryptionType() = default;
    SymmetricEncryptionType(const SymmetricEncryptionType &) = default;
    SymmetricEncryptionType(SymmetricEncryptionType &&) = default;
    auto operator=(const SymmetricEncryptionType &) -> SymmetricEncryptionType & = default;
    auto operator=(SymmetricEncryptionType &&) -> SymmetricEncryptionType & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const SymmetricEncryptionType &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(
        const Value value, const SymmetricEncryptionType &other, value, other._value);

public: // tests
    /// Test if this value identifies an encryption type.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool { return _value != None; }
    /// Test if this type provides authenticated encryption with associated data.
    [[nodiscard]] auto isAead() const noexcept -> bool;
    /// Test if this type requires an initialization vector instead of a nonce.
    [[nodiscard]] auto requiresIv() const noexcept -> bool;

public: // accessors
    /// Get the raw encryption type value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }
    /// Get the cipher family, or `None` for an invalid encryption type.
    [[nodiscard]] auto cipher() const noexcept -> SymmetricCipher;
    /// Get the required key width in bits.
    [[nodiscard]] auto keyBitCount() const noexcept -> std::size_t;
    /// Get the required key length.
    [[nodiscard]] auto keyLength() const noexcept -> unit::ByteLength;
    /// Get the required nonce length, or zero when this type does not use a nonce.
    [[nodiscard]] auto nonceLength() const noexcept -> unit::ByteLength;
    /// Get the required authentication-tag length, or zero for unauthenticated encryption.
    [[nodiscard]] auto tagLength() const noexcept -> unit::ByteLength;
    /// Get the required initialization-vector length, or zero when this type does not use one.
    [[nodiscard]] auto ivLength() const noexcept -> unit::ByteLength;
    /// Calculate an upper bound for the encrypted payload length.
    /// Authentication tags are separate and are not included. Infinite and overflowing lengths saturate.
    /// @param originalLength The original plaintext length.
    /// @return The maximum encrypted payload length for this type.
    [[nodiscard]] auto maximumEncryptedLength(unit::ByteLength originalLength) const noexcept -> unit::ByteLength;
    /// Get the current coarse security level.
    [[nodiscard]] auto security() const noexcept -> CryptographicSecurity;

public: // conversion
    /// Convert the encryption type to its stable lowercase identifier.
    [[nodiscard]] auto toString() const -> text::String;

public: // factories
    /// Parse an exact lowercase encryption type identifier.
    /// @param text The identifier to parse.
    /// @return The matching type, or no value for unsupported text.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<SymmetricEncryptionType>;
    /// Parse an exact lowercase encryption type identifier.
    /// @param text The identifier to parse.
    /// @return The matching encryption type.
    /// @throws err::ParseError If `text` is not a supported identifier.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> SymmetricEncryptionType;
    /// Get all supported encryption types in stable preference order.
    [[nodiscard]] static auto all() noexcept -> std::span<const SymmetricEncryptionType>;

private:
    Value _value{None}; ///< The raw encryption type value.
};

}
