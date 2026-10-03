// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/ByteBlockEditor.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/SignedByteBlock.hpp>
#include <erbsland/SigningPrivateKey.hpp>
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
        const auto parsed = SignedByteBlock::fromByteBlockOrThrow(encoded.span(), el::unit::ByteCount{3U});
        const auto verified = parsed.verify(key.publicKey(), "example.test/v1"_el);
        REQUIRE(verified.has_value());
        REQUIRE_EQUAL(*verified, payload);
        REQUIRE_EQUAL(parsed.toByteBlock(), encoded);
        REQUIRE_FALSE(parsed.verify(key.publicKey(), "example.test/v2"_el).has_value());
        const auto otherKey = SigningPrivateKey::generate(SigningKeyProfile::Ed25519);
        REQUIRE_FALSE(parsed.verify(otherKey.publicKey(), "example.test/v1"_el).has_value());
        REQUIRE(SignedByteBlock::fromByteBlock(encoded.span(), el::unit::ByteCount{2U}).isEmpty());
        REQUIRE_THROWS_AS(
            el::err::ParseError, SignedByteBlock::fromByteBlockOrThrow(encoded.span(), el::unit::ByteCount{2U}));
        REQUIRE(SignedByteBlock::fromByteBlock(encoded.span(), el::unit::ByteCount::infinite()).isEmpty());
        REQUIRE_THROWS(SignedByteBlock::sign(key, payload.span(), ""_el));
        REQUIRE_THROWS(SignedByteBlock::sign(key, payload.span(), "example.test/v1"_el, "bad-id"_el));
    }

    void testTamperingAndMalformedInput() {
        const auto scope = ApplicationTestScope<>{};
        const auto key = SigningPrivateKey::generate(SigningKeyProfile::Ed25519);
        const auto payload = el::mem::ByteBlock{el::mem::Byte{1U}, el::mem::Byte{2U}};
        const auto encoded = SignedByteBlock::sign(key, payload.span(), "purpose/v1"_el, "key_1"_el).toByteBlock();
        for (
            const auto offset :
            {el::unit::ByteIndex{0U},
                el::unit::ByteIndex{4U},
                el::unit::ByteIndex{5U},
                el::unit::ByteIndex{6U},
                el::unit::ByteIndex{7U},
                el::unit::ByteIndex{8U}}) {
            auto modified = el::mem::ByteBlockEditor{encoded};
            modified.xorAt(offset, el::mem::Byte{1U});
            REQUIRE(SignedByteBlock::fromByteBlock(modified.span(), el::unit::ByteCount{64U}).isEmpty());
        }
        for (
            const auto offset : std::array<el::unit::ByteIndex, 4U>{
                el::unit::ByteIndex{12U}, el::unit::ByteIndex{17U}, el::unit::ByteIndex{27U}, encoded.lastIndex()}) {
            auto modified = el::mem::ByteBlockEditor{encoded};
            modified.xorAt(offset, el::mem::Byte{1U});
            const auto parsed = SignedByteBlock::fromByteBlock(modified.span(), el::unit::ByteCount{64U});
            if (!parsed.isEmpty()) {
                REQUIRE_FALSE(parsed.verify(key.publicKey(), "purpose/v1"_el).has_value());
            }
        }
        REQUIRE(
            SignedByteBlock::fromByteBlock(
                encoded.slice(el::unit::ByteIndex{0U}, el::unit::ByteCount{10U}).span(), el::unit::ByteCount{64U})
                .isEmpty());
        auto invalidText = el::mem::ByteBlockEditor{encoded};
        invalidText.xorAt(el::unit::ByteIndex{12U}, el::mem::Byte{0xffU});
        REQUIRE(SignedByteBlock::fromByteBlock(invalidText.span(), el::unit::ByteCount{64U}).isEmpty());
        auto trailing = el::mem::ByteBlockEditor{encoded};
        trailing.append(el::mem::Byte{0U});
        REQUIRE(SignedByteBlock::fromByteBlock(trailing.span(), el::unit::ByteCount{64U}).isEmpty());
        const auto oversized = el::mem::ByteBlock{el::unit::ByteCount{500U}, el::mem::Byte{0U}};
        REQUIRE(SignedByteBlock::fromByteBlock(oversized.span(), el::unit::ByteCount{2U}).isEmpty());
        REQUIRE_THROWS_AS(
            el::err::ParseError, SignedByteBlock::fromByteBlockOrThrow(oversized.span(), el::unit::ByteCount{2U}));
    }
};
