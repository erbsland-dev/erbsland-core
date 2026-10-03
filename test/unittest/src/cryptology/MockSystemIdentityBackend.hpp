#pragma once

#include <erbsland/cryptology/impl/NativeTlsAlgorithms.hpp>
#include <erbsland/cryptology/impl/SystemIdentityBackend.hpp>
#include <erbsland/CryptologyError.hpp>
#include <erbsland/Literals.hpp>

/// Isolated identity source exposing only the explicitly supplied fixture identity.
class MockSystemIdentityBackend final : public el::cryptology::impl::SystemIdentityBackend {
public:
    /// Retain the fixture chain and provider for exact-fingerprint lookup.
    MockSystemIdentityBackend(el::cryptology::X509CertificateBundle chain, el::cryptology::impl::TlsSignerPtr signer) :
        _chain{std::move(chain)}, _signer{std::move(signer)} {}

public:
    [[nodiscard]] auto isSupported() const noexcept -> bool override { return true; }
    [[nodiscard]] auto list() const -> el::util::List<el::cryptology::SystemIdentityInfo> override {
        return el::util::List<el::cryptology::SystemIdentityInfo>{el::cryptology::SystemIdentityInfo{
            el::cryptology::impl::native_tls_algorithms::fingerprint(_chain.certificates().first().toDer()),
            _chain.certificates().first()}};
    }
    [[nodiscard]] auto load(const el::mem::ByteBlock &fingerprint) const
        -> el::cryptology::impl::SystemIdentityMaterial override {
        using namespace el::text::literals;
        if (fingerprint != list().first().fingerprint) {
            throw el::cryptology::CryptologyError{"Missing test identity."_el};
        }
        return {_chain, _signer};
    }

private:
    el::cryptology::X509CertificateBundle _chain;
    el::cryptology::impl::TlsSignerPtr _signer;
};
