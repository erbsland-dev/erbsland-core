#pragma once

#include "TlsSigner_fwd.hpp"

#include "../x509/X509CertificateBundle.hpp"

namespace erbsland::cryptology::impl {

/// Validated public chain and opaque native signer returned by an identity backend.
/// @tested{SystemIdentityStoreTest}
struct SystemIdentityMaterial {
    X509CertificateBundle chain; ///< Leaf-first public chain.
    TlsSignerPtr signer;         ///< Provider-owned non-exported private key.
};

}
