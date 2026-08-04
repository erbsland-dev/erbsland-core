// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyResponseReader.hpp"
#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/x509/X509CertificateBundle.hpp>
#include <erbsland/cryptology/x509/X509CertificateValidation.hpp>
#include <erbsland/cryptology/x509/X509CertificateValidationFailure.hpp>
#include <erbsland/cryptology/x509/X509CertificateValidationFailureCategory.hpp>
#include <erbsland/cryptology/x509/X509ServerCertificatePolicy.hpp>
#include <erbsland/network/HostName.hpp>
#include <erbsland/network/IpAddress.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <initializer_list>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(
    X509ServerCertificatePolicy X509CertificateValidation X509CertificateValidationFailure
        X509CertificateValidationFailureCategory)
class X509ServerCertificatePolicyTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
private:
    enum Fixture : std::size_t {
        RsaRoot,
        RsaIntermediate,
        RsaLeaf,
        RsaWrongIntermediate,
        RsaCommonNameLeaf,
        P256Root,
        P256Leaf,
        P384Root,
        P384Leaf,
        Ed25519Root,
        Ed25519Leaf,
        RsaPssRoot,
        RsaPssLeaf,
    };

public:
    void testExplicitResultForEmptyInputs() {
        const auto emptyPolicy = X509ServerCertificatePolicy{X509CertificateBundle{}};
        const auto emptyPeer =
            emptyPolicy.validate(X509CertificateBundle{}, dnsHost("server.example.test"_el), el::time::DateTime::now());
        REQUIRE(emptyPeer.isRejected());
        REQUIRE_FALSE(emptyPeer.isAccepted());
        REQUIRE(emptyPeer.validatedPath().isEmpty());
        REQUIRE(emptyPeer.failure().has_value());
        REQUIRE(emptyPeer.failure()->category() == X509CertificateValidationFailureCategory::EmptyPeerCertificates);
        REQUIRE_FALSE(emptyPeer.failure()->certificate().has_value());

        const auto noAnchor =
            emptyPolicy.validate(bundle({RsaLeaf}), dnsHost("server.example.test"_el), el::time::DateTime::now());
        REQUIRE(noAnchor.isRejected());
        REQUIRE(noAnchor.failure()->category() == X509CertificateValidationFailureCategory::EmptyTrustAnchors);
        REQUIRE_FALSE(noAnchor.failure()->certificate().has_value());

        const auto invalidTime =
            policy({RsaRoot}).validate(bundle({RsaLeaf, RsaIntermediate}), dnsHost("server.example.test"_el), {});
        REQUIRE(invalidTime.isRejected());
        REQUIRE(invalidTime.failure()->category() == X509CertificateValidationFailureCategory::InvalidValidationTime);
        REQUIRE(invalidTime.failure()->certificate().has_value());
    }

    void testRsaPathConstructionAndDeterministicOrdering() {
        const auto result = policy({RsaRoot}).validate(
            bundle({RsaLeaf, RsaWrongIntermediate, RsaIntermediate}),
            dnsHost("server.example.test"_el),
            el::time::DateTime::now());
        REQUIRE(result.isAccepted());
        REQUIRE_FALSE(result.failure().has_value());
        REQUIRE_EQUAL(result.validatedPath().count(), el::unit::ItemCount{3U});
        REQUIRE_EQUAL(result.validatedPath().toRawValue()[0U].toDer(), certificate(RsaLeaf).toDer());
        REQUIRE_EQUAL(result.validatedPath().toRawValue()[1U].toDer(), certificate(RsaIntermediate).toDer());
        REQUIRE_EQUAL(result.validatedPath().toRawValue()[2U].toDer(), certificate(RsaRoot).toDer());
    }

    void testConfiguredIntermediateAndDuplicateDeduplication() {
        const auto configured =
            policy({RsaRoot}, {RsaIntermediate})
                .validate(bundle({RsaLeaf}), dnsHost("server.example.test"_el), el::time::DateTime::now());
        REQUIRE(configured.isAccepted());
        REQUIRE_EQUAL(configured.validatedPath().count(), el::unit::ItemCount{3U});

        const auto duplicated = policy({RsaRoot}, {RsaIntermediate})
                                    .validate(
                                        bundle({RsaLeaf, RsaIntermediate, RsaIntermediate, RsaRoot}),
                                        dnsHost("server.example.test"_el),
                                        el::time::DateTime::now());
        REQUIRE(duplicated.isAccepted());
        REQUIRE_EQUAL(duplicated.validatedPath().count(), el::unit::ItemCount{3U});
    }

    void testMissingIssuerAndAnchor() {
        const auto missingIssuer =
            policy({RsaRoot}).validate(bundle({RsaLeaf}), dnsHost("server.example.test"_el), el::time::DateTime::now());
        REQUIRE(missingIssuer.isRejected());
        REQUIRE(missingIssuer.failure()->category() == X509CertificateValidationFailureCategory::IssuerNotFound);
        REQUIRE(missingIssuer.failure()->certificate().has_value());
        REQUIRE_FALSE(missingIssuer.failure()->partialPath().isEmpty());

        const auto wrongAnchor =
            policy({P256Root})
                .validate(
                    bundle({RsaLeaf, RsaIntermediate}), dnsHost("server.example.test"_el), el::time::DateTime::now());
        REQUIRE(wrongAnchor.isRejected());
        REQUIRE(wrongAnchor.failure().has_value());
    }

    void testCertificateSignatureAlgorithms() {
        requireAcceptedChain(P256Leaf, P256Root);
        requireAcceptedChain(P384Leaf, P384Root);
        requireAcceptedChain(Ed25519Leaf, Ed25519Root);
        requireAcceptedChain(RsaPssLeaf, RsaPssRoot);
        requireAcceptedChain(RsaLeaf, RsaRoot, RsaIntermediate);
    }

    void testDirectAnchorStillValidatesTarget() {
        const auto accepted = policy({RsaLeaf}).validate(
            bundle({RsaLeaf}), dnsHost("server.example.test"_el), certificate(RsaLeaf).validFrom());
        REQUIRE(accepted.isAccepted());
        REQUIRE_EQUAL(accepted.validatedPath().count(), el::unit::ItemCount{1U});

        const auto inclusiveEnd = policy({RsaLeaf}).validate(
            bundle({RsaLeaf}), dnsHost("server.example.test"_el), certificate(RsaLeaf).validTo());
        REQUIRE(inclusiveEnd.isAccepted());

        const auto notYetValid = policy({RsaLeaf}).validate(
            bundle({RsaLeaf}), dnsHost("server.example.test"_el), el::time::DateTime::first());
        REQUIRE(notYetValid.isRejected());
        REQUIRE(notYetValid.failure()->category() == X509CertificateValidationFailureCategory::CertificateNotYetValid);

        const auto expired = policy({RsaLeaf}).validate(
            bundle({RsaLeaf}), dnsHost("server.example.test"_el), el::time::DateTime::last());
        REQUIRE(expired.isRejected());
        REQUIRE(expired.failure()->category() == X509CertificateValidationFailureCategory::CertificateExpired);
    }

    void testDnsIdentityMatching() {
        REQUIRE(validateDirect(RsaLeaf, dnsHost("server.example.test"_el)).isAccepted());
        REQUIRE(validateDirect(RsaLeaf, dnsHost("SERVER.EXAMPLE.TEST"_el)).isAccepted());
        REQUIRE(validateDirect(RsaLeaf, dnsHost("one.wild.example.test"_el)).isAccepted());
        REQUIRE(validateDirect(RsaLeaf, dnsHost("two.one.wild.example.test"_el)).isRejected());
        REQUIRE(validateDirect(RsaLeaf, dnsHost("wild.example.test"_el)).isRejected());
        REQUIRE(validateDirect(RsaLeaf, dnsHost("xn--bcher-kva.example.test"_el)).isAccepted());
        REQUIRE(validateDirect(RsaLeaf, dnsHost("missing.example.test"_el)).isRejected());

        const auto unicode = validateDirect(RsaLeaf, dnsHost("bücher.example.test"_el));
        REQUIRE(unicode.isAccepted());
    }

    void testIpIdentityAndTypeSeparation() {
        REQUIRE(validateDirect(RsaLeaf, ipHost("192.0.2.10"_el)).isAccepted());
        REQUIRE(validateDirect(RsaLeaf, ipHost("2001:db8::10"_el)).isAccepted());
        REQUIRE(validateDirect(RsaLeaf, ipHost("192.0.2.11"_el)).isRejected());

        const auto dnsLookingLikeIp = validateDirect(RsaLeaf, dnsHost("192.0.2.10"_el));
        REQUIRE(dnsLookingLikeIp.isRejected());
        REQUIRE(
            dnsLookingLikeIp.failure()->category() == X509CertificateValidationFailureCategory::ServerIdentityMismatch);
    }

    void testCommonNameIsNeverFallbackIdentity() {
        const auto result =
            policy({RsaCommonNameLeaf})
                .validate(bundle({RsaCommonNameLeaf}), dnsHost("cn-only.example.test"_el), el::time::DateTime::now());
        REQUIRE(result.isRejected());
        REQUIRE(result.failure()->category() == X509CertificateValidationFailureCategory::ServerIdentityMismatch);
    }

    void testPinnedX509LimboSubset() {
        const auto records = CryptologyResponseReader{"data/cryptology/x509/x509_limbo.rsp"_el}.read();
        REQUIRE_EQUAL(records.size(), std::size_t{13U});
        for (const auto &record : records) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    REQUIRE_EQUAL(record.setting("SourceCommit"_el), "c6040f178a947b3fa4d4a5c118d5594f0e0ca6e2"_el);
                    REQUIRE_EQUAL(record.value("IdentityKind"_el), "DNS"_el);
                    auto peerCertificates = el::util::List<X509Certificate>{X509Certificate::fromDerOrThrow(
                        bytesFromHex(record.value("PeerDer"_el)), X509CertificateProfileMode::Compatible)};
                    const auto intermediateCount = record.unsignedValue("IntermediateCount"_el);
                    for (auto index = std::size_t{}; index < intermediateCount; ++index) {
                        const auto name = el::text::StringFormat{"Intermediate{}Der"_el}.build(index);
                        peerCertificates.append(X509Certificate::fromDerOrThrow(bytesFromHex(record.value(name))));
                    }
                    const auto validationTimeText = record.value("ValidationTime"_el);
                    const auto validationTime = validationTimeText.isEmpty()
                        ? el::time::DateTime::now()
                        : el::time::DateTime::fromIsoStringOrThrow(validationTimeText);
                    const auto anchor = X509Certificate::fromDerOrThrow(bytesFromHex(record.value("AnchorDer"_el)));
                    const auto result =
                        X509ServerCertificatePolicy{X509CertificateBundle{el::util::List<X509Certificate>{anchor}}}
                            .validate(
                                X509CertificateBundle{std::move(peerCertificates)},
                                dnsHost(record.value("Identity"_el)),
                                validationTime);
                    REQUIRE_EQUAL(result.isAccepted(), record.value("Expected"_el) == "SUCCESS"_el);
                },
                [&]() -> std::string { return el::StringConverter{record.diagnostic()}.toStdString(); });
        }
    }

