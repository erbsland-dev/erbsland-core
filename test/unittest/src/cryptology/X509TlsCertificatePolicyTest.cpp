// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyResponseReader.hpp"
#include "CryptologyTestHelper.hpp"

#include <erbsland/ByteBlockEditor.hpp>
#include <erbsland/HostName.hpp>
#include <erbsland/IpAddress.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/X509CertificateBundle.hpp>
#include <erbsland/X509CertificateValidation.hpp>
#include <erbsland/X509CertificateValidationFailure.hpp>
#include <erbsland/X509CertificateValidationFailureCategory.hpp>
#include <erbsland/X509TlsCertificatePolicy.hpp>

#include <initializer_list>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(
    X509TlsCertificatePolicy X509CertificateValidation X509CertificateValidationFailure
        X509CertificateValidationFailureCategory)
class X509TlsCertificatePolicyTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
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
    void testClientAuthenticationLight() {
        const auto client = X509CertificateBundle::fromPemOrThrow(
            el::String{el::unittest::fh::readDataText("data/network/tls-interop/client.pem")});
        REQUIRE(X509TlsCertificatePolicy{client}.validateClient(client).isAccepted());
        REQUIRE(policy({P256Leaf}).validateClient(bundle({P256Leaf})).isRejected());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testClientPurposeConstraintsAndValidity() {
        const auto fixtures = X509CertificateBundle::fromPemOrThrow(
            el::String{el::unittest::fh::readDataText("data/cryptology/x509/client_authentication.pem")},
            X509CertificateProfileMode::Compatible);
        const auto &certificates = fixtures.certificates().toRawValue();
        const auto at = [&](std::size_t index) {
            return X509CertificateBundle{el::util::List<X509Certificate>{certificates[index]}};
        };
        const auto policy = X509TlsCertificatePolicy{at(0)};
        const auto time = el::time::Timestamp::fromIsoStringOrThrow("2027-01-01T00:00:00Z"_el);
        // A client CN is an application identity, never a DNS/IP reference identity.
        REQUIRE(policy.validateClient(at(1), time).isAccepted());
        REQUIRE(policy.validateClient(at(3), time).isAccepted());
        using Category = X509CertificateValidationFailureCategory;
        for (
            const auto [index, category] :
            {std::pair{2U, Category::ExtendedKeyUsageRejected},
                std::pair{4U, Category::CertificateProfileRejected},
                std::pair{5U, Category::KeyUsageRejected},
                std::pair{6U, Category::UnknownCriticalExtension}}) {
            const auto result = policy.validateClient(at(index), time);
            REQUIRE(result.isRejected());
            REQUIRE_EQUAL(result.failure()->category(), category);
        }
        REQUIRE_EQUAL(
            policy.validateClient(at(1), el::time::Timestamp::first()).failure()->category(),
            Category::CertificateNotYetValid);
        REQUIRE_EQUAL(
            policy.validateClient(at(1), el::time::Timestamp::last()).failure()->category(),
            Category::CertificateExpired);
        const auto wrongPurpose = policy.validateClient(
            X509CertificateBundle{el::util::List<X509Certificate>{certificates[8], certificates[7]}}, time);
        REQUIRE(wrongPurpose.isRejected());
        REQUIRE_EQUAL(wrongPurpose.failure()->category(), Category::ExtendedKeyUsageRejected);
        REQUIRE(policy
                .validateClient(
                    X509CertificateBundle{el::util::List<X509Certificate>{certificates[10], certificates[9]}}, time)
                .isAccepted());
        auto damaged = el::mem::ByteBlockEditor{certificates[1].toDer()};
        damaged.set(damaged.lastIndex(), certificates[1].toDer().span().back() ^ el::mem::Byte{1U});
        const auto invalidSignature = policy.validateClient(
            X509CertificateBundle{el::util::List<X509Certificate>{X509Certificate::fromDerOrThrow(damaged)}}, time);
        REQUIRE(invalidSignature.isRejected());
        auto oversized = el::util::List<X509Certificate>{};
        for (auto i = 0U; i <= X509TlsCertificatePolicy::cMaximumCandidateCertificates; ++i) {
            auto der = el::mem::ByteBlockEditor{certificates[1].toDer()};
            der.set(der.lastIndex(), el::mem::Byte{static_cast<uint8_t>(i)});
            der.set(der.lastIndex().decremented(), el::mem::Byte{static_cast<uint8_t>(i >> 8U)});
            oversized.append(X509Certificate::fromDerOrThrow(der));
        }
        REQUIRE_EQUAL(
            policy.validateClient(X509CertificateBundle{oversized}, time).failure()->category(),
            Category::CandidateLimitExceeded);
    }

    void testExplicitResultForEmptyInputs() {
        const auto emptyPolicy = X509TlsCertificatePolicy{X509CertificateBundle{}};
        const auto emptyPeer = emptyPolicy.validateServer(
            X509CertificateBundle{}, dnsHost("server.example.test"_el), el::time::Timestamp::now());
        REQUIRE(emptyPeer.isRejected());
        REQUIRE_FALSE(emptyPeer.isAccepted());
        REQUIRE(emptyPeer.validatedPath().isEmpty());
        REQUIRE(emptyPeer.failure().has_value());
        REQUIRE(emptyPeer.failure()->category() == X509CertificateValidationFailureCategory::EmptyPeerCertificates);
        REQUIRE_FALSE(emptyPeer.failure()->certificate().has_value());

        const auto noAnchor = emptyPolicy.validateServer(
            bundle({RsaLeaf}), dnsHost("server.example.test"_el), el::time::Timestamp::now());
        REQUIRE(noAnchor.isRejected());
        REQUIRE(noAnchor.failure()->category() == X509CertificateValidationFailureCategory::EmptyTrustAnchors);
        REQUIRE_FALSE(noAnchor.failure()->certificate().has_value());

        const auto invalidTime =
            policy({RsaRoot}).validateServer(bundle({RsaLeaf, RsaIntermediate}), dnsHost("server.example.test"_el), {});
        REQUIRE(invalidTime.isRejected());
        REQUIRE(invalidTime.failure()->category() == X509CertificateValidationFailureCategory::InvalidValidationTime);
        REQUIRE(invalidTime.failure()->certificate().has_value());
    }

    void testRsaPathConstructionAndDeterministicOrdering() {
        const auto result = policy({RsaRoot}).validateServer(
            bundle({RsaLeaf, RsaWrongIntermediate, RsaIntermediate}),
            dnsHost("server.example.test"_el),
            el::time::Timestamp::now());
        REQUIRE(result.isAccepted());
        REQUIRE_FALSE(result.failure().has_value());
        REQUIRE_EQUAL(result.validatedPath().count(), el::unit::ItemCount{3U});
        REQUIRE_EQUAL(result.validatedPath().toRawValue()[0U].toDer(), certificate(RsaLeaf).toDer());
        REQUIRE_EQUAL(result.validatedPath().toRawValue()[1U].toDer(), certificate(RsaIntermediate).toDer());
        REQUIRE_EQUAL(result.validatedPath().toRawValue()[2U].toDer(), certificate(RsaRoot).toDer());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testConfiguredIntermediateAndDuplicateDeduplication() {
        const auto configured =
            policy({RsaRoot}, {RsaIntermediate})
                .validateServer(bundle({RsaLeaf}), dnsHost("server.example.test"_el), el::time::Timestamp::now());
        REQUIRE(configured.isAccepted());
        REQUIRE_EQUAL(configured.validatedPath().count(), el::unit::ItemCount{3U});

        const auto duplicated = policy({RsaRoot}, {RsaIntermediate})
                                    .validateServer(
                                        bundle({RsaLeaf, RsaIntermediate, RsaIntermediate, RsaRoot}),
                                        dnsHost("server.example.test"_el),
                                        el::time::Timestamp::now());
        REQUIRE(duplicated.isAccepted());
        REQUIRE_EQUAL(duplicated.validatedPath().count(), el::unit::ItemCount{3U});
    }

    void testMissingIssuerAndAnchor() {
        const auto missingIssuer = policy({RsaRoot}).validateServer(
            bundle({RsaLeaf}), dnsHost("server.example.test"_el), el::time::Timestamp::now());
        REQUIRE(missingIssuer.isRejected());
        REQUIRE(missingIssuer.failure()->category() == X509CertificateValidationFailureCategory::IssuerNotFound);
        REQUIRE(missingIssuer.failure()->certificate().has_value());
        REQUIRE_FALSE(missingIssuer.failure()->partialPath().isEmpty());

        const auto wrongAnchor =
            policy({P256Root})
                .validateServer(
                    bundle({RsaLeaf, RsaIntermediate}), dnsHost("server.example.test"_el), el::time::Timestamp::now());
        REQUIRE(wrongAnchor.isRejected());
        REQUIRE(wrongAnchor.failure().has_value());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testCertificateSignatureAlgorithms() {
        requireAcceptedChain(P256Leaf, P256Root);
        requireAcceptedChain(P384Leaf, P384Root);
        requireAcceptedChain(Ed25519Leaf, Ed25519Root);
        requireAcceptedChain(RsaPssLeaf, RsaPssRoot);
        requireAcceptedChain(RsaLeaf, RsaRoot, RsaIntermediate);
    }

    void testDirectAnchorStillValidatesTarget() {
        const auto accepted = policy({RsaLeaf}).validateServer(
            bundle({RsaLeaf}), dnsHost("server.example.test"_el), certificate(RsaLeaf).validFrom());
        REQUIRE(accepted.isAccepted());
        REQUIRE_EQUAL(accepted.validatedPath().count(), el::unit::ItemCount{1U});

        const auto inclusiveEnd = policy({RsaLeaf}).validateServer(
            bundle({RsaLeaf}), dnsHost("server.example.test"_el), certificate(RsaLeaf).validTo());
        REQUIRE(inclusiveEnd.isAccepted());

        const auto notYetValid = policy({RsaLeaf}).validateServer(
            bundle({RsaLeaf}), dnsHost("server.example.test"_el), el::time::Timestamp::first());
        REQUIRE(notYetValid.isRejected());
        REQUIRE(notYetValid.failure()->category() == X509CertificateValidationFailureCategory::CertificateNotYetValid);

        const auto expired = policy({RsaLeaf}).validateServer(
            bundle({RsaLeaf}), dnsHost("server.example.test"_el), el::time::Timestamp::last());
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
                .validateServer(
                    bundle({RsaCommonNameLeaf}), dnsHost("cn-only.example.test"_el), el::time::Timestamp::now());
        REQUIRE(result.isRejected());
        REQUIRE(result.failure()->category() == X509CertificateValidationFailureCategory::ServerIdentityMismatch);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
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
                        ? el::time::Timestamp::now()
                        : el::time::Timestamp::fromIsoStringOrThrow(validationTimeText);
                    const auto anchor = X509Certificate::fromDerOrThrow(bytesFromHex(record.value("AnchorDer"_el)));
                    const auto result =
                        X509TlsCertificatePolicy{X509CertificateBundle{el::util::List<X509Certificate>{anchor}}}
                            .validateServer(
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
        -> X509TlsCertificatePolicy {
        return X509TlsCertificatePolicy{bundle(anchors), bundle(intermediates)};
    }

    [[nodiscard]] static auto dnsHost(const el::text::String &name) -> el::network::Host {
        return el::network::HostName::fromStringOrThrow(name);
    }

    [[nodiscard]] static auto ipHost(const el::text::String &address) -> el::network::Host {
        return el::network::IpAddress::fromStringOrThrow(address);
    }

    [[nodiscard]] static auto validateDirect(const Fixture leaf, const el::network::Host &host)
        -> X509CertificateValidation {
        return policy({leaf}).validateServer(bundle({leaf}), host, el::time::Timestamp::now());
    }

    void requireAcceptedChain(const Fixture leaf, const Fixture root, const std::optional<Fixture> intermediate = {}) {
        const auto peer = intermediate.has_value() ? bundle({leaf, intermediate.value()}) : bundle({leaf});
        const auto result =
            policy({root}).validateServer(peer, dnsHost("server.example.test"_el), el::time::Timestamp::now());
        REQUIRE(result.isAccepted());
    }
};
