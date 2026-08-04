// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsServerIdentity_fwd.hpp"
#include "TlsSignatureScheme.hpp"

#include "../keys/SigningPrivateKey.hpp"
#include "../x509/X509CertificateBundle.hpp"

#include "../../util/List.hpp"

#include <optional>

namespace erbsland::cryptology {

/// One immutable-after-construction TLS server certificate chain and protected signing key.
///
/// Matching follows RFC 8410 for Ed25519, RFC 5480 for P-256, and RFC 3279/RFC 4055 for RSA. Signature-scheme
/// selection follows RFC 8446 section 4.2.3 in the peer-provided order and never permits certificate-only PKCS#1
/// schemes. The identity owns the move-only key; configurations share it only through a pointer to const.
/// @seedoc{/reference/cryptology/signing_keys}
/// @tested{TlsServerIdentityTest}
class TlsServerIdentity final {
public:
    /// Create a validated server identity.
    /// @param certificateChain Leaf-first ordered certificate chain.
    /// @param signingKey Private key matching the leaf public key.
    /// @throws err::ParameterError If either value is empty, the leaf and key differ, or no TLS 1.3 scheme is usable.
    TlsServerIdentity(X509CertificateBundle certificateChain, SigningPrivateKey signingKey);

    // defaults/deletions
    ~TlsServerIdentity() = default;
    TlsServerIdentity(const TlsServerIdentity &) = delete;
    TlsServerIdentity(TlsServerIdentity &&) noexcept = default;
    auto operator=(const TlsServerIdentity &) -> TlsServerIdentity & = delete;
    auto operator=(TlsServerIdentity &&) noexcept -> TlsServerIdentity & = default;

public: // selection
    /// Select the first peer-offered scheme compatible with this identity.
    /// @param offeredSchemes ClientHello `signature_algorithms` values in wire order.
    /// @return The first compatible TLS 1.3 `CertificateVerify` scheme, or no value.
    [[nodiscard]] auto selectSignatureScheme(const util::List<TlsSignatureScheme> &offeredSchemes) const noexcept
        -> std::optional<TlsSignatureScheme>;

public: // accessors
    /// Get the leaf-first certificate chain.
    [[nodiscard]] auto certificateChain() const noexcept -> const X509CertificateBundle & { return _certificateChain; }
    /// Get the protected signing key.
    [[nodiscard]] auto signingKey() const noexcept -> const SigningPrivateKey & { return _signingKey; }

private:
    X509CertificateBundle _certificateChain; ///< Leaf-first nonempty certificate chain.
    SigningPrivateKey _signingKey;           ///< Matching protected private key.
};

}
