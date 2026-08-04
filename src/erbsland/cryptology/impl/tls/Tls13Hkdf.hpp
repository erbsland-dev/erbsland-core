// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../../unit/ByteLength.hpp"
#include "../../HashAlgorithm.hpp"

namespace erbsland::cryptology::impl {

/// Internal RFC 8446 Section 7.1 TLS 1.3 HKDF label operations.
/// This type centralizes the exact `HkdfLabel` wire representation so traffic protection and the handshake key
/// schedule use one auditable implementation.
/// Specification: https://www.rfc-editor.org/rfc/rfc8446.html#section-7.1
/// @tested{Tls13KeyScheduleTest TlsRecordProtectionTest}
class Tls13Hkdf final {
public:
    /// Bind the TLS label operations to one cipher-suite hash.
    /// @param algorithm The SHA-256 or SHA-384 HKDF algorithm.
    explicit Tls13Hkdf(HashAlgorithm algorithm);

    // defaults
    ~Tls13Hkdf() = default;
    Tls13Hkdf(const Tls13Hkdf &) = default;
    Tls13Hkdf(Tls13Hkdf &&) noexcept = default;
    auto operator=(const Tls13Hkdf &) -> Tls13Hkdf & = default;
    auto operator=(Tls13Hkdf &&) noexcept -> Tls13Hkdf & = default;

public:
    /// Expand a TLS 1.3 secret with an exact label and context.
    /// @param algorithm The SHA-256 or SHA-384 HKDF algorithm.
    /// @param secret The digest-sized HKDF pseudorandom key.
    /// @param label The label without the mandatory `"tls13 "` prefix.
    /// @param context The context encoded in `HkdfLabel`.
    /// @param outputLength The requested output length.
    /// @return Sensitive output keying material.
    /// @throws err::ParameterError If a one-byte label or context bound, or the two-byte output bound, is exceeded.
    [[nodiscard]] auto expandLabel(
        mem::ConstByteSpan secret,
        mem::ConstByteSpan label,
        mem::ConstByteSpan context,
        unit::ByteLength outputLength) const -> mem::ByteBlock;
    /// Derive a digest-sized secret using a transcript hash as the context.
    /// @param algorithm The SHA-256 or SHA-384 HKDF algorithm.
    /// @param secret The digest-sized HKDF pseudorandom key.
    /// @param label The label without the mandatory `"tls13 "` prefix.
    /// @param transcriptHash The exact digest-sized transcript hash.
    /// @return The derived secret in sensitive storage.
    /// @throws err::ParameterError If the transcript hash has the wrong length.
    [[nodiscard]] auto deriveSecret(
        mem::ConstByteSpan secret, mem::ConstByteSpan label, mem::ConstByteSpan transcriptHash) const -> mem::ByteBlock;
    /// Calculate Hash("") for the selected transcript hash.
    /// @param algorithm The SHA-256 or SHA-384 transcript hash.
    /// @return The digest of the empty string.
    [[nodiscard]] auto emptyHash() const -> mem::ByteBlock;

private:
    HashAlgorithm _algorithm; ///< Bound SHA-256 or SHA-384 HKDF algorithm.
};

}
