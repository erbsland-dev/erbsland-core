#pragma once

#include "TlsSigner_fwd.hpp"

#include "../tls/TlsSignatureScheme.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteSpan.hpp"

namespace erbsland::cryptology::impl {

/// Internal signing contract; private keys remain owned by the selected provider.
/// @tested{TlsIdentityTest}
class TlsSigner {
public:
    // defaults
    virtual ~TlsSigner() = default;

public: // operations
    /// Test whether this provider supports a TLS CertificateVerify scheme.
    [[nodiscard]] virtual auto supports(TlsSignatureScheme scheme) const noexcept -> bool = 0;
    /// Sign the exact RFC 8446 section 4.4.3 content; may block in a native provider.
    /// @param scheme A previously discovered supported scheme.
    /// @param message Complete CertificateVerify content.
    /// @return The TLS signature representation.
    /// @throws CryptologyError If access, interaction policy, or signing fails.
    [[nodiscard]] virtual auto sign(TlsSignatureScheme scheme, mem::ConstByteSpan message) const -> mem::ByteBlock = 0;
    /// Test whether this provider retains the key in an operating-system vault.
    [[nodiscard]] virtual auto isNative() const noexcept -> bool = 0;
};

}
