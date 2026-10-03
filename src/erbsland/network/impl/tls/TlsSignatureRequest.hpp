#pragma once

#include "../../../cryptology/tls/TlsIdentity.hpp"

namespace erbsland::network::impl {

/// Immutable signing work emitted by a deterministic protocol and executed outside its owning event loop.
/// @tested{TlsServerProtocolTest TlsClientProtocolTest}
struct TlsSignatureRequest {
    cryptology::TlsIdentityConstPtr identity; ///< Explicitly configured immutable identity.
    cryptology::TlsSignatureScheme scheme;    ///< Discovered supported signing scheme.
    mem::ByteBlock content;                   ///< Exact public RFC 8446 section 4.4.3 signed content.

    /// Execute signing synchronously in the caller's admitted blocking-operation domain.
    /// @return The verified TLS signature representation.
    /// @throws cryptology::CryptologyError If signing or provider access fails.
    [[nodiscard]] auto execute() const -> mem::ByteBlock {
        return identity->signTlsCertificateVerify(scheme, content.span());
    }
};

}
