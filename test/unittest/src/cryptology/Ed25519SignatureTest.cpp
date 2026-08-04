// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/keys/PublicKey.hpp>
#include <erbsland/cryptology/x509/X509AlgorithmIdentifier.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/text/Literals.hpp>

#include <string>
#include <string_view>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(Ed25519Signature PublicKey X509AlgorithmIdentifier)
class Ed25519SignatureTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    void testRfc8032KnownAnswers() {
        WITH_CONTEXT(requireValid(
            "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a",
            "",
            "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e06522490155"
            "5fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b"));
        WITH_CONTEXT(requireValid(
            "3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c",
            "72",
            "92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da"
            "085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00"));
        WITH_CONTEXT(requireValid(
            "fc51cd8e6218a1a38da47ed00230f0580816ed13ba3303ac5deb911548908025",
            "af82",
            "6291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac"
            "18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a"));
    }

    void testRfc8410CertificateSignatureInputs() {
        const auto publicKey = publicKeyFromHex("19bf44096984cdfe8541bac167dc3b96c85086aa30b6b6cb0c5c38ad703166e1");
        const auto tbsCertificate = bytesFromHex(
            "3081dfa00302010202085601474a2a8dc330300506032b657030193117301506035504030c0e4945544620546573742044656d6f"
            "301e170d3136303830313132313932345a170d3430313233313233353935395a30193117301506035504030c0e49455446205465"
            "73742044656d6f302a300506032b656e0321008520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a"
            "a3453043300f0603551d130101ff04053003010100300e0603551d0f01010004040302030830200603551d0e010100041604149b"
            "1f5eeded043385e4f7bc623c5975b90bc8bb3b");
        const auto signature = bytesFromHex(
            "af2301feddc9e6ffc1cca73d74d648a4398082cddb69b14e4d06ecf81a25ce50"
            "d4c2c3eb746c4edd8346856ec86f3dce1a1865c57ac27b50a0c35007f5e7d907");
        REQUIRE(publicKey.verifySignature(ed25519Algorithm(), tbsCertificate.span(), signature.span()));
    }

    void testModifiedAndOutOfRangeSignatures() {
        const auto algorithm = ed25519Algorithm();
        const auto publicKey = publicKeyFromHex("d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a");
        const auto signature = bytesFromHex(
            "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e06522490155"
            "5fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b");

        auto modified = el::ByteBlockEditor{signature};
        modified.xorAt(el::ByteIndex{17U}, el::Byte{1U});
        REQUIRE_FALSE(publicKey.verifySignature(algorithm, {}, modified.span()));

        auto orderAsS = el::ByteBlockEditor{signature};
        orderAsS.overwrite(
            el::ByteIndex{32U},
            bytesFromHex("edd3f55c1a631258d69cf7a2def9de1400000000000000000000000000000010").span());
        REQUIRE_FALSE(publicKey.verifySignature(algorithm, {}, orderAsS.span()));
    }

    void testCanonicalPointAndSubgroupChecks() {
        const auto algorithm = ed25519Algorithm();
        const auto validKey = publicKeyFromHex("d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a");

        auto noncanonicalR = std::string{"ed"};
        noncanonicalR += std::string(60U, 'f');
        noncanonicalR += "7f";
        noncanonicalR += std::string(64U, '0');
        REQUIRE_FALSE(validKey.verifySignature(algorithm, {}, bytesFromHex(noncanonicalR).span()));

        auto negativeZeroR = std::string{"01"};
        negativeZeroR += std::string(60U, '0');
        negativeZeroR += "80";
        negativeZeroR += std::string(64U, '0');
        REQUIRE_FALSE(validKey.verifySignature(algorithm, {}, bytesFromHex(negativeZeroR).span()));

        auto identityKeyHex = std::string{"01"};
        identityKeyHex += std::string(62U, '0');
        const auto identityKey = publicKeyFromHex(identityKeyHex);
        REQUIRE_THROWS_AS(
            el::err::ParseError,
            identityKey.verifySignature(algorithm, {}, bytesFromHex(std::string(128U, '0')).span()));
    }

    void testStrictX509EncodingAndAlgorithmConfusion() {
        const auto algorithm = ed25519Algorithm();
        const auto algorithmWithNull = X509AlgorithmIdentifier::fromDerOrThrow(bytesFromHex("300706032b65700500"));
        const auto publicKey = publicKeyFromHex("d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a");
        const auto signature = bytesFromHex(
            "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e06522490155"
            "5fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b");

        REQUIRE_THROWS_AS(el::err::ParseError, publicKey.verifySignature(algorithmWithNull, {}, signature.span()));
        REQUIRE_THROWS_AS(el::err::ParseError, publicKey.verifySignature(algorithm, {}, signature.span().first(63U)));

        const auto keyWithNull = PublicKey::fromDerOrThrow(bytesFromHex(
            "302c300706032b65700500032100"
            "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a"));
        REQUIRE_THROWS_AS(el::err::ParseError, keyWithNull.verifySignature(algorithm, {}, signature.span()));

        const auto shortKey = PublicKey::fromDerOrThrow(bytesFromHex(
            "3029300506032b6570032000"
            "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f70751"));
        REQUIRE_THROWS_AS(el::err::ParseError, shortKey.verifySignature(algorithm, {}, signature.span()));

        const auto ecdsaAlgorithm = X509AlgorithmIdentifier::fromDerOrThrow(bytesFromHex("300a06082a8648ce3d040302"));
        REQUIRE_THROWS_AS(el::err::ParseError, publicKey.verifySignature(ecdsaAlgorithm, {}, signature.span()));
    }

private:
    [[nodiscard]] static auto ed25519Algorithm() -> X509AlgorithmIdentifier {
        return X509AlgorithmIdentifier::fromDerOrThrow(bytesFromHex("300506032b6570"));
    }

    [[nodiscard]] static auto publicKeyFromHex(const std::string_view keyHex) -> PublicKey {
        return PublicKey::fromDerOrThrow(bytesFromHex(std::string{"302a300506032b6570032100"} + std::string{keyHex}));
    }

    void requireValid(
        const std::string_view publicKeyHex, const std::string_view messageHex, const std::string_view signatureHex) {
        const auto publicKey = publicKeyFromHex(publicKeyHex);
        const auto message = bytesFromHex(messageHex);
        const auto signature = bytesFromHex(signatureHex);
        REQUIRE(publicKey.verifySignature(ed25519Algorithm(), message.span(), signature.span()));
    }
};
