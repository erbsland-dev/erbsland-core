// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../mem/ByteBlock.hpp"
#include "../../../../mem/SecureErase.hpp"
#include "../../../HashAlgorithm.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace erbsland::cryptology::impl::rsa_signature {

constexpr auto cWordBits = std::size_t{32U};
constexpr auto cMinimumModulusBits = std::size_t{2048U};
constexpr auto cMaximumModulusBits = std::size_t{8192U};
constexpr auto cMaximumWords = cMaximumModulusBits / cWordBits;
constexpr auto cMaximumExponentBytes = std::size_t{32U};

/// Supported RFC 8017 encoding methods.
enum class Padding : uint8_t {
    Pkcs1V15,
    Pss,
};

/// Decoded RSA signature encoding parameters.
/// @tested{RsaSignatureTest X509CertificateTest}
struct SignatureParameters final {
    Padding padding{Padding::Pkcs1V15}; ///< Selected RFC 8017 encoding method.
    HashAlgorithm hash;                 ///< Message hash and, for PSS, MGF1 hash.
    std::size_t saltLength{};           ///< Exact PSS salt length; zero for PKCS#1 v1.5.
};

/// Bounded nonnegative RSA arithmetic value.
/// @tested{RsaSignatureTest X509CertificateTest}
struct Number final {
    std::array<uint32_t, cMaximumWords> words{}; ///< Little-endian base-2^32 limbs.
    std::size_t count{};                         ///< Active fixed-width limbs.

    /// Erase all fixed storage and reset the active width.
    void secureErase() noexcept {
        mem::secureErase(std::span{words});
        count = 0U;
    }
};

/// Decoded RSA public-key values and optional restrictions.
/// @tested{RsaSignatureTest X509CertificateTest}
struct PublicKeyData final {
    Number modulus;                                  ///< RSA modulus `n`.
    mem::ByteBlock exponent;                         ///< Minimal big-endian public exponent `e`.
    std::size_t modulusBits{};                       ///< Exact bit length of `n`.
    std::size_t encodedLength{};                     ///< RFC 8017 modulus length `k` in octets.
    std::optional<SignatureParameters> restrictions; ///< Optional id-RSASSA-PSS key restrictions.
};

}
