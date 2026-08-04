// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "X509TestData.hpp"

#include <erbsland/cryptology/keys/PublicKey.hpp>
#include <erbsland/cryptology/x509/X509Certificate.hpp>
#include <erbsland/cryptology/x509/X509CertificateBundle.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace el::cryptology;
using namespace el::text::literals;
using erbsland::test::x509::certificatePem;

TESTED_TARGETS(
    X509Certificate X509CertificateBundle X509Version X509CertificateFormat X509CertificateProfileMode
        X509CertificateProfileIssue X509AlgorithmIdentifier X509Name X509NameAttribute X509RelativeDistinguishedName
            X509GeneralName X509Extension X509BasicConstraints X509KeyUsage PublicKey)
class X509CertificateTest final : public el::UnitTest {
public:
    void testEmptyCertificate() {
        const auto certificate = X509Certificate{};
        REQUIRE(certificate.isEmpty());
        REQUIRE(certificate.toPem().isEmpty());
        REQUIRE(certificate.toDer().isEmpty());
        REQUIRE(certificate.toString().isEmpty());
        REQUIRE(certificate.asn1().isEmpty());
        REQUIRE(certificate.publicKey().isEmpty());
        REQUIRE(certificate.subject().isEmpty());
        REQUIRE(certificate.profileIssues().isEmpty());
    }

    void testParseCertificateAndAttributes() {
        const auto certificate = X509Certificate::fromPemOrThrow(certificatePem());
        REQUIRE_FALSE(certificate.isEmpty());
        REQUIRE(certificate.version() == X509Version::V3);
        REQUIRE_EQUAL(certificate.signatureAlgorithmId(), "1.2.840.113549.1.1.11"_el);
        REQUIRE_EQUAL(certificate.publicKey().algorithm().oid().toString(), "1.2.840.113549.1.1.1"_el);
        REQUIRE_FALSE(certificate.publicKey().toDer().isEmpty());
        REQUIRE_FALSE(certificate.tbsCertificateDer().isEmpty());
        REQUIRE_FALSE(certificate.signatureData().isEmpty());
        REQUIRE_EQUAL(certificate.signatureUnusedBitCount(), 0U);
        REQUIRE_EQUAL(certificate.subject().commonNames().first(), "example.test"_el);
        REQUIRE_EQUAL(certificate.subject().organizations().first(), "Erbsland Test"_el);
        REQUIRE_EQUAL(certificate.subject().organizationalUnits().first(), "Core"_el);
        REQUIRE_EQUAL(certificate.subject().countries().first(), "CH"_el);
        REQUIRE_EQUAL(certificate.subject().toString(), "CN=example.test,OU=Core,O=Erbsland Test,C=CH"_el);
        REQUIRE_EQUAL(certificate.issuer().toString(), certificate.subject().toString());
        REQUIRE(certificate.validFrom().isValid());
        REQUIRE(certificate.validTo().isValid());
        REQUIRE(certificate.validTo() > certificate.validFrom());
        REQUIRE(certificate.profileIssues().isEmpty());
    }

    void testVerifySelfSignedCertificate() {
        const auto certificate = X509Certificate::fromPemOrThrow(certificatePem());
        REQUIRE(certificate.publicKey().verifySignature(
            certificate.signatureAlgorithm(),
            certificate.tbsCertificateDer().span(),
            certificate.signatureData().span()));

        auto modifiedMessage = el::mem::ByteBlockEditor{certificate.tbsCertificateDer()};
        modifiedMessage.xorAt(el::unit::ByteIndex{32U}, el::mem::Byte{1U});
        REQUIRE_FALSE(certificate.publicKey().verifySignature(
            certificate.signatureAlgorithm(),
            modifiedMessage.slice(el::unit::ByteIndex::zero(), modifiedMessage.length()).span(),
            certificate.signatureData().span()));

        auto modifiedSignature = el::mem::ByteBlockEditor{certificate.signatureData()};
        modifiedSignature.xorAt(el::unit::ByteIndex{64U}, el::mem::Byte{1U});
        REQUIRE_FALSE(certificate.publicKey().verifySignature(
            certificate.signatureAlgorithm(),
            certificate.tbsCertificateDer().span(),
            modifiedSignature.slice(el::unit::ByteIndex::zero(), modifiedSignature.length()).span()));
    }

