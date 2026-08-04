// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/configuration/CryptologyConfiguration.hpp>
#include <erbsland/cryptology/keys/SigningPrivateKey.hpp>
#include <erbsland/cryptology/protected_data/ProtectedDataMode.hpp>
#include <erbsland/cryptology/tls/TlsConfiguration.hpp>
#include <erbsland/cryptology/tls/TlsServerIdentity.hpp>
#include <erbsland/cryptology/x509/X509CertificateBundle.hpp>
#include <erbsland/err/ParameterError.hpp>

#include <type_traits>

using namespace el::cryptology;

TESTED_TARGETS(TlsServerIdentity TlsConfiguration)
class TlsServerIdentityTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
private:
    [[nodiscard]] static auto readText(const std::string_view path) -> el::String {
        return el::String{el::unittest::fh::readDataText(path)};
    }

public:
    static_assert(!std::is_copy_constructible_v<TlsServerIdentity>);
    static_assert(!std::is_copy_assignable_v<TlsServerIdentity>);
    static_assert(std::is_move_constructible_v<TlsServerIdentity>);
    static_assert(std::is_copy_constructible_v<TlsConfiguration>);

    void testRsaIdentitySelectionAndConfigurationLifetime() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        auto identity = TlsServerIdentity{
            X509CertificateBundle::fromPemOrThrow(readText("data/network/tls-interop/server.pem")),
            SigningPrivateKey::fromPemOrThrow(readText("data/network/tls-interop/server-key.pem"))};
        const auto offered = el::List<TlsSignatureScheme>{
            TlsSignatureScheme::RsaPkcs1Sha256,
            TlsSignatureScheme::RsaPssPssSha256,
            TlsSignatureScheme::RsaPssRsaeSha384,
            TlsSignatureScheme::RsaPssRsaeSha256};
        REQUIRE_EQUAL(identity.selectSignatureScheme(offered), TlsSignatureScheme::RsaPssRsaeSha384);

        auto configuration = TlsConfiguration{};
        configuration.setServerIdentity(std::move(identity));
        REQUIRE(configuration.hasServerIdentity());
        auto snapshot = configuration;
        REQUIRE(snapshot.serverIdentity() == configuration.serverIdentity());
        configuration.clearServerIdentity();
        REQUIRE_FALSE(configuration.hasServerIdentity());
        REQUIRE(snapshot.hasServerIdentity());
        REQUIRE_EQUAL(snapshot.serverIdentity()->certificateChain().certificates().count(), el::ItemCount{1U});
    }

    void testRejectsEmptyAndMismatchedValues() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto chain = X509CertificateBundle::fromPemOrThrow(readText("data/network/tls-interop/server.pem"));
        REQUIRE_THROWS_AS(el::err::ParameterError, TlsServerIdentity{X509CertificateBundle{}, SigningPrivateKey{}});
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            TlsServerIdentity{
                chain, SigningPrivateKey::fromPemOrThrow(readText("data/cryptology/signing/rsa-2048-pkcs8.pem"))});
    }
};
