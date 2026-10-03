#pragma once

#include "TlsSigner.hpp"

#include "../keys/SigningPrivateKey.hpp"

namespace erbsland::cryptology::impl {

/// Protected-memory software implementation of the internal signer contract.
/// @tested{TlsIdentityTest}
class SoftwareTlsSigner final : public TlsSigner {
public:
    /// Transfer ownership of a validated software private key.
    explicit SoftwareTlsSigner(SigningPrivateKey key) : _key{std::move(key)} {}

public: // implement TlsSigner
    [[nodiscard]] auto supports(TlsSignatureScheme scheme) const noexcept -> bool override {
        return _key.supports(scheme);
    }
    [[nodiscard]] auto sign(TlsSignatureScheme scheme, mem::ConstByteSpan message) const -> mem::ByteBlock override {
        return _key.signTlsCertificateVerify(scheme, message);
    }
    [[nodiscard]] auto isNative() const noexcept -> bool override { return false; }

private:
    SigningPrivateKey _key; ///< Sole move-only ownership of protected private material.
};

}
