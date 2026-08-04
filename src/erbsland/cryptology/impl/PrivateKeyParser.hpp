// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PrivateKeyParser_fwd.hpp"

#include "../keys/SigningPrivateKey.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../text/String.hpp"

namespace erbsland::cryptology::impl {

/// Strict bounded unencrypted PKCS#8 private-key parser.
///
/// The outer structure follows RFC 5208 section 6. Ed25519 private-key encoding follows RFC 8410 section 7. ECDSA
/// and RSA algorithm-specific structures are accepted only in the narrow profiles documented by SigningPrivateKey.
/// @tested{SigningPrivateKeyTest}
class PrivateKeyParser final {
public:
    /// Bind a parser to one bounded DER source.
    explicit PrivateKeyParser(const mem::ByteBlock &der) noexcept : _der{der} {}

    /// Parse one complete canonical DER `PrivateKeyInfo` value.
    [[nodiscard]] auto parse() const -> SigningPrivateKey;
    /// Decode one exact RFC 7468-style `PRIVATE KEY` PEM block.
    [[nodiscard]] static auto decodePem(const text::String &pem) -> mem::ByteBlock;

private:
    /// Parse and validate the Ed25519 algorithm-specific key.
    [[nodiscard]] auto parseEd25519(const Asn1Node &algorithm, const mem::ByteBlock &privateKey) const
        -> SigningPrivateKey;
    /// Parse and validate the P-256 algorithm-specific key.
    [[nodiscard]] auto parseEcdsaP256(const Asn1Node &algorithm, const mem::ByteBlock &privateKey) const
        -> SigningPrivateKey;
    /// Parse and validate one two-prime RSA key with RSAE or supported PSS parameters.
    [[nodiscard]] auto parseRsa(const Asn1Node &algorithm, const mem::ByteBlock &privateKey) const -> SigningPrivateKey;
    /// Require one exact ASN.1 node shape.
    static void requireNode(
        const Asn1Node &node, Asn1TagClass tagClass, uint32_t tagNumber, bool constructed, const text::String &name);
    /// Decode one small nonnegative canonical INTEGER.
    [[nodiscard]] static auto decodeSmallInteger(const Asn1Node &node, const text::String &name) -> uint32_t;
    /// Throw one stable private-key parse error.
    [[noreturn]] static void throwParseError(const text::String &reason);

private:
    const mem::ByteBlock &_der; ///< Complete bounded DER source retained for the parse lifetime.
};

}
