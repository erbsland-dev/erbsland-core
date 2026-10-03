#pragma once

#include "../HashAlgorithm.hpp"
#include "../keys/PublicKey.hpp"
#include "../tls/TlsSignatureScheme.hpp"
#include "../x509/X509Certificate_fwd.hpp"

#include "../../util/List.hpp"

namespace erbsland::cryptology::impl::native_tls_algorithms {

/// Get modern TLS CertificateVerify schemes considered for native providers.
/// @tested{TlsIdentityTest}
[[nodiscard]] auto schemes() -> util::List<TlsSignatureScheme>;
/// Check the public key's curve, strength and PSS constraints before provider capability discovery.
/// @tested{TlsIdentityTest}
[[nodiscard]] auto matches(const PublicKey &key, TlsSignatureScheme scheme) noexcept -> bool;
/// Get the SHA-2 digest for a supported native signing scheme.
/// @tested{TlsIdentityTest}
[[nodiscard]] auto hash(TlsSignatureScheme scheme) -> HashAlgorithm;
/// Compute the exact SHA-256 certificate selector.
/// @tested{SystemIdentityStoreTest}
[[nodiscard]] auto fingerprint(const mem::ByteBlock &der) -> mem::ByteBlock;
/// Verify a self-signature before omitting the final root from a native TLS identity chain (RFC 8446 section 4.4.2).
/// @tested{SystemIdentityStoreTest}
[[nodiscard]] auto isSelfSigned(const X509Certificate &certificate) noexcept -> bool;
/// Normalize Windows' fixed-width ECDSA integers into RFC 3279 DER.
/// @tested{SystemIdentityStoreTest}
[[nodiscard]] auto ecdsaDer(mem::ConstByteSpan signature) -> mem::ByteBlock;

}
