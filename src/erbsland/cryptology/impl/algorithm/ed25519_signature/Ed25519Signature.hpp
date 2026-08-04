// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Ed25519SignatureTypes.hpp"

#include "../../../../mem/ByteArray.hpp"
#include "../../../../mem/ByteSpan.hpp"
#include "../../../keys/PublicKey.hpp"
#include "../../../x509/X509AlgorithmIdentifier.hpp"

#include <optional>

namespace erbsland::cryptology::impl::ed25519_signature {

/// Test whether an AlgorithmIdentifier selects Ed25519.
[[nodiscard]] auto isSignatureAlgorithm(const X509AlgorithmIdentifier &algorithm) noexcept -> bool;
/// Verify one pure Ed25519 X.509 signature.
/// Verification follows RFC 8032 sections 5.1.2--5.1.4 and 5.1.7, with X.509 encoding from RFC 8410. All arithmetic
/// operands are public verification data, and points outside the prime-order subgroup are rejected.
/// @param publicKey The exact RFC 8410 X.509 SubjectPublicKeyInfo.
/// @param signatureAlgorithm The complete id-Ed25519 signature AlgorithmIdentifier.
/// @param message The exact signed message, without prehashing or a context.
/// @param signature The raw 64-octet `ENC(R) || ENC(S)` signature.
/// @return `true` only for a valid signature.
/// @throws err::ParseError If an encoding, algorithm, or fixed representation bound is invalid.
[[nodiscard]] auto verify(
    const PublicKey &publicKey,
    const X509AlgorithmIdentifier &signatureAlgorithm,
    mem::ConstByteSpan message,
    mem::ConstByteSpan signature) -> bool;
/// Decode and validate one canonical RFC 8032 section 5.1.3 point.
[[nodiscard]] auto decodePoint(mem::ConstByteSpan bytes) noexcept -> std::optional<Point>;
/// Create the Ed25519 base point B.
[[nodiscard]] auto basePoint() noexcept -> Point;
/// Create the neutral point `(0,1)`.
[[nodiscard]] auto identity() noexcept -> Point;
/// Add two points using RFC 8032 section 5.1.4 complete formulas.
[[nodiscard]] auto add(const Point &first, const Point &second) noexcept -> Point;
/// Double one point using RFC 8032 section 5.1.4 complete formulas.
[[nodiscard]] auto doublePoint(const Point &point) noexcept -> Point;
/// Multiply one public point by one public scalar.
[[nodiscard]] auto multiply(const Point &point, mem::ConstByteSpan scalar) noexcept -> Point;
/// Test two projective points for equality without inversion.
[[nodiscard]] auto equal(const Point &first, const Point &second) noexcept -> bool;
/// Test whether a point is in the nonzero prime-order subgroup.
[[nodiscard]] auto isPrimeOrderPoint(const Point &point) noexcept -> bool;
/// Decode canonical S, returning no scalar when S is not below L.
[[nodiscard]] auto decodeScalar(mem::ConstByteSpan bytes) noexcept -> std::optional<mem::ByteArray<32U>>;
/// Reduce a 64-octet little-endian SHA-512 digest modulo L.
[[nodiscard]] auto reduceScalar(mem::ConstByteSpan bytes) noexcept -> mem::ByteArray<32U>;
/// Compare two little-endian fixed scalar values.
[[nodiscard]] auto compareScalar(const Scalar &a, const Scalar &b) noexcept -> int;
/// Subtract one scalar value from another.
void subtractScalar(Scalar &a, const Scalar &b) noexcept;
/// Return the RFC 8032 subgroup order L as little-endian limbs.
[[nodiscard]] constexpr auto subgroupOrder() noexcept -> Scalar {
    return {0x5cf5d3edU, 0x5812631aU, 0xa2f79cd6U, 0x14def9deU, 0U, 0U, 0U, 0x10000000U};
}
/// Return the RFC 8032 subgroup order L as little-endian octets.
[[nodiscard]] auto subgroupOrderBytes() noexcept -> mem::ByteArray<32U>;
/// Require absent RFC 8410 AlgorithmIdentifier parameters.
void requireEd25519Algorithm(const X509AlgorithmIdentifier &algorithm, const text::String &use);
/// Throw a parse failure with a stable Ed25519 context.
[[noreturn]] void throwParseError(const text::String &reason);

}
