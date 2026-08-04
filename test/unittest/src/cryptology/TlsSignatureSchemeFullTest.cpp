// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyResponseReader.hpp"
#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/keys/PublicKey.hpp>
#include <erbsland/cryptology/tls/TlsSignatureScheme.hpp>
#include <erbsland/cryptology/x509/X509AlgorithmIdentifier.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/text/Literals.hpp>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(TlsSignatureScheme PublicKey)
class TlsSignatureSchemeFullTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testCertificateVerifySchemes() {
        WITH_CONTEXT(requireVector(
            TlsSignatureScheme::EcdsaSecp256r1Sha256, "data/cryptology/signature/ECDSA_P256_SHA256.rsp"_el));
        WITH_CONTEXT(requireVector(
            TlsSignatureScheme::EcdsaSecp384r1Sha384, "data/cryptology/signature/ECDSA_P384_SHA384.rsp"_el));
        WITH_CONTEXT(requireVector(TlsSignatureScheme::Ed25519, "data/cryptology/signature/ED25519.rsp"_el));
        WITH_CONTEXT(
            requireVector(TlsSignatureScheme::RsaPssRsaeSha256, "data/cryptology/signature/PSS_SHA256.rsp"_el));
        WITH_CONTEXT(
            requireVector(TlsSignatureScheme::RsaPssRsaeSha384, "data/cryptology/signature/PSS_SHA384.rsp"_el));
        WITH_CONTEXT(requireVector(
            TlsSignatureScheme::RsaPssPssSha256, "data/cryptology/signature/PSS_SHA256_PSS_KEY_SALT32.rsp"_el));

