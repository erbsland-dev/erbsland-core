#pragma once

#include "../../../mem/ByteBlock.hpp"
#include "../../../mem/ByteSpan.hpp"

/// P-256 ECDH using SEC 1 sections 3.2.1, 3.2.2.1 and 3.3.1, and RFC 8446 section 7.4.2.
/// Scalars are 32-byte big-endian integers, public keys are uncompressed 65-byte SEC 1 points, and secrets are
/// fixed-width 32-byte affine x coordinates. All private scratch is erased before returning protected-owner input.
namespace erbsland::cryptology::impl::p256_key_agreement {

/// Validate a private scalar with a fixed byte schedule.
/// @param data Candidate 32-byte scalar.
/// @return Whether the scalar is in [1,n-1].
/// @tested{P256KeyAgreementTest}
[[nodiscard]] auto isValidPrivateKey(mem::ConstByteSpan data) noexcept -> bool;
/// Validate an uncompressed, nonidentity P-256 point.
/// @param data Candidate SEC 1 point.
/// @return Whether the point is an allowed TLS P-256 public key.
/// @tested{P256KeyAgreementTest}
[[nodiscard]] auto isValidPublicKey(mem::ConstByteSpan data) noexcept -> bool;

/// Derive the uncompressed public key Q = [d]G.
/// @param privateKey Validated or candidate private scalar.
/// @return Public SEC 1 encoding.
/// @throws CryptologyError If the private scalar is invalid.
/// @tested{P256KeyAgreementTest}
[[nodiscard]] auto publicKey(mem::ConstByteSpan privateKey) -> mem::ByteBlock;
/// Derive the fixed-width shared x coordinate Z = x([d]Q).
/// @param privateKey Private scalar.
/// @param peerPublicKey Uncompressed peer point.
/// @return Sensitive owning bytes; the caller must protect and erase them.
/// @throws CryptologyError If either key is invalid or the result is the identity.
/// @tested{P256KeyAgreementTest}
[[nodiscard]] auto agree(mem::ConstByteSpan privateKey, mem::ConstByteSpan peerPublicKey) -> mem::ByteBlock;

}
