// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/configuration/CryptologyConfiguration.hpp>
#include <erbsland/cryptology/impl/DerParser.hpp>
#include <erbsland/cryptology/keys/SigningPrivateKey.hpp>
#include <erbsland/cryptology/protected_data/ProtectedDataMode.hpp>
#include <erbsland/cryptology/x509/X509CertificateBuilder.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/network/IpAddress.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathError.hpp>
#include <erbsland/path/PathOperations.hpp>
#include <erbsland/path/TempDirectory.hpp>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(SigningKeyProfile X509CertificateBuilder X509CertificateProfile X509CertificateSigningRequest)
class X509CertificateBuilderTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    void testEcKeyGenerationAndSerialization() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        for (const auto profile : {SigningKeyProfile::EcdsaP256, SigningKeyProfile::EcdsaP384}) {
            const auto key = SigningPrivateKey::generate(profile);
            REQUIRE_FALSE(key.isEmpty());
            const auto expectedAlgorithm = profile == SigningKeyProfile::EcdsaP256 ? SigningKeyAlgorithm::EcdsaP256
                                                                                   : SigningKeyAlgorithm::EcdsaP384;
            REQUIRE(key.algorithm() == expectedAlgorithm);
            REQUIRE(SigningPrivateKey::fromDerOrThrow(key.toDer()).matches(key.publicKey()));
            REQUIRE(SigningPrivateKey::fromPemOrThrow(key.toPem()).matches(key.publicKey()));
            REQUIRE_EQUAL(PublicKey::fromPemOrThrow(key.publicKey().toPem()).toDer(), key.publicKey().toDer());
        }
    }

    void testCertificateHierarchyAndRequest() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto caKey = SigningPrivateKey::generate();
        const auto ca = X509CertificateBuilder::certificateAuthority("Test Root CA"_el)
                            .setOrganization("Example Organization"_el)
                            .createSelfSignedCertificate(caKey);
        REQUIRE(ca.basicConstraints().has_value());
        REQUIRE(ca.basicConstraints()->isCertificateAuthority());
        REQUIRE(caKey.publicKey().verifySignature(
            ca.signatureAlgorithm(), ca.tbsCertificateDer().span(), ca.signatureData().span()));

        const auto serverKey = SigningPrivateKey::generate(SigningKeyProfile::EcdsaP384);
        auto serverBuilder = X509CertificateBuilder::tlsServer("service.example.test"_el);
        serverBuilder.addDnsName("service.example.test"_el);
        serverBuilder.addIpAddress(el::network::IpAddress::fromStringOrThrow("127.0.0.1"_el));
        const auto server = serverBuilder.createCertificate(serverKey, ca, caKey);
        REQUIRE(serverKey.matches(server.publicKey()));
        REQUIRE(server.dnsNames().first() == "service.example.test"_el);
        REQUIRE(caKey.publicKey().verifySignature(
            server.signatureAlgorithm(), server.tbsCertificateDer().span(), server.signatureData().span()));
        REQUIRE(server.validFrom() >= ca.validFrom());
        REQUIRE(server.validTo() <= ca.validTo());

        const auto request = serverBuilder.createSigningRequest(serverKey);
        REQUIRE_FALSE(request.isEmpty());
        REQUIRE(request.toPem().startsWith("-----BEGIN CERTIFICATE REQUEST-----"_el));
        REQUIRE_FALSE(el::cryptology::impl::DerParser{request.toDer()}.parseDocument().isEmpty());
    }

    void testAllCertificateProfilesAndHierarchy() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto rootKey = SigningPrivateKey::generate();
        const auto root = X509CertificateBuilder::certificateAuthority("Profile Root"_el)
                              .setCaPathLength(1U)
                              .createSelfSignedCertificate(rootKey);
        const auto intermediateKey = SigningPrivateKey::generate();
        const auto intermediate = X509CertificateBuilder::certificateAuthority("Profile Intermediate"_el)
                                      .createCertificate(intermediateKey, root, rootKey);
        REQUIRE(intermediate.basicConstraints()->pathLength() == 0U);

        const auto leafKey = SigningPrivateKey::generate();
        auto serverBuilder = X509CertificateBuilder::tlsServer("server.example.test"_el);
        serverBuilder.addDnsName("server.example.test"_el);
        const auto server = serverBuilder.createCertificate(leafKey, intermediate, intermediateKey);
        REQUIRE(server.keyUsage()->isSet(X509KeyUsage::DigitalSignature));
        REQUIRE(server.extendedKeyUsage().count() == el::ItemCount{1U});
        REQUIRE(server.extendedKeyUsage().first().toString() == "1.3.6.1.5.5.7.3.1"_el);

        const auto client = X509CertificateBuilder::tlsClient("client.example.test"_el)
                                .createCertificate(leafKey, intermediate, intermediateKey);
        REQUIRE(client.subjectAlternativeNames().isEmpty());
        REQUIRE(client.extendedKeyUsage().first().toString() == "1.3.6.1.5.5.7.3.2"_el);

        auto dualBuilder = X509CertificateBuilder::tlsServerAndClient("dual.example.test"_el);
        dualBuilder.addIpAddress(el::network::IpAddress::fromStringOrThrow("::1"_el));
        const auto dual = dualBuilder.createCertificate(leafKey, intermediate, intermediateKey);
        REQUIRE(dual.extendedKeyUsage().count() == el::ItemCount{2U});
        REQUIRE(dual.serialNumber() != server.serialNumber());

        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            X509CertificateBuilder::tlsServer("missing-san.example.test"_el)
                .createCertificate(leafKey, intermediate, intermediateKey));
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            X509CertificateBuilder::certificateAuthority("Forbidden Intermediate"_el)
                .createCertificate(SigningPrivateKey::generate(), intermediate, intermediateKey));
    }

    void testArtifactFileRoundTrips() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto directory = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto key = SigningPrivateKey::generate();
        auto builder = X509CertificateBuilder::tlsServer("files.example.test"_el);
        builder.addDnsName("files.example.test"_el);
        const auto request = builder.createSigningRequest(key);
        const auto keyPath = directory->path() / "identity.key"_el;
        const auto publicPath = directory->path() / "identity.pub"_el;
        const auto requestPath = directory->path() / "identity.csr"_el;
        key.writeToFile(keyPath);
        key.publicKey().writeToFile(publicPath);
        request.writeToFile(requestPath);
        REQUIRE(SigningPrivateKey::fromFileOrThrow(keyPath).matches(key.publicKey()));
        REQUIRE_EQUAL(PublicKey::fromFileOrThrow(publicPath).toDer(), key.publicKey().toDer());
        REQUIRE(requestPath.content().readTextOrThrow().startsWith("-----BEGIN CERTIFICATE REQUEST-----"_el));
        REQUIRE_THROWS_AS(el::path::PathError, key.writeToFile(keyPath));
    }

    void testControlledReissuance() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto caKey = SigningPrivateKey::generate();
        const auto ca =
            X509CertificateBuilder::certificateAuthority("Reissuance Root"_el).createSelfSignedCertificate(caKey);
        const auto oldKey = SigningPrivateKey::generate();
        auto oldBuilder = X509CertificateBuilder::tlsServer("old.example.test"_el);
        oldBuilder.setOrganization("Preserved Organization"_el).addDnsName("old.example.test"_el);
        const auto oldCertificate = oldBuilder.createCertificate(oldKey, ca, caKey);

        const auto newKey = SigningPrivateKey::generate();
        auto renewal = X509CertificateBuilder::fromCertificate(oldCertificate, X509CertificateProfile::TlsServer);
        renewal.setCommonName("new.example.test"_el).addDnsName("new.example.test"_el);
        const auto newCertificate = renewal.createCertificate(newKey, ca, caKey);
        REQUIRE(newCertificate.subject().commonNames().first() == "new.example.test"_el);
        REQUIRE(newCertificate.subject().organizations().first() == "Preserved Organization"_el);
        REQUIRE(newCertificate.dnsNames().count() == el::ItemCount{2U});
        REQUIRE(newCertificate.serialNumber() != oldCertificate.serialNumber());
        REQUIRE(newCertificate.subjectId() != oldCertificate.subjectId());
        REQUIRE_EQUAL(oldCertificate.subject().commonNames().first(), "old.example.test"_el);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testRsaGenerationProfiles() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        for (
            const auto profile : {SigningKeyProfile::Rsa2048, SigningKeyProfile::Rsa3072, SigningKeyProfile::Rsa4096}) {
            const auto key = SigningPrivateKey::generate(profile);
            REQUIRE(key.algorithm() == SigningKeyAlgorithm::Rsa);
            REQUIRE(SigningPrivateKey::fromDerOrThrow(key.toDer()).matches(key.publicKey()));
            REQUIRE_EQUAL(PublicKey::fromDerOrThrow(key.publicKey().toDer()).toDer(), key.publicKey().toDer());
            const auto certificate =
                X509CertificateBuilder::certificateAuthority("RSA Profile CA"_el).createSelfSignedCertificate(key);
            REQUIRE(key.publicKey().verifySignature(
                certificate.signatureAlgorithm(),
                certificate.tbsCertificateDer().span(),
                certificate.signatureData().span()));
            const auto request = X509CertificateBuilder::tlsClient("RSA Profile Client"_el).createSigningRequest(key);
            REQUIRE_FALSE(request.isEmpty());
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testEncryptedPrivateKey() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto key = SigningPrivateKey::generate();
        const auto password = el::String{"correct horse battery staple"};
        const auto encrypted = key.toEncryptedDer(password);
        REQUIRE(SigningPrivateKey::fromEncryptedDerOrThrow(encrypted, password).matches(key.publicKey()));
        REQUIRE(
            SigningPrivateKey::fromEncryptedPemOrThrow(key.toEncryptedPem(password), password)
                .matches(key.publicKey()));
        REQUIRE(SigningPrivateKey::fromEncryptedDer(encrypted, el::String{"wrong password"}).isEmpty());
    }
};