private:
    [[nodiscard]] static auto certificates() -> const X509CertificateBundle & {
        static const auto value = X509CertificateBundle::fromPemOrThrow(
            el::text::String{el::unittest::fh::readDataText("data/cryptology/x509/server_authentication.pem")});
        return value;
    }

    [[nodiscard]] static auto certificate(const Fixture fixture) -> X509Certificate {
        return certificates().certificates().toRawValue()[static_cast<std::size_t>(fixture)];
    }

    [[nodiscard]] static auto bundle(const std::initializer_list<Fixture> fixtures) -> X509CertificateBundle {
        auto values = el::util::List<X509Certificate>{};
        for (const auto fixture : fixtures) {
            values.append(certificate(fixture));
        }
        return X509CertificateBundle{std::move(values)};
    }

    [[nodiscard]] static auto policy(
        const std::initializer_list<Fixture> anchors, const std::initializer_list<Fixture> intermediates = {})
        -> X509ServerCertificatePolicy {
        return X509ServerCertificatePolicy{bundle(anchors), bundle(intermediates)};
    }

    [[nodiscard]] static auto dnsHost(const el::text::String &name) -> el::network::Host {
        return el::network::HostName::fromStringOrThrow(name);
    }

    [[nodiscard]] static auto ipHost(const el::text::String &address) -> el::network::Host {
        return el::network::IpAddress::fromStringOrThrow(address);
    }

    [[nodiscard]] static auto validateDirect(const Fixture leaf, const el::network::Host &host)
        -> X509CertificateValidation {
        return policy({leaf}).validate(bundle({leaf}), host, el::time::DateTime::now());
    }

    void requireAcceptedChain(const Fixture leaf, const Fixture root, const std::optional<Fixture> intermediate = {}) {
        const auto peer = intermediate.has_value() ? bundle({leaf, intermediate.value()}) : bundle({leaf});
        const auto result = policy({root}).validate(peer, dnsHost("server.example.test"_el), el::time::DateTime::now());
        REQUIRE(result.isAccepted());
    }
};
