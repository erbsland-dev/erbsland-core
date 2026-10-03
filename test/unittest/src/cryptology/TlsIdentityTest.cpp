// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/CryptologyConfiguration.hpp>
#include <erbsland/CryptologyError.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/ProtectedDataMode.hpp>
#include <erbsland/SigningPrivateKey.hpp>
#include <erbsland/TlsConfiguration.hpp>
#include <erbsland/TlsIdentity.hpp>
#include <erbsland/X509CertificateBundle.hpp>

#include <type_traits>

using namespace el::cryptology;

TESTED_TARGETS(TlsIdentity TlsConfiguration)
class TlsIdentityTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
private:
    [[nodiscard]] static auto readText(const std::string_view path) -> el::String {
        return el::String{el::unittest::fh::readDataText(path)};
    }

public:
    static_assert(!std::is_copy_constructible_v<TlsIdentity>);
    static_assert(!std::is_copy_assignable_v<TlsIdentity>);
    static_assert(std::is_move_constructible_v<TlsIdentity>);
    static_assert(std::is_copy_constructible_v<TlsConfiguration>);

    void testRsaIdentitySelectionAndConfigurationLifetime() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        auto identity = TlsIdentity{
            X509CertificateBundle::fromPemOrThrow(readText("data/network/tls-interop/server.pem")),
            SigningPrivateKey::fromPemOrThrow(readText("data/network/tls-interop/server-key.pem"))};
        const auto offered = el::List<TlsSignatureScheme>{
            TlsSignatureScheme::RsaPkcs1Sha256,
            TlsSignatureScheme::RsaPssPssSha256,
            TlsSignatureScheme::RsaPssRsaeSha384,
            TlsSignatureScheme::RsaPssRsaeSha256};
        REQUIRE_EQUAL(identity.selectSignatureScheme(offered), TlsSignatureScheme::RsaPssRsaeSha384);

        auto configuration = TlsConfiguration{};
        configuration.setServer(el::cryptology::TlsServerConfiguration{std::move(identity)});
        REQUIRE(configuration.server().has_value());
        auto snapshot = configuration;
        REQUIRE(snapshot.server()->identity() == configuration.server()->identity());
        configuration.clearRole(el::cryptology::TlsRole::Server);
        REQUIRE_FALSE(configuration.server().has_value());
        REQUIRE(snapshot.server().has_value());
        REQUIRE_EQUAL(snapshot.server()->identity()->certificateChain().certificates().count(), el::ItemCount{1U});
        REQUIRE_FALSE(snapshot.server()->identity()->isNative());
    }

    void testRejectsEmptyAndMismatchedValues() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto chain = X509CertificateBundle::fromPemOrThrow(readText("data/network/tls-interop/server.pem"));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, TlsIdentity{X509CertificateBundle{}, SigningPrivateKey{}});
        REQUIRE_THROWS_AS(
            CryptologyError,
            TlsIdentity{
                chain, SigningPrivateKey::fromPemOrThrow(readText("data/cryptology/signing/rsa-2048-pkcs8.pem"))});
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testRoleNeutralClientIdentitySignsWithOwnedProvider() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        auto configuration = TlsConfiguration{};
        configuration.setClient(
            el::cryptology::TlsClientConfiguration{TlsIdentity{
                X509CertificateBundle::fromPemOrThrow(readText("data/network/tls-interop/server.pem")),
                SigningPrivateKey::fromPemOrThrow(readText("data/network/tls-interop/server-key.pem"))}});
        REQUIRE(configuration.client().has_value());
        const auto retained = configuration.client()->identity();
        configuration.clearRole(el::cryptology::TlsRole::Client);
        REQUIRE_FALSE(configuration.client().has_value());
        const auto scheme = TlsSignatureScheme{TlsSignatureScheme::RsaPssRsaeSha512};
        const auto signature = retained->signTlsCertificateVerify(scheme, {});
        REQUIRE(retained->certificateChain().certificates().first().publicKey().verifyTlsCertificateVerifySignature(
            scheme, {}, signature.span()));
    }
};