    void testTlsExtensions() {
        const auto certificate = X509Certificate::fromPemOrThrow(certificatePem());
        const auto names = certificate.subjectAlternativeNames();
        REQUIRE_EQUAL(names.count(), el::unit::ItemCount{4U});
        REQUIRE(names.toRawValue()[0U].kind() == X509GeneralName::Kind::Dns);
        REQUIRE_EQUAL(names.toRawValue()[0U].text(), "example.test"_el);
        REQUIRE(names.toRawValue()[1U].kind() == X509GeneralName::Kind::Dns);
        REQUIRE_EQUAL(names.toRawValue()[1U].text(), "www.example.test"_el);
        REQUIRE(names.toRawValue()[2U].kind() == X509GeneralName::Kind::IpAddress);
        REQUIRE(names.toRawValue()[2U].ipAddress().has_value());
        REQUIRE_EQUAL(names.toRawValue()[2U].ipAddress()->toString(), "127.0.0.1"_el);
        REQUIRE(names.toRawValue()[3U].kind() == X509GeneralName::Kind::Uri);
        REQUIRE_EQUAL(names.toRawValue()[3U].text(), "https://example.test/"_el);
        REQUIRE_EQUAL(certificate.dnsNames().count(), el::unit::ItemCount{2U});
        REQUIRE_EQUAL(certificate.dnsNames().first(), "example.test"_el);
        REQUIRE_EQUAL(certificate.ipAddresses().count(), el::unit::ItemCount{1U});
        REQUIRE_EQUAL(certificate.ipAddresses().first().toString(), "127.0.0.1"_el);

        const auto constraints = certificate.basicConstraints();
        REQUIRE(constraints.has_value());
        REQUIRE_FALSE(constraints->isCertificateAuthority());
        REQUIRE_FALSE(constraints->pathLength().has_value());
        const auto usage = certificate.keyUsage();
        REQUIRE(usage.has_value());
        REQUIRE(usage->isSet(X509KeyUsage::DigitalSignature));
        REQUIRE(usage->isSet(X509KeyUsage::KeyEncipherment));
        REQUIRE(usage->isCleared(X509KeyUsage::KeyCertificateSign));
        REQUIRE_EQUAL(certificate.extendedKeyUsage().count(), el::unit::ItemCount{2U});
        REQUIRE_EQUAL(certificate.extendedKeyUsage().toRawValue()[0U].toString(), "1.3.6.1.5.5.7.3.1"_el);
        REQUIRE_EQUAL(certificate.extendedKeyUsage().toRawValue()[1U].toString(), "1.3.6.1.5.5.7.3.2"_el);
        REQUIRE_EQUAL(certificate.extensions().count(), el::unit::ItemCount{4U});
    }

    void testRoundTripAndBundle() {
        const auto certificate = X509Certificate::fromPemOrThrow(certificatePem());
        REQUIRE_EQUAL(certificate.toPem(), certificatePem());
        REQUIRE_EQUAL(X509Certificate::fromDerOrThrow(certificate.toDer()).toDer(), certificate.toDer());
        auto bundleText = el::text::StringEditor{};
        bundleText.append(certificatePem());
        bundleText.append(" \r\n\t"_el);
        bundleText.append(certificatePem());
        const auto bundle = X509CertificateBundle::fromPemOrThrow(el::text::String{bundleText});
        REQUIRE_EQUAL(bundle.certificates().count(), el::unit::ItemCount{2U});
        REQUIRE_EQUAL(
            X509CertificateBundle::fromPemOrThrow(bundle.toPem()).certificates().count(), el::unit::ItemCount{2U});
        REQUIRE(X509Certificate::fromPem(bundle.toPem()).isEmpty());
        REQUIRE_THROWS_AS(el::err::ParseError, X509Certificate::fromPemOrThrow(bundle.toPem()));
        REQUIRE_THROWS(bundle.toDer());
    }

    void testCompatibleProfileIssue() {
        const auto certificate = X509Certificate::fromPemOrThrow(certificatePem());
        auto bytes = certificate.toDer().toUInt8Vector();
        constexpr auto pattern = std::array<uint8_t, 15U>{
            0x30U, 0x0DU, 0x06U, 0x09U, 0x2AU, 0x86U, 0x48U, 0x86U, 0xF7U, 0x0DU, 0x01U, 0x01U, 0x0BU, 0x05U, 0x00U};
        auto match = std::size_t{};
        auto found = false;
        for (auto index = std::size_t{}; index + pattern.size() <= bytes.size(); ++index) {
            auto equal = true;
            for (auto patternIndex = std::size_t{}; patternIndex < pattern.size(); ++patternIndex) {
                equal = equal && bytes[index + patternIndex] == pattern[patternIndex];
            }
            if (equal) {
                match = index;
                found = true;
            }
        }
        REQUIRE(found);
        bytes[match + 12U] = 0x0CU;
        const auto mismatched = el::mem::ByteBlock::fromVector(bytes);
        REQUIRE(X509Certificate::fromDer(mismatched).isEmpty());
        const auto compatible = X509Certificate::fromDerOrThrow(mismatched, X509CertificateProfileMode::Compatible);
        REQUIRE_FALSE(compatible.isEmpty());
        REQUIRE_EQUAL(compatible.profileIssues().count(), el::unit::ItemCount{1U});
        REQUIRE(
            compatible.profileIssues().first().category() ==
            X509CertificateProfileIssueCategory::SignatureAlgorithmMismatch);
        REQUIRE_EQUAL(compatible.signatureAlgorithmId(), "1.2.840.113549.1.1.12"_el);
    }

