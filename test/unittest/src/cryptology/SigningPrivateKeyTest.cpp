// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/configuration/CryptologyConfiguration.hpp>
#include <erbsland/cryptology/impl/algorithm/rsa_signature/RsaSigner.hpp>
#include <erbsland/cryptology/impl/DerEncoder.hpp>
#include <erbsland/cryptology/impl/DerParser.hpp>
#include <erbsland/cryptology/impl/PrivateKeyParser.hpp>
#include <erbsland/cryptology/keys/SigningPrivateKey.hpp>
#include <erbsland/cryptology/protected_data/ProtectedDataMode.hpp>
#include <erbsland/err/ParseError.hpp>

#include <type_traits>

using namespace el::cryptology;

TESTED_TARGETS(SigningPrivateKey SigningKeyAlgorithm)
class SigningPrivateKeyTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    static_assert(!std::is_copy_constructible_v<SigningPrivateKey>);
    static_assert(!std::is_copy_assignable_v<SigningPrivateKey>);
    static_assert(std::is_move_constructible_v<SigningPrivateKey>);

    void testEd25519Rfc8032Vector() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto key = SigningPrivateKey::fromDerOrThrow(bytesFromHex(
            "302e020100300506032b657004220420"
            "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60"));
        REQUIRE(key.algorithm() == SigningKeyAlgorithm::Ed25519);
        REQUIRE(key.supports(TlsSignatureScheme::Ed25519));
        REQUIRE_FALSE(key.supports(TlsSignatureScheme::EcdsaSecp256r1Sha256));
        REQUIRE_EQUAL(
            key.publicKey().keyData(),
            bytesFromHex("d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a"));

        const auto signature = key.signTlsCertificateVerify(TlsSignatureScheme::Ed25519, {});
        REQUIRE_EQUAL(
            signature,
            bytesFromHex(
                "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e06522490155"
                "5fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b"));
        REQUIRE(key.publicKey().verifyTlsCertificateVerifySignature(TlsSignatureScheme::Ed25519, {}, signature.span()));
    }

    void testEd25519StrictPkcs8() {
        REQUIRE(SigningPrivateKey::fromDer({}).isEmpty());
        REQUIRE_THROWS_AS(
            el::err::ParseError,
            SigningPrivateKey::fromDerOrThrow(bytesFromHex(
                "3030020100300706032b6570050004220420"
                "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60")));
        REQUIRE_THROWS_AS(
            el::err::ParseError,
            SigningPrivateKey::fromDerOrThrow(bytesFromHex(
                "302d020100300506032b65700421041f"
                "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f")));
    }

    void testEd25519StrictPemProfile() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto pem = el::String{"-----BEGIN PRIVATE KEY-----\n"
                                    "MC4CAQAwBQYDK2VwBCIEIJ1hsZ3v/VpguoRK9JLsLMREScVpezJpGXA7rAMcrn9g\n"
                                    "-----END PRIVATE KEY-----\n"};
        REQUIRE(SigningPrivateKey::fromPemOrThrow(pem).algorithm() == SigningKeyAlgorithm::Ed25519);
        REQUIRE_THROWS_AS(
            el::err::ParseError,
            SigningPrivateKey::fromPemOrThrow(
                el::String{"-----BEGIN ENCRYPTED PRIVATE KEY-----\nAA==\n-----END ENCRYPTED PRIVATE KEY-----\n"}));
        REQUIRE_THROWS_AS(el::err::ParseError, SigningPrivateKey::fromPemOrThrow(el::String::fromJoined({pem, pem})));
    }

    void testEcdsaP256Rfc6979Vector() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto key = SigningPrivateKey::fromDerOrThrow(bytesFromHex(
            "3041020100301306072a8648ce3d020106082a8648ce3d030107042730250201010420"
            "c9afa9d845ba75166b5c215767b1d6934e50c3db36e89b127b8a622b120f6721"));
        REQUIRE(key.algorithm() == SigningKeyAlgorithm::EcdsaP256);
        REQUIRE(key.supports(TlsSignatureScheme::EcdsaSecp256r1Sha256));
        REQUIRE_EQUAL(
            key.publicKey().keyData(),
            bytesFromHex(
                "0460fed4ba255a9d31c961eb74c6356d68c049b8923b61fa6ce669622e60f29fb67903fe1008b8bc99a41ae9e95628bc64f2"
                "f1b20c2d7e9f5177a3c294d4462299"));

        const auto message = bytesFromHex("73616d706c65");
        const auto signature = key.signTlsCertificateVerify(TlsSignatureScheme::EcdsaSecp256r1Sha256, message.span());
        REQUIRE_EQUAL(
            signature,
            bytesFromHex(
                "3046022100efd48b2aacb6a8fd1140dd9cd45e81d69d2c877b56aaf991c34d0ea84eaf3716"
                "022100f7cb1c942d657c41d436c7a1b6e29f65f3e900dbb9aff4064dc4ab2f843acda8"));
        REQUIRE(key.publicKey().verifyTlsCertificateVerifySignature(
            TlsSignatureScheme::EcdsaSecp256r1Sha256, message.span(), signature.span()));

        // RFC 5480 section 2.2: the same affine leaf point may use compressed SEC 1 form.
        const auto compressed = PublicKey::fromDerOrThrow(bytesFromHex(
            "3039301306072a8648ce3d020106082a8648ce3d03010703220003"
            "60fed4ba255a9d31c961eb74c6356d68c049b8923b61fa6ce669622e60f29fb6"));
        REQUIRE(key.matches(compressed));
    }

    void testEmptyAndSecureErase() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        auto key = SigningPrivateKey{};
        REQUIRE(key.isEmpty());
        REQUIRE_THROWS(key.publicKey());
        REQUIRE_THROWS(key.signTlsCertificateVerify(TlsSignatureScheme::Ed25519, {}));

        key = SigningPrivateKey::fromDerOrThrow(bytesFromHex(
            "302e020100300506032b657004220420"
            "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60"));
        key.secureErase();
        REQUIRE(key.isEmpty());
        REQUIRE(key.algorithm() == SigningKeyAlgorithm::Unknown);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testRsaPssRoundTrip() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto pem = el::String{el::unittest::fh::readDataText("data/cryptology/signing/rsa-2048-pkcs8.pem")};
        const auto key = SigningPrivateKey::fromPemOrThrow(pem);
        REQUIRE(key.algorithm() == SigningKeyAlgorithm::Rsa);
        REQUIRE(key.supports(TlsSignatureScheme::RsaPssRsaeSha256));
        REQUIRE(key.supports(TlsSignatureScheme::RsaPssRsaeSha384));
        REQUIRE_FALSE(key.supports(TlsSignatureScheme::RsaPssPssSha256));
        REQUIRE_FALSE(key.supports(TlsSignatureScheme::RsaPkcs1Sha256));

        const auto message = bytesFromHex("5253412d50535320736572766572207369676e696e67");
        const auto der = el::cryptology::impl::PrivateKeyParser::decodePem(pem);
        const auto privateKeyInfo = el::cryptology::impl::DerParser{der}.parseDocument();
        const auto privateKey = privateKeyInfo.child(el::ItemIndex{2U}).contentData();

        // RFC 8017 appendix A.1.2: altering qInv must fail the mandatory CRT consistency validation.
        auto corruptedPrivateKey = el::ByteBlockEditor{privateKey};
        const auto lastPrivateKeyByte = el::ByteIndex{corruptedPrivateKey.length().toSizeT() - 1U};
        corruptedPrivateKey.set(lastPrivateKeyByte, corruptedPrivateKey.getOrThrow(lastPrivateKeyByte) ^ el::Byte{1U});
        auto corruptedChildren = el::ByteBlockEditor{};
        corruptedChildren.append(privateKeyInfo.child(el::ItemIndex{0U}).encodedData());
        corruptedChildren.append(privateKeyInfo.child(el::ItemIndex{1U}).encodedData());
        corruptedChildren.append(el::cryptology::impl::der_encoder::octetString(corruptedPrivateKey.span()));
        REQUIRE_THROWS_AS(
            el::err::ParseError,
            SigningPrivateKey::fromDerOrThrow(el::cryptology::impl::der_encoder::sequence(corruptedChildren.span())));

        // RFC 4055 section 3.1: an id-RSASSA-PSS key restricts both the TLS key family and the permitted hash.
        const auto pssAlgorithm = bytesFromHex(
            "304106092a864886f70d01010a3034a00f300d06096086480165030402010500"
            "a11c301a06092a864886f70d010108300d06096086480165030402010500a203020120");
        auto pssChildren = el::ByteBlockEditor{};
        pssChildren.append(privateKeyInfo.child(el::ItemIndex{0U}).encodedData());
        pssChildren.append(pssAlgorithm);
        pssChildren.append(el::cryptology::impl::der_encoder::octetString(privateKey.span()));
        const auto pssKey =
            SigningPrivateKey::fromDerOrThrow(el::cryptology::impl::der_encoder::sequence(pssChildren.span()));
        REQUIRE(pssKey.supports(TlsSignatureScheme::RsaPssPssSha256));
        REQUIRE_FALSE(pssKey.supports(TlsSignatureScheme::RsaPssPssSha384));
        REQUIRE_FALSE(pssKey.supports(TlsSignatureScheme::RsaPssRsaeSha256));
        const auto salt = bytesFromHex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
        auto blindingFactor = el::ByteBlockEditor{el::ByteLength{256U}};
        blindingFactor.set(el::ByteIndex{255U}, el::Byte{2U});
        const auto signature = el::cryptology::impl::rsa_signer::signWithRandom(
            privateKey.span(),
            TlsSignatureScheme::RsaPssRsaeSha256,
            message.span(),
            key.publicKey(),
            salt.span(),
            blindingFactor.span());
        const auto repeatedSignature = el::cryptology::impl::rsa_signer::signWithRandom(
            privateKey.span(),
            TlsSignatureScheme::RsaPssRsaeSha256,
            message.span(),
            key.publicKey(),
            salt.span(),
            blindingFactor.span());
        REQUIRE_EQUAL(signature, repeatedSignature);
        REQUIRE(key.publicKey().verifyTlsCertificateVerifySignature(
            TlsSignatureScheme::RsaPssRsaeSha256, message.span(), signature.span()));
    }
};
