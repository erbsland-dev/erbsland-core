// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HashAlgorithm.hpp"
#include "Hkdf_fwd.hpp"

#include "keys/KeyAgreementSharedSecret_fwd.hpp"

#include "../mem/ByteBlock_fwd.hpp"
#include "../mem/ByteSpan.hpp"
#include "../unit/ByteLength.hpp"

namespace erbsland::cryptology {

/// The RFC 5869 HMAC-based extract-and-expand key derivation function.
/// This value stores only the selected algorithm. Every non-empty result uses sensitive storage; no input keying
/// material or pseudorandom key is retained after an operation returns.
/// `extract()` implements RFC 5869 section 2.2 and `expand()` implements section 2.3.
/// Specification: https://www.rfc-editor.org/rfc/rfc5869.html#section-2
/// @seedoc{/reference/cryptology/cryptographic_operations}
/// @tested{HkdfTest}
class Hkdf final {
public:
    /// Create HKDF for SHA-256 or SHA-384.
    /// @param algorithm The underlying HMAC hash algorithm.
    /// @throws err::ParameterError If the algorithm is unsupported.
    explicit Hkdf(HashAlgorithm algorithm);

    // defaults
    ~Hkdf() = default;
    Hkdf(const Hkdf &) = default;
    Hkdf(Hkdf &&) noexcept = default;
    auto operator=(const Hkdf &) -> Hkdf & = default;
    auto operator=(Hkdf &&) noexcept -> Hkdf & = default;

public:
    /// Extract a digest-sized pseudorandom key from input keying material.
    /// An empty salt is interpreted as a digest-sized all-zero salt as defined by RFC 5869.
    /// @param inputKeyMaterial The source keying material.
    /// @param salt The optional salt bytes.
    /// @return A digest-sized pseudorandom key in sensitive storage.
    [[nodiscard]] auto extract(mem::ConstByteSpan inputKeyMaterial, mem::ConstByteSpan salt = {}) const
        -> mem::ByteBlock;
    /// Extract a pseudorandom key from a protected key-agreement shared secret.
    /// @param sharedSecret The protected source keying material.
    /// @param salt The optional salt bytes.
    /// @return A digest-sized pseudorandom key in sensitive storage.
    /// @throws err::LogicError If `sharedSecret` is empty.
    [[nodiscard]] auto extract(const KeyAgreementSharedSecret &sharedSecret, mem::ConstByteSpan salt = {}) const
        -> mem::ByteBlock;
    /// Expand a pseudorandom key into output keying material.
    /// @param pseudoRandomKey A pseudorandom key of at least the digest size.
    /// @param info Optional application and context information.
    /// @param outputLength The requested output length, at most 255 times the digest size.
    /// @return Output keying material in sensitive storage when non-empty.
    /// @throws err::ParameterError If the key is too short or the output length exceeds the RFC limit.
    [[nodiscard]] auto expand(
        mem::ConstByteSpan pseudoRandomKey, mem::ConstByteSpan info, unit::ByteLength outputLength) const
        -> mem::ByteBlock;

public: // accessors
    /// Get the configured underlying hash algorithm.
    [[nodiscard]] auto algorithm() const noexcept -> HashAlgorithm { return _algorithm; }

private:
    HashAlgorithm _algorithm; ///< The supported underlying HMAC hash algorithm.
};

}
