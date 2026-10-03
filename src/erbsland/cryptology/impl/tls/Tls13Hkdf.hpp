// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../../unit/ByteCount.hpp"
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
    /// @param secret The digest-sized HKDF pseudorandom key.
    /// @param label The label without the mandatory `"tls13 "` prefix. Must contain at most 249 bytes.
    /// @param context The context encoded in `HkdfLabel`. Must contain at most 255 bytes.
    /// @param outputLength The requested output length. Must fit in 16 bits and not exceed 255 times the digest size.
    /// @return Sensitive output keying material.
    [[nodiscard]] auto expandLabel(
        mem::ConstByteSpan secret,
        mem::ConstByteSpan label,
        mem::ConstByteSpan context,
        unit::ByteCount outputLength) const -> mem::ByteBlock;
    /// Derive a digest-sized secret using a transcript hash as the context.
    /// @param secret The digest-sized HKDF pseudorandom key.
    /// @param label The label without the mandatory `"tls13 "` prefix.
    /// @param transcriptHash The exact digest-sized transcript hash.
    /// @return The derived secret in sensitive storage.
    [[nodiscard]] auto deriveSecret(
        mem::ConstByteSpan secret, mem::ConstByteSpan label, mem::ConstByteSpan transcriptHash) const -> mem::ByteBlock;
    /// Calculate Hash("") for the selected transcript hash.
    /// @return The digest of the empty string.
    [[nodiscard]] auto emptyHash() const -> mem::ByteBlock;

private:
    HashAlgorithm _algorithm; ///< Bound SHA-256 or SHA-384 HKDF algorithm.
};

}
