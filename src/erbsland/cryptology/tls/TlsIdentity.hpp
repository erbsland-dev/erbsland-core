// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SystemIdentityStore_fwd.hpp"
#include "TlsIdentity_fwd.hpp"
#include "TlsSignatureScheme.hpp"

#include "../impl/TlsSigner_fwd.hpp"
#include "../keys/SigningPrivateKey.hpp"
#include "../x509/X509CertificateBundle.hpp"

#include "../../util/List.hpp"

#include <optional>

namespace erbsland::cryptology {

/// One immutable role-neutral TLS certificate chain and opaque software or native signer.
///
/// Matching follows RFC 8410 for Ed25519, RFC 5480 for P-256, and RFC 3279/RFC 4055 for RSA. Signature-scheme
/// selection follows RFC 8446 section 4.2.3 in the peer-provided order and never permits certificate-only PKCS#1
/// schemes. The identity owns the move-only key; configurations share it only through a pointer to const.
/// @seedoc{/reference/cryptology/tls}
/// @tested{TlsIdentityTest}
class TlsIdentity final {
    friend class SystemIdentityStore;

public:
    /// Create a validated software identity usable in either TLS role.
    /// @param certificateChain The leaf-first certificate chain. Must not be empty, and its leaf public key must match
    /// `signingKey`.
    /// @param signingKey The matching private key. Must not be empty and must support a usable TLS 1.3 signature
    /// scheme.
    /// @throws CryptologyError If the certificate and key do not match or support no TLS 1.3 signature scheme.
    TlsIdentity(X509CertificateBundle certificateChain, SigningPrivateKey signingKey);

    // defaults/deletions
    ~TlsIdentity() = default;
    TlsIdentity(const TlsIdentity &) = delete;
    TlsIdentity(TlsIdentity &&) noexcept = default;
    auto operator=(const TlsIdentity &) -> TlsIdentity & = delete;
    auto operator=(TlsIdentity &&) noexcept -> TlsIdentity & = default;

public: // selection
    /// Select the first peer-offered scheme compatible with this identity.
    /// @param offeredSchemes ClientHello or CertificateRequest `signature_algorithms` values in wire order.
    /// @return The first compatible TLS 1.3 `CertificateVerify` scheme, or no value.
    [[nodiscard]] auto selectSignatureScheme(const util::List<TlsSignatureScheme> &offeredSchemes) const noexcept
        -> std::optional<TlsSignatureScheme>;

public: // accessors
    /// Get the leaf-first certificate chain.
    [[nodiscard]] auto certificateChain() const noexcept -> const X509CertificateBundle & { return _certificateChain; }
    /// Test whether the private key remains in a native provider.
    [[nodiscard]] auto isNative() const noexcept -> bool;
    /// Sign RFC 8446 section 4.4.3 content using the owned provider; native operations may block.
    /// @param scheme A compatible TLS CertificateVerify scheme.
    /// @param message Complete CertificateVerify content.
    /// @return The verified signature, without exposing private key material.
    /// @throws CryptologyError If signing, provider access, or the signature fault check fails.
    [[nodiscard]] auto signTlsCertificateVerify(TlsSignatureScheme scheme, mem::ConstByteSpan message) const
        -> mem::ByteBlock;

private:
    /// Retain an opaque signer and its matching public certificate chain from a native store.
    TlsIdentity(X509CertificateBundle certificateChain, impl::TlsSignerPtr signer);
    /// Require at least one permitted and discovered TLS signing scheme.
    void requireUsableSigner() const;

private:
    X509CertificateBundle _certificateChain; ///< Leaf-first nonempty certificate chain.
    impl::TlsSignerPtr _signer;              ///< Provider owning the private key without a serialization path.
};

}
