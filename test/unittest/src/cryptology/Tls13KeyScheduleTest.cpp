// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/cryptology/Hasher.hpp>
#include <erbsland/cryptology/impl/tls/Tls13KeySchedule.hpp>
#include <erbsland/cryptology/impl/tls/Tls13KeyScheduleTestAccess.hpp>
#include <erbsland/cryptology/impl/tls/Tls13Transcript.hpp>
#include <erbsland/cryptology/keys/KeyAgreementAlgorithm.hpp>
#include <erbsland/cryptology/keys/KeyAgreementPrivateKey.hpp>
#include <erbsland/cryptology/keys/KeyAgreementPublicKey.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/mem/ByteBlock.hpp>

using namespace el::cryptology;
using el::cryptology::impl::Tls13KeySchedule;
using el::cryptology::impl::Tls13KeyScheduleTestAccess;
using el::cryptology::impl::Tls13Transcript;
using el::mem::ByteBlock;

TESTED_TARGETS(Tls13Hkdf Tls13KeySchedule Tls13Transcript)
class Tls13KeyScheduleTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    void testRfc8448SimpleHandshakeSchedule() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto clientPrivateBytes =
            bytesFromHex("49af42ba7f7994852d713ef2784bcbcaa7911de26adc5642cb634540e7ea5005");
        const auto serverPublicBytes = bytesFromHex("c9828876112095fe66762bdbf7c672e156d6cc253b833df1dd69b1b04e751f0f");
        auto clientPrivate =
            KeyAgreementPrivateKey::fromBytes(KeyAgreementAlgorithm::X25519, clientPrivateBytes.span());
        const auto serverPublic = KeyAgreementPublicKey{KeyAgreementAlgorithm::X25519, serverPublicBytes.span()};
        auto schedule = Tls13KeySchedule{HashAlgorithm::Sha2_256};
        const auto access = Tls13KeyScheduleTestAccess{schedule};

        // RFC 8448 section 3: the non-PSK early secret is HKDF-Extract with all-zero inputs.
        REQUIRE_EQUAL(
            access.bytes(Tls13KeyScheduleTestAccess::Secret::Early),
            bytesFromHex("33ad0a1c607ec03b09e6cd9893680ce210adf300aa1f2660e1b22e10f170f92a"));

        const auto helloTranscriptHash =
            bytesFromHex("860c06edc07858ee8e78f0e7428c58edd6b43f2ca3e6e95f02ed063cf0e1cad8");
        schedule.initializeHandshake(clientPrivate.agree(serverPublic), helloTranscriptHash.span());
        REQUIRE_EQUAL(
            access.bytes(Tls13KeyScheduleTestAccess::Secret::ClientHandshake),
            bytesFromHex("b3eddb126e067f35a780b3abf45e2d8f3b1a950738f52e9600746a0e27a55a21"));
        REQUIRE_EQUAL(
            access.bytes(Tls13KeyScheduleTestAccess::Secret::ServerHandshake),
            bytesFromHex("b67b7d690cc16c4e75e54213cb2d37b4e9c912bcded9105d42befd59d391ad38"));
        REQUIRE_EQUAL(
            access.bytes(Tls13KeyScheduleTestAccess::Secret::Master),
            bytesFromHex("18df06843d13a08bf2a449844c5f8a478001bc4d4c627984d5a41da8d0402919"));

        const auto serverFinishedTranscriptHash =
            bytesFromHex("9608102a0f1ccc6db6250b7b7e417b1a000eaada3daae4777a7686c9ff83df13");
        schedule.initializeApplication(serverFinishedTranscriptHash.span());
        REQUIRE_EQUAL(
            access.bytes(Tls13KeyScheduleTestAccess::Secret::ClientApplication),
            bytesFromHex("9e40646ce79a7f9dc05af8889bce6552875afa0b06df0087f792ebb7c17504a5"));
        REQUIRE_EQUAL(
            access.bytes(Tls13KeyScheduleTestAccess::Secret::ServerApplication),
            bytesFromHex("a11af9f05531f856ad47116b45a950328204b4f44bfb6b3a4b4f1f3fcb631643"));
        REQUIRE_EQUAL(
            access.bytes(Tls13KeyScheduleTestAccess::Secret::ExporterMaster),
            bytesFromHex("fe22f881176eda18eb8f44529e6792c50c9a3f89452f68d8ae311b4309d3cf50"));

        const auto clientFinishedTranscriptHash =
            bytesFromHex("209145a96ee8e2a122ff810047cc952684658d6049e86429426db87c54ad143d");
        schedule.discardResumptionMaster(clientFinishedTranscriptHash.span());
        REQUIRE(access.bytes(Tls13KeyScheduleTestAccess::Secret::Master).isEmpty());
        REQUIRE(access.bytes(Tls13KeyScheduleTestAccess::Secret::ClientHandshake).isEmpty());
        REQUIRE_FALSE(access.bytes(Tls13KeyScheduleTestAccess::Secret::ExporterMaster).isEmpty());

        schedule.secureErase();
        REQUIRE(schedule.isEmpty());
        REQUIRE(access.bytes(Tls13KeyScheduleTestAccess::Secret::ExporterMaster).isEmpty());
    }

    void testIndependentSha384FinishedAndExporter() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto clientPrivateBytes =
            bytesFromHex("49af42ba7f7994852d713ef2784bcbcaa7911de26adc5642cb634540e7ea5005");
        const auto serverPublicBytes = bytesFromHex("c9828876112095fe66762bdbf7c672e156d6cc253b833df1dd69b1b04e751f0f");
        auto clientPrivate =
            KeyAgreementPrivateKey::fromBytes(KeyAgreementAlgorithm::X25519, clientPrivateBytes.span());
        const auto serverPublic = KeyAgreementPublicKey{KeyAgreementAlgorithm::X25519, serverPublicBytes.span()};
        auto schedule = Tls13KeySchedule{HashAlgorithm::Sha2_384};
        const auto access = Tls13KeyScheduleTestAccess{schedule};

        // Independently generated with Python hashlib/hmac from the RFC 8448 section 3 X25519 shared secret and fixed
        // synthetic transcript hashes. This exercises the SHA-384 path that RFC 8448 does not cover.
        REQUIRE_EQUAL(
            access.bytes(Tls13KeyScheduleTestAccess::Secret::Early),
            bytesFromHex(
                "7ee8206f5570023e6dc7519eb1073bc4e791ad37b5c382aa10ba18e2357e7169"
                "71f9362f2c2fe2a76bfd78dfec4ea9b5"));
        const auto helloTranscriptHash = bytesFromHex(
            "92a8a9bc44257ec2bf163d03ff29df8b92d3bff552a955cd8162582b26e5688d"
            "490605bebe08073086b86b640d77dd06");
        schedule.initializeHandshake(clientPrivate.agree(serverPublic), helloTranscriptHash.span());
        REQUIRE_EQUAL(
            access.bytes(Tls13KeyScheduleTestAccess::Secret::ClientHandshake),
            bytesFromHex(
                "4823d2c1ec9017c839e0433e06c2ca96f0e45131fad01f533264e7e66597b5a0"
                "f09b90aefaeb2094b5e4a672f267c161"));
        REQUIRE_EQUAL(
            access.bytes(Tls13KeyScheduleTestAccess::Secret::ServerHandshake),
            bytesFromHex(
                "d3d336fea1ba2f06663ffe558523ee23066b5d7ce1273573d72907adc8365fedf"
                "ed162898980524df311a4f998500642"));
        REQUIRE_EQUAL(
            access.bytes(Tls13KeyScheduleTestAccess::Secret::Master),
            bytesFromHex(
                "2915f95014de3957dad1c2764430fa490ffbe027a09be69e4da30a27969b4008"
                "1308dbd17cb65a35332215cfc8cf4a2f"));

        const auto beforeFinishedHash = bytesFromHex(
            "a400a0ae6764cb6de18ad9b325fb5624161bda94af0b90f061f140defafd0d1a"
            "eec7d181e582cfae8cf683e36a8bceb4");
        const auto finished = schedule.finishedVerifyData(false, beforeFinishedHash.span());
        REQUIRE_EQUAL(
            finished,
            bytesFromHex(
                "31f7be9a0f2b02a1a0111bf3c2f81e5d6348d21ffc7f52bf6fbca053b0061ff"
                "c7fec7a7d2dec9440c4f011f744a3b0ec"));
        REQUIRE(schedule.verifyFinished(false, beforeFinishedHash.span(), finished.span()));

        const auto serverFinishedTranscriptHash = bytesFromHex(
            "2493f613f3e72756531c1f72d8bebc4b0aaa5d7b11c7dfc3f6e36b852fac53c2"
            "58d63017d170fea0019edd913d737a35");
        schedule.initializeApplication(serverFinishedTranscriptHash.span());
        REQUIRE_EQUAL(
            access.bytes(Tls13KeyScheduleTestAccess::Secret::ClientApplication),
            bytesFromHex(
                "1084b75843ee66221c1448b99fc00041ed531233104104b855887cbc7426d8f4e"
                "c3bffb8be6ad6877afcd4c547b12fac"));
        REQUIRE_EQUAL(
            access.bytes(Tls13KeyScheduleTestAccess::Secret::ServerApplication),
            bytesFromHex(
                "4a5fbaabe63124a5b9e6afd48096d4f7f641be26947e60b29b22f593dddb00f8"
                "d3b9c2e8d2d4ca1c656517d8ecb5422f"));
        REQUIRE_EQUAL(
            access.bytes(Tls13KeyScheduleTestAccess::Secret::ExporterMaster),
            bytesFromHex(
                "d65f1aa22c5ee581afb7053912405fe11efe44baa123f5eebfc90d65a3ab36367"
                "89196d8b5ba41e982cd688bda19a523"));

        const auto exporterLabel = ByteBlock({'t', 'e', 's', 't', ' ', 'e', 'x', 'p', 'o', 'r', 't', 'e', 'r'});
        const auto exporterContext = ByteBlock({'c', 'o', 'n', 't', 'e', 'x', 't'});
        REQUIRE_EQUAL(
            schedule.exportKey(exporterLabel.span(), exporterContext.span(), el::unit::ByteLength{42U}),
            bytesFromHex(
                "f1617adc5f0b78d29346f1854e6e3b735dce028a7a0fb54b0c321b9a313e5c54"
                "b2502a87f9f6a7e0deee"));
    }

    void testTranscriptFramingAndSnapshot() {
        auto transcript = Tls13Transcript{HashAlgorithm::Sha2_256};
        const auto first = bytesFromHex("01000003010203");
        const auto second = bytesFromHex("0200000104");
        transcript.update(first.span());

        auto direct = Hasher{HashAlgorithm::Sha2_256};
        direct.update(first);
        REQUIRE_EQUAL(transcript.hash(), direct.finalize());

        transcript.update(second.span());
        direct.reset();
        direct.update(first);
        direct.update(second);
        REQUIRE_EQUAL(transcript.hash(), direct.finalize());
        REQUIRE_THROWS_AS(el::err::ParameterError, transcript.update(bytesFromHex("0100000201").span()));
        transcript.secureErase();
        REQUIRE_EQUAL(transcript.hash(), Hasher{HashAlgorithm::Sha2_256}.finalize());
    }
};
