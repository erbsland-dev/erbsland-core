#pragma once

#include <erbsland/cryptology/impl/TlsSigner.hpp>
#include <erbsland/SigningPrivateKey.hpp>

/// Model an opaque native provider with one supported scheme and injectable malformed output.
class MockNativeTlsSigner final : public el::cryptology::impl::TlsSigner {
public:
    /// Retain software material solely as the mock provider's private implementation.
    explicit MockNativeTlsSigner(el::cryptology::SigningPrivateKey key) : _key{std::move(key)} {}

public:
    [[nodiscard]] auto supports(el::cryptology::TlsSignatureScheme scheme) const noexcept -> bool override {
        return scheme == el::cryptology::TlsSignatureScheme::RsaPssRsaeSha512;
    }
    [[nodiscard]] auto sign(el::cryptology::TlsSignatureScheme scheme, el::mem::ConstByteSpan message) const
        -> el::mem::ByteBlock override {
        if (fail) {
            return el::mem::ByteBlock{el::unit::ByteCount{256U}};
        }
        return _key.signTlsCertificateVerify(scheme, message);
    }
    [[nodiscard]] auto isNative() const noexcept -> bool override { return true; }

public:
    bool fail{};

private:
    el::cryptology::SigningPrivateKey _key;
};
