// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/cryptology/keys/SigningPrivateKey.hpp>
#include <erbsland/cryptology/SignedByteBlock.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(SignedByteBlock SigningKeyProfile)
class SignedByteBlockTest final : public el::UnitTest {
public:
    void testEd25519GenerationAndRoundTrip() {
        const auto scope = ApplicationTestScope<>{};
        const auto key = SigningPrivateKey::generate(SigningKeyProfile::Ed25519);
        REQUIRE_EQUAL(key.algorithm(), SigningKeyAlgorithm::Ed25519);
        const auto payload = el::mem::ByteBlock{el::mem::Byte{0U}, el::mem::Byte{1U}, el::mem::Byte{255U}};
        const auto signedBlock = SignedByteBlock::sign(key, payload.span(), "example.test/v1"_el, "device_1"_el);
        REQUIRE(signedBlock.untrustedKeyIdHint().has_value());
        REQUIRE_EQUAL(*signedBlock.untrustedKeyIdHint(), "device_1"_el);
        const auto encoded = signedBlock.toByteBlock();
        const auto parsed = SignedByteBlock::fromByteBlockOrThrow(encoded.span(), el::unit::ByteLength{3U});
        const auto verified = parsed.verify(key.publicKey(), "example.test/v1"_el);
        REQUIRE(verified.has_value());
        REQUIRE_EQUAL(*verified, payload);
        REQUIRE_EQUAL(parsed.toByteBlock(), encoded);
        REQUIRE_FALSE(parsed.verify(key.publicKey(), "example.test/v2"_el).has_value());
        const auto otherKey = SigningPrivateKey::generate(SigningKeyProfile::Ed25519);
        REQUIRE_FALSE(parsed.verify(otherKey.publicKey(), "example.test/v1"_el).has_value());
        REQUIRE(SignedByteBlock::fromByteBlock(encoded.span(), el::unit::ByteLength{2U}).isEmpty());
        REQUIRE(SignedByteBlock::fromByteBlock(encoded.span(), el::unit::ByteLength::infinite()).isEmpty());
        REQUIRE_THROWS(SignedByteBlock::sign(key, payload.span(), ""_el));
        REQUIRE_THROWS(SignedByteBlock::sign(key, payload.span(), "example.test/v1"_el, "bad-id"_el));
    }

    void testTamperingAndMalformedInput() {
        const auto scope = ApplicationTestScope<>{};
        const auto key = SigningPrivateKey::generate(SigningKeyProfile::Ed25519);
        const auto payload = el::mem::ByteBlock{el::mem::Byte{1U}, el::mem::Byte{2U}};
        const auto encoded = SignedByteBlock::sign(key, payload.span(), "purpose/v1"_el, "key_1"_el).toByteBlock();
        for (const auto offset : {0U, 4U, 5U, 6U, 7U, 8U}) {
            auto modified = el::mem::ByteBlockEditor{encoded};
            modified.xorAt(el::unit::ByteIndex{offset}, el::mem::Byte{1U});
            REQUIRE(SignedByteBlock::fromByteBlock(modified.span(), el::unit::ByteLength{64U}).isEmpty());
        }
        for (const auto offset : std::array<std::size_t, 4U>{12U, 17U, 27U, encoded.length().toSizeT() - 1U}) {
            auto modified = el::mem::ByteBlockEditor{encoded};
            modified.xorAt(el::unit::ByteIndex{offset}, el::mem::Byte{1U});
            const auto parsed = SignedByteBlock::fromByteBlock(modified.span(), el::unit::ByteLength{64U});
            if (!parsed.isEmpty()) {
                REQUIRE_FALSE(parsed.verify(key.publicKey(), "purpose/v1"_el).has_value());
            }
        }
        REQUIRE(
            SignedByteBlock::fromByteBlock(
                encoded.slice(el::unit::ByteIndex{0U}, el::unit::ByteLength{10U}).span(), el::unit::ByteLength{64U})
                .isEmpty());
        auto trailing = el::mem::ByteBlockEditor{encoded};
        trailing.append(el::mem::Byte{0U});
        REQUIRE(SignedByteBlock::fromByteBlock(trailing.span(), el::unit::ByteLength{64U}).isEmpty());
        const auto oversized = el::mem::ByteBlock{el::unit::ByteLength{500U}, el::mem::Byte{0U}};
        REQUIRE(SignedByteBlock::fromByteBlock(oversized.span(), el::unit::ByteLength{2U}).isEmpty());
    }
};
