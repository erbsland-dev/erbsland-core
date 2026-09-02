// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../HashAlgorithm.hpp"
#include "../symmetric/SymmetricEncryptionType.hpp"

#include "../../text/String_fwd.hpp"
#include "../../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <optional>
#include <span>

namespace erbsland::cryptology {

/// A supported TLS 1.3 cipher suite from RFC 8446 section 9.1.
/// The suite binds one AEAD construction to the hash used by HKDF and the handshake transcript. Only the three
/// cipher suites defined by RFC 8446 are represented.
/// @seedoc{/reference/cryptology/tls}
/// @tested{TlsRecordProtectionTest}
class TlsCipherSuite final {
public:
    /// Supported RFC 8446 cipher-suite values.
    enum Value : uint16_t {
        Aes128GcmSha256 = 0x1301U,        ///< TLS_AES_128_GCM_SHA256.
        Aes256GcmSha384 = 0x1302U,        ///< TLS_AES_256_GCM_SHA384.
        ChaCha20Poly1305Sha256 = 0x1303U, ///< TLS_CHACHA20_POLY1305_SHA256.
    };

public:
    /// Create the mandatory-to-implement TLS_AES_128_GCM_SHA256 suite.
    constexpr TlsCipherSuite() noexcept = default;
    /// Create a suite from a supported raw value.
    constexpr TlsCipherSuite(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    ~TlsCipherSuite() = default;
    TlsCipherSuite(const TlsCipherSuite &) = default;
    TlsCipherSuite(TlsCipherSuite &&) = default;
    auto operator=(const TlsCipherSuite &) -> TlsCipherSuite & = default;
    auto operator=(TlsCipherSuite &&) -> TlsCipherSuite & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const TlsCipherSuite &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const TlsCipherSuite &other, value, other._value);

public: // accessors
    /// Get the exact unsigned 16-bit TLS wire value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> uint16_t { return static_cast<uint16_t>(_value); }
    /// Get the suite's HKDF and transcript hash.
    [[nodiscard]] auto hashAlgorithm() const noexcept -> HashAlgorithm;
    /// Get the suite's AEAD construction.
    [[nodiscard]] auto encryptionType() const noexcept -> SymmetricEncryptionType;

public: // conversion
    /// Convert the suite to its exact TLS registry name.
    [[nodiscard]] auto toString() const -> text::String;

public: // factories
    /// Parse one supported unsigned 16-bit TLS wire value.
    [[nodiscard]] static auto fromRawValue(uint16_t value) noexcept -> std::optional<TlsCipherSuite>;
    /// Parse one supported unsigned 16-bit TLS wire value.
    /// @throws err::ParseError If the value is not supported.
    [[nodiscard]] static auto fromRawValueOrThrow(uint16_t value) -> TlsCipherSuite;
    /// Get all supported TLS 1.3 cipher suites in preference order.
    [[nodiscard]] static auto all() noexcept -> std::span<const TlsCipherSuite>;

private:
    Value _value{Aes128GcmSha256}; ///< Supported TLS cipher-suite code point.
};

}
