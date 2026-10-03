#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/impl/key_agreement/P256KeyAgreement.hpp>
#include <erbsland/CryptologyError.hpp>
#include <erbsland/KeyAgreementAlgorithm.hpp>
#include <erbsland/KeyAgreementPublicKey.hpp>
#include <erbsland/ParameterFailure.hpp>

TESTED_TARGETS(P256KeyAgreement NistPrimeCurve KeyAgreementAlgorithm KeyAgreementPublicKey)
class P256KeyAgreementTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testLibreSslLeadingZeroCoordinate() {
        using namespace el::cryptology::impl;
        const auto scalar = bytesFromHex("000000000000000000000000000000000000000000000000000000000000017b");
        const auto one = bytesFromHex("0000000000000000000000000000000000000000000000000000000000000001");
        const auto publicKey = bytesFromHex(
            "04005543894af3d00ed7d740abdbd75c96b06877b787db5f70eea78b90a8d7c00a"
            "bb4c85a3d8ea29efaafa24406912dd84d5b14dc32bf656ef6c6bd58a5d943f92");
        REQUIRE_EQUAL(p256_key_agreement::publicKey(scalar.span()), publicKey);
        const auto shared = p256_key_agreement::agree(one.span(), publicKey.span());
        REQUIRE_EQUAL(shared, bytesFromHex("005543894af3d00ed7d740abdbd75c96b06877b787db5f70eea78b90a8d7c00a"));
        REQUIRE_EQUAL(shared.length(), el::ByteCount{32U});
        REQUIRE(shared.isSensitive());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testBasePointAndIndependentDoublePoint() {
        using namespace el::cryptology::impl;
        const auto scalarOne = bytesFromHex("0000000000000000000000000000000000000000000000000000000000000001");
        const auto scalarTwo = bytesFromHex("0000000000000000000000000000000000000000000000000000000000000002");
        const auto basePoint = bytesFromHex(
            "046b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296"
            "4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5");
        const auto doublePoint = bytesFromHex(
            "047cf27b188d034f7e8a52380304b51ac3c08969e277f21b35a60b48fc47669978"
            "07775510db8ed040293d9ac69f7430dbba7dade63ce982299e04b79d227873d1");
        REQUIRE_EQUAL(p256_key_agreement::publicKey(scalarOne.span()), basePoint);
        REQUIRE_EQUAL(p256_key_agreement::publicKey(scalarTwo.span()), doublePoint);
        const auto expected = bytesFromHex("7cf27b188d034f7e8a52380304b51ac3c08969e277f21b35a60b48fc47669978");
        const auto first = p256_key_agreement::agree(scalarOne.span(), doublePoint.span());
        const auto second = p256_key_agreement::agree(scalarTwo.span(), basePoint.span());
        REQUIRE_EQUAL(first, expected);
        REQUIRE_EQUAL(second, expected);
        REQUIRE(first.isSensitive());
        REQUIRE(second.isSensitive());
    }

    void testScalarBoundsAndPublicEncoding() {
        using namespace el::cryptology::impl;
        const auto zero = bytesFromHex("0000000000000000000000000000000000000000000000000000000000000000");
        const auto order = bytesFromHex("ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551");
        const auto maximum = bytesFromHex("ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632550");
        REQUIRE_FALSE(p256_key_agreement::isValidPrivateKey({}));
        REQUIRE_FALSE(p256_key_agreement::isValidPrivateKey(zero.span()));
        REQUIRE_FALSE(p256_key_agreement::isValidPrivateKey(order.span()));
        REQUIRE(p256_key_agreement::isValidPrivateKey(maximum.span()));
        REQUIRE_THROWS_AS(el::cryptology::CryptologyError, p256_key_agreement::publicKey(zero.span()));
        REQUIRE_FALSE(p256_key_agreement::isValidPublicKey({}));
        const auto compressed = bytesFromHex("036b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296");
        REQUIRE_FALSE(p256_key_agreement::isValidPublicKey(compressed.span()));
        const auto zeroPoint = bytesFromHex(
            "040000000000000000000000000000000000000000000000000000000000000000"
            "0000000000000000000000000000000000000000000000000000000000000000");
        const auto nonFieldPoint = bytesFromHex(
            "04ffffffff00000001000000000000000000000000ffffffffffffffffffffffff"
            "4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5");
        REQUIRE_FALSE(p256_key_agreement::isValidPublicKey(zeroPoint.span()));
        REQUIRE_FALSE(p256_key_agreement::isValidPublicKey(nonFieldPoint.span()));
        REQUIRE_THROWS_AS(el::cryptology::CryptologyError, p256_key_agreement::agree(maximum.span(), zeroPoint.span()));
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure,
            el::cryptology::KeyAgreementPublicKey(el::cryptology::KeyAgreementAlgorithm::P256, compressed.span()));
        const auto algorithm = el::cryptology::KeyAgreementAlgorithm{el::cryptology::KeyAgreementAlgorithm::P256};
        REQUIRE_EQUAL(algorithm.publicKeySize(), el::ByteCount{65U});
        REQUIRE_EQUAL(algorithm.privateKeySize(), el::ByteCount{32U});
        REQUIRE_EQUAL(algorithm.sharedSecretSize(), el::ByteCount{32U});
    }
};