    void testKnownExtensionProfileIssue() {
        const auto certificate = X509Certificate::fromPemOrThrow(certificatePem());
        auto bytes = certificate.toDer().toUInt8Vector();
        constexpr auto pattern = std::array<uint8_t, 4U>{0x03U, 0x02U, 0x05U, 0xA0U};
        auto match = std::size_t{};
        auto matchCount = std::size_t{};
        for (auto index = std::size_t{}; index + pattern.size() <= bytes.size(); ++index) {
            auto equal = true;
            for (auto patternIndex = std::size_t{}; patternIndex < pattern.size(); ++patternIndex) {
                equal = equal && bytes[index + patternIndex] == pattern[patternIndex];
            }
            if (equal) {
                match = index;
                ++matchCount;
            }
        }
        REQUIRE_EQUAL(matchCount, 1U);
        bytes[match + 2U] = 0U;
        const auto malformed = el::mem::ByteBlock::fromVector(bytes);
        REQUIRE(X509Certificate::fromDer(malformed).isEmpty());
        const auto compatible = X509Certificate::fromDerOrThrow(malformed, X509CertificateProfileMode::Compatible);
        REQUIRE_FALSE(compatible.isEmpty());
        REQUIRE_FALSE(compatible.keyUsage().has_value());
        REQUIRE_EQUAL(compatible.profileIssues().count(), el::unit::ItemCount{1U});
        REQUIRE(compatible.profileIssues().first().category() == X509CertificateProfileIssueCategory::ExtensionValue);
    }

    void testMalformedInput() {
        const auto certificate = X509Certificate::fromPemOrThrow(certificatePem());
        auto truncated = certificate.toDer().toUInt8Vector();
        truncated.pop_back();
        REQUIRE(X509Certificate::fromDer(el::mem::ByteBlock::fromVector(truncated)).isEmpty());

        auto trailing = certificate.toDer().toUInt8Vector();
        trailing.push_back(0U);
        REQUIRE(X509Certificate::fromDer(el::mem::ByteBlock::fromVector(trailing)).isEmpty());
        REQUIRE(
            X509Certificate::fromDer(el::mem::ByteBlock::fromVector(std::vector<uint8_t>{0x30U, 0x80U, 0x00U, 0x00U}))
                .isEmpty());
        REQUIRE(
            X509Certificate::fromDer(el::mem::ByteBlock::fromVector(std::vector<uint8_t>{0x30U, 0x81U, 0x00U}))
                .isEmpty());
        REQUIRE(
            X509Certificate::fromPem("-----BEGIN X509 CERTIFICATE-----\nAA==\n-----END X509 CERTIFICATE-----\n"_el)
                .isEmpty());
        REQUIRE(X509Certificate::fromPem("-----BEGIN CERTIFICATE-----\n***\n-----END CERTIFICATE-----\n"_el).isEmpty());
    }

    void testPemWhitespaceAndTransactionalBoundaries() {
        auto decorated = el::text::StringEditor{" \t\n"_el};
        decorated.append(certificatePem()).append("\r\n \t"_el);
        REQUIRE_FALSE(X509Certificate::fromPemOrThrow(el::text::String{decorated}).isEmpty());

        const auto missingRequiredWhitespace =
            certificatePem().replacedFirst("-----BEGIN CERTIFICATE-----\n"_el, "-----BEGIN CERTIFICATE-----"_el);
        REQUIRE(X509Certificate::fromPem(missingRequiredWhitespace).isEmpty());

        const auto partialBoundary =
            certificatePem().replacedFirst("-----BEGIN CERTIFICATE-----"_el, "-----BEGIN CERTIFICATX-----"_el);
        try {
            const auto unexpected = X509Certificate::fromPemOrThrow(partialBoundary);
            REQUIRE(unexpected.isEmpty());
        } catch (const el::err::ParseError &error) {
            REQUIRE(error.codePointIndex().isZero());
        }
    }

    void testDerParseErrorsContainByteIndex() {
        try {
            static_cast<void>(X509Certificate::fromDerOrThrow(
                el::mem::ByteBlock::fromVector(std::vector<uint8_t>{0x30U, 0x80U, 0x00U, 0x00U})));
            REQUIRE(false);
        } catch (const el::err::ParseError &error) {
            REQUIRE_EQUAL(error.byteIndex(), el::unit::ByteIndex{1U});
        }
    }

    void testDerParseErrorsUseSliceRelativeByteIndex() {
        const auto storage = el::mem::ByteBlock::fromVector(
            std::vector<uint8_t>{0xAAU, 0xBBU, 0x30U, 0x06U, 0x05U, 0x00U, 0x05U, 0x00U, 0x05U, 0x00U, 0xCCU});
        const auto der = storage.slice(el::unit::ByteIndex{2U}, el::unit::ByteLength{8U});
        try {
            static_cast<void>(X509Certificate::fromDerOrThrow(der));
            REQUIRE(false);
        } catch (const el::err::ParseError &error) {
            REQUIRE_EQUAL(error.byteIndex(), el::unit::ByteIndex{2U});
        }
    }
};
