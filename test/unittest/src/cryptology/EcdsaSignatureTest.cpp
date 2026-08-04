// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyResponseReader.hpp"
#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/keys/PublicKey.hpp>
#include <erbsland/cryptology/x509/X509AlgorithmIdentifier.hpp>
#include <erbsland/cryptology/x509/X509Certificate.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/text/Literals.hpp>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(EcdsaSignature NistPrimeCurve PublicKey X509AlgorithmIdentifier)
class EcdsaSignatureTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    void testP256KnownAnswersAndCompressedPoints() {
        const auto algorithm = X509AlgorithmIdentifier::fromDerOrThrow(bytesFromHex("300a06082a8648ce3d040302"));

        WITH_CONTEXT(requireValid(
            algorithm,
            "3059301306072a8648ce3d020106082a8648ce3d03010703420004"
            "04aaec73635726f213fb8a9e64da3b8632e41495a944d0045b522eba7240fad5"
            "87d9315798aaa3a5ba01775787ced05eaaf7b4e09fc81d6d1aa546e8365d525d",
            "",
            "3045022100b292a619339f6e567a305c951c0dcbcc42d16e47f219f9e98e76e09d8770b34a"
            "02200177e60492c5a8242f76f07bfe3661bde59ec2a17ce5bd2dab2abebdf89a62e2"));
        WITH_CONTEXT(requireValid(
            algorithm,
            "3039301306072a8648ce3d020106082a8648ce3d03010703220003"
            "04aaec73635726f213fb8a9e64da3b8632e41495a944d0045b522eba7240fad5",
            "",
            "3045022100b292a619339f6e567a305c951c0dcbcc42d16e47f219f9e98e76e09d8770b34a"
            "02200177e60492c5a8242f76f07bfe3661bde59ec2a17ce5bd2dab2abebdf89a62e2"));
        WITH_CONTEXT(requireValid(
            algorithm,
            "3039301306072a8648ce3d020106082a8648ce3d03010703220002"
            "2927b10512bae3eddcfe467828128bad2903269919f7086069c8c4df6c732838",
            "313233343030",
            "304402202ba3a8be6b94d5ec80a6d9d1190a436effe50d85a1eee859b8cc6af9bd5c2e18"
            "02204cd60b855d442f5b3c7b11eb6c4e0ae7525fe710fab9aa7c77a67f79e6fadd76"));
    }

    void testP384KnownAnswer() {
        const auto algorithm = X509AlgorithmIdentifier::fromDerOrThrow(bytesFromHex("300a06082a8648ce3d040303"));
        WITH_CONTEXT(requireValid(
            algorithm,
            "3076301006072a8648ce3d020106052b8104002203620004"
            "29bdb76d5fa741bfd70233cb3a66cc7d44beb3b0663d92a8136650478bcefb61ef182e155a54345a5e8e5e88f064e5bc9"
            "a525ab7f764dad3dae1468c2b419f3b62b9ba917d5e8c4fb1ec47404a3fc76474b2713081be9db4c00e043ada9fc4a3",
            "",
            "3064023032401249714e9091f05a5e109d5c1216fdc05e98614261aa0dbd9e9cd4415dee29238afbd3b103c1e40ee5c9144aee0f"
            "02304326756fb2c4fd726360dd6479b5849478c7a9d054a833a58c1631c33b63c3441336ddf2c7fe0ed129aae6d4ddfeb753"));
    }

    void testP256SelfSignedCertificateIntegration() {
        const auto certificate = X509Certificate::fromPemOrThrow(el::String{R"(-----BEGIN CERTIFICATE-----
MIIBdDCCARoCCQCMg29sAwroMjAKBggqhkjOPQQDAjBCMQswCQYDVQQGEwJDSDEW
MBQGA1UECgwNRXJic2xhbmQtVGVzdDEbMBkGA1UEAwwSZWNkc2EuZXhhbXBsZS50
ZXN0MB4XDTI2MDgxNzE1NDQwNVoXDTI3MDgxNzE1NDQwNVowQjELMAkGA1UEBhMC
Q0gxFjAUBgNVBAoMDUVyYnNsYW5kLVRlc3QxGzAZBgNVBAMMEmVjZHNhLmV4YW1w
bGUudGVzdDBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABFlqXwB5EbZRDXHO9rVR
8KjTuLtcFy9Q+L5NLtvakpoqMvGuKCP8KzO23Y5FleRSfMxDBUMeqzMihS3pbwFw
rXIwCgYIKoZIzj0EAwIDSAAwRQIhAOV62aDdBiBZXvGQIexABN6bUCxrb6adARby
iI0aFcRkAiAlmiyenEKam1whV8N+Bi/5TJJQbrKEV4eRFZJNhHmQ9Q==
-----END CERTIFICATE-----
)"});
        REQUIRE(certificate.publicKey().verifySignature(
            certificate.signatureAlgorithm(),
            certificate.tbsCertificateDer().span(),
            certificate.signatureData().span()));
    }

    void testMalformedAndMismatchedInputs() {
        const auto sha256 = X509AlgorithmIdentifier::fromDerOrThrow(bytesFromHex("300a06082a8648ce3d040302"));
        const auto p384Key = PublicKey::fromDerOrThrow(bytesFromHex(
            "3076301006072a8648ce3d020106052b8104002203620004"
            "29bdb76d5fa741bfd70233cb3a66cc7d44beb3b0663d92a8136650478bcefb61ef182e155a54345a5e8e5e88f064e5bc9"
            "a525ab7f764dad3dae1468c2b419f3b62b9ba917d5e8c4fb1ec47404a3fc76474b2713081be9db4c00e043ada9fc4a3"));
        const auto signature = bytesFromHex("3006020101020101");
        REQUIRE_THROWS_AS(el::err::ParseError, p384Key.verifySignature(sha256, {}, signature.span()));

        const auto p256Key = PublicKey::fromDerOrThrow(bytesFromHex(
            "3059301306072a8648ce3d020106082a8648ce3d03010703420004"
            "04aaec73635726f213fb8a9e64da3b8632e41495a944d0045b522eba7240fad5"
            "87d9315798aaa3a5ba01775787ced05eaaf7b4e09fc81d6d1aa546e8365d525d"));
        REQUIRE_FALSE(p256Key.verifySignature(sha256, {}, signature.span()));
        REQUIRE_THROWS_AS(
            el::err::ParseError, p256Key.verifySignature(sha256, {}, bytesFromHex("300702020001020101").span()));
        REQUIRE_THROWS_AS(
            el::err::ParseError, p256Key.verifySignature(sha256, {}, bytesFromHex("300602010102010100").span()));
        REQUIRE_THROWS_AS(
            el::err::ParseError, p256Key.verifySignature(sha256, {}, bytesFromHex("3006030101020101").span()));
        REQUIRE_FALSE(p256Key.verifySignature(sha256, {}, bytesFromHex("3006020100020101").span()));
        REQUIRE_FALSE(p256Key.verifySignature(
            sha256,
            {},
            bytesFromHex("3026022100ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551020101").span()));
    }

    void testStrictAlgorithmAndPointEncoding() {
        const auto sha256 = X509AlgorithmIdentifier::fromDerOrThrow(bytesFromHex("300a06082a8648ce3d040302"));
        const auto sha256WithNull =
            X509AlgorithmIdentifier::fromDerOrThrow(bytesFromHex("300c06082a8648ce3d0403020500"));
        const auto signature = bytesFromHex("3006020101020101");
        const auto point = std::string_view{"04aaec73635726f213fb8a9e64da3b8632e41495a944d0045b522eba7240fad5"
                                            "87d9315798aaa3a5ba01775787ced05eaaf7b4e09fc81d6d1aa546e8365d525d"};

        const auto absentCurve = PublicKey::fromDerOrThrow(
            bytesFromHex(std::string{"304f300906072a8648ce3d020103420004"} + std::string{point}));
        REQUIRE_THROWS_AS(el::err::ParseError, absentCurve.verifySignature(sha256, {}, signature.span()));

        const auto nullCurve = PublicKey::fromDerOrThrow(
            bytesFromHex(std::string{"3051300b06072a8648ce3d0201050003420004"} + std::string{point}));
        REQUIRE_THROWS_AS(el::err::ParseError, nullCurve.verifySignature(sha256, {}, signature.span()));

        auto nonzeroUnusedBits = std::string{"3059301306072a8648ce3d020106082a8648ce3d03010703420104"};
        nonzeroUnusedBits += point;
        REQUIRE_THROWS_AS(el::err::ParseError, PublicKey::fromDerOrThrow(bytesFromHex(nonzeroUnusedBits)));

        auto hybridPoint = std::string{"3059301306072a8648ce3d020106082a8648ce3d03010703420006"};
        hybridPoint += point;
        const auto hybridKey = PublicKey::fromDerOrThrow(bytesFromHex(hybridPoint));
        REQUIRE_THROWS_AS(el::err::ParseError, hybridKey.verifySignature(sha256, {}, signature.span()));

        const auto infinityKey =
            PublicKey::fromDerOrThrow(bytesFromHex("3019301306072a8648ce3d020106082a8648ce3d03010703020000"));
        REQUIRE_THROWS_AS(el::err::ParseError, infinityKey.verifySignature(sha256, {}, signature.span()));

        const auto shortCompressedKey = PublicKey::fromDerOrThrow(bytesFromHex(
            "3038301306072a8648ce3d020106082a8648ce3d03010703210002"
            "00000000000000000000000000000000000000000000000000000000000001"));
        REQUIRE_THROWS_AS(el::err::ParseError, shortCompressedKey.verifySignature(sha256, {}, signature.span()));

        const auto nonsquareCompressedKey = PublicKey::fromDerOrThrow(bytesFromHex(
            "3039301306072a8648ce3d020106082a8648ce3d03010703220002"
            "0000000000000000000000000000000000000000000000000000000000000001"));
        REQUIRE_THROWS_AS(el::err::ParseError, nonsquareCompressedKey.verifySignature(sha256, {}, signature.span()));

        const auto outOfFieldKey = PublicKey::fromDerOrThrow(bytesFromHex(
            "3059301306072a8648ce3d020106082a8648ce3d03010703420004"
            "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
            "87d9315798aaa3a5ba01775787ced05eaaf7b4e09fc81d6d1aa546e8365d525d"));
        REQUIRE_THROWS_AS(el::err::ParseError, outOfFieldKey.verifySignature(sha256, {}, signature.span()));

        const auto unsupportedCurve = PublicKey::fromDerOrThrow(bytesFromHex(
            "3059301306072a8648ce3d020106082a8648ce3d03010803420004"
            "04aaec73635726f213fb8a9e64da3b8632e41495a944d0045b522eba7240fad5"
            "87d9315798aaa3a5ba01775787ced05eaaf7b4e09fc81d6d1aa546e8365d525d"));
        REQUIRE_THROWS_AS(el::err::ParseError, unsupportedCurve.verifySignature(sha256, {}, signature.span()));

        const auto validKey = PublicKey::fromDerOrThrow(
            bytesFromHex(std::string{"3059301306072a8648ce3d020106082a8648ce3d03010703420004"} + std::string{point}));
        REQUIRE_THROWS_AS(el::err::ParseError, validKey.verifySignature(sha256WithNull, {}, signature.span()));
    }

    void testRsaFamilyRegressionAndAlgorithmConfusion() {
        const auto records = CryptologyResponseReader{"data/cryptology/signature/PSS_SHA256.rsp"_el}.read();
        const auto &record = records.front();
        const auto rsaKey = PublicKey::fromDerOrThrow(bytesFromHex(record.setting("PublicKeyDer"_el)));
        const auto rsaAlgorithm =
            X509AlgorithmIdentifier::fromDerOrThrow(bytesFromHex(record.setting("AlgorithmDer"_el)));
        const auto message = bytesFromHex(record.value("Msg"_el));
        const auto rsaSignature = bytesFromHex(record.value("Sig"_el));
        REQUIRE(rsaKey.verifySignature(rsaAlgorithm, message.span(), rsaSignature.span()));

        const auto ecdsaAlgorithm = X509AlgorithmIdentifier::fromDerOrThrow(bytesFromHex("300a06082a8648ce3d040302"));
        REQUIRE_THROWS_AS(
            el::err::ParseError, rsaKey.verifySignature(ecdsaAlgorithm, message.span(), rsaSignature.span()));
    }

private:
    void requireValid(
        const X509AlgorithmIdentifier &algorithm,
        const std::string_view publicKeyHex,
        const std::string_view messageHex,
        const std::string_view signatureHex) {
        const auto publicKey = PublicKey::fromDerOrThrow(bytesFromHex(publicKeyHex));
        const auto message = bytesFromHex(messageHex);
        const auto signature = bytesFromHex(signatureHex);
        REQUIRE(publicKey.verifySignature(algorithm, message.span(), signature.span()));

        auto modified = el::ByteBlockEditor{signature};
        modified.set(
            el::ByteIndex{modified.length().toSizeT() - 1U},
            modified.get(el::ByteIndex{modified.length().toSizeT() - 1U}) ^ el::Byte{1U});
        REQUIRE_FALSE(publicKey.verifySignature(
            algorithm, message.span(), modified.slice(el::ByteIndex::zero(), modified.length()).span()));
    }
};