        const auto sha384Records = CryptologyResponseReader{"data/cryptology/signature/PSS_SHA384.rsp"_el}.read();
        const auto &sha384Record = sha384Records.front();
        const auto pssPublicKey = pssSha384PublicKey(bytesFromHex(sha384Record.setting("PublicKeyDer"_el)));
        WITH_CONTEXT(requireRecord(TlsSignatureScheme::RsaPssPssSha384, sha384Record, pssPublicKey));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testCertificateOnlyPkcs1Schemes() {
        WITH_CONTEXT(requireCertificateOnlyVector(
            TlsSignatureScheme::RsaPkcs1Sha256, "data/cryptology/signature/PKCS1_SHA256.rsp"_el));
        WITH_CONTEXT(requireCertificateOnlyVector(
            TlsSignatureScheme::RsaPkcs1Sha384, "data/cryptology/signature/PKCS1_SHA384.rsp"_el));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testKeyAndSchemeConfusion() {
        const auto rsaeRecords = CryptologyResponseReader{"data/cryptology/signature/PSS_SHA256.rsp"_el}.read();
        const auto pssRecords =
            CryptologyResponseReader{"data/cryptology/signature/PSS_SHA256_PSS_KEY_SALT32.rsp"_el}.read();
        const auto ecdsaRecords = CryptologyResponseReader{"data/cryptology/signature/ECDSA_P256_SHA256.rsp"_el}.read();
        const auto &rsaeRecord = rsaeRecords.front();
        const auto &pssRecord = pssRecords.front();
        const auto &ecdsaRecord = ecdsaRecords.front();
        const auto rsaeKey = PublicKey::fromDerOrThrow(bytesFromHex(rsaeRecord.setting("PublicKeyDer"_el)));
        const auto pssKey = PublicKey::fromDerOrThrow(bytesFromHex(pssRecord.setting("PublicKeyDer"_el)));
        const auto ecdsaKey = PublicKey::fromDerOrThrow(bytesFromHex(ecdsaRecord.setting("PublicKeyDer"_el)));
        const auto rsaeMessage = bytesFromHex(rsaeRecord.value("Msg"_el));
        const auto rsaeSignature = bytesFromHex(rsaeRecord.value("Sig"_el));
        const auto pssMessage = bytesFromHex(pssRecord.value("Msg"_el));
        const auto pssSignature = bytesFromHex(pssRecord.value("Sig"_el));
        const auto ecdsaMessage = bytesFromHex(ecdsaRecord.value("Msg"_el));
        const auto ecdsaSignature = bytesFromHex(ecdsaRecord.value("Sig"_el));

        REQUIRE_THROWS_AS(
            el::err::ParseError,
            rsaeKey.verifyTlsCertificateVerifySignature(
                TlsSignatureScheme::RsaPssPssSha256, rsaeMessage.span(), rsaeSignature.span()));
        REQUIRE_THROWS_AS(
            el::err::ParseError,
            pssKey.verifyTlsCertificateVerifySignature(
                TlsSignatureScheme::RsaPssRsaeSha256, pssMessage.span(), pssSignature.span()));
        REQUIRE_THROWS_AS(
            el::err::ParseError,
            ecdsaKey.verifyTlsCertificateVerifySignature(
                TlsSignatureScheme::Ed25519, ecdsaMessage.span(), ecdsaSignature.span()));
        REQUIRE_THROWS_AS(
            el::err::LogicError,
            PublicKey{}.verifyTlsCertificateVerifySignature(
                TlsSignatureScheme::Ed25519, ecdsaMessage.span(), ecdsaSignature.span()));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testModifiedSignature() {
        const auto records = CryptologyResponseReader{"data/cryptology/signature/ED25519.rsp"_el}.read();
        const auto &record = records.front();
        const auto publicKey = PublicKey::fromDerOrThrow(bytesFromHex(record.setting("PublicKeyDer"_el)));
        const auto message = bytesFromHex(record.value("Msg"_el));
        auto signature = el::ByteBlockEditor{bytesFromHex(record.value("Sig"_el))};
        signature.xorAt(el::ByteIndex{17U}, el::Byte{1U});
        REQUIRE_FALSE(publicKey.verifyTlsCertificateVerifySignature(
            TlsSignatureScheme::Ed25519, message.span(), signature.span()));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testExactPssSaltLength() {
        const auto records = CryptologyResponseReader{"data/cryptology/signature/PSS_SHA256_SALT0.rsp"_el}.read();
        const auto &record = records.front();
        const auto publicKey = PublicKey::fromDerOrThrow(bytesFromHex(record.setting("PublicKeyDer"_el)));
        const auto message = bytesFromHex(record.value("Msg"_el));
        const auto signature = bytesFromHex(record.value("Sig"_el));

        // RFC 8446 section 4.2.3 requires the PSS salt length to equal the selected hash output length.
        REQUIRE_FALSE(publicKey.verifyTlsCertificateVerifySignature(
            TlsSignatureScheme::RsaPssRsaeSha256, message.span(), signature.span()));
    }

private:
    void requireVector(const TlsSignatureScheme::Value value, const el::String &path) {
        const auto records = CryptologyResponseReader{path}.read();
        const auto &record = records.front();
        const auto publicKey = PublicKey::fromDerOrThrow(bytesFromHex(record.setting("PublicKeyDer"_el)));
        WITH_CONTEXT(requireRecord(value, record, publicKey));
    }

    void requireRecord(
        const TlsSignatureScheme::Value value,
        const CryptologyResponseReader::Record &record,
        const PublicKey &publicKey) {
        const auto message = bytesFromHex(record.value("Msg"_el));
        const auto signature = bytesFromHex(record.value("Sig"_el));
        REQUIRE(publicKey.verifyTlsCertificateVerifySignature(value, message.span(), signature.span()));
    }

    void requireCertificateOnlyVector(const TlsSignatureScheme::Value value, const el::String &path) {
        const auto records = CryptologyResponseReader{path}.read();
        const auto &record = records.front();
        const auto publicKey = PublicKey::fromDerOrThrow(bytesFromHex(record.setting("PublicKeyDer"_el)));
        const auto algorithm = X509AlgorithmIdentifier::fromDerOrThrow(bytesFromHex(record.setting("AlgorithmDer"_el)));
        const auto message = bytesFromHex(record.value("Msg"_el));
        const auto signature = bytesFromHex(record.value("Sig"_el));
        REQUIRE(publicKey.verifySignature(algorithm, message.span(), signature.span()));
        REQUIRE_THROWS_AS(
            el::err::ParseError,
            publicKey.verifyTlsCertificateVerifySignature(value, message.span(), signature.span()));
    }

    [[nodiscard]] auto pssSha384PublicKey(const el::ByteBlock &rsaePublicKey) -> PublicKey {
        // RFC 4055 section 3.1 uses the PSS parameters in SubjectPublicKeyInfo to restrict every signature operation.
        auto der = el::ByteBlockEditor{};
        der.append(bytesFromHex(
            "30820156"
            "304106092a864886f70d01010a3034"
            "a00f300d06096086480165030402020500"
            "a11c301a06092a864886f70d010108300d06096086480165030402020500"
            "a203020130"));
        der.append(rsaePublicKey.slice(el::ByteIndex{19U}, rsaePublicKey.length() - el::ByteLength{19U}));
        return PublicKey::fromDerOrThrow(el::ByteBlock{der});
    }
};
