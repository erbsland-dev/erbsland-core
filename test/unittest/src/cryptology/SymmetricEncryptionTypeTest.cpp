// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/StdFormat.hpp>
#include <erbsland/cryptology/symmetric/SymmetricEncryptionRequirements.hpp>
#include <erbsland/cryptology/symmetric/SymmetricEncryptionSelector.hpp>
#include <erbsland/cryptology/symmetric/SymmetricEncryptionType.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>
#include <optional>

using namespace el::cryptology;
using namespace el::text::literals;
using el::unit::ByteLength;

TESTED_TARGETS(SymmetricCipher SymmetricEncryptionRequirements SymmetricEncryptionSelector SymmetricEncryptionType)
class SymmetricEncryptionTypeTest final : public el::UnitTest {
public:
    void testInvalidType() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto selector = SymmetricEncryptionSelector{};
        const auto type = SymmetricEncryptionType{};
        REQUIRE_FALSE(type.isValid());
        REQUIRE_EQUAL(type.toRawValue(), SymmetricEncryptionType::None);
        REQUIRE_EQUAL(type.cipher(), SymmetricCipher::None);
        REQUIRE(type.keyLength().isZero());
        REQUIRE_EQUAL(type.keyBitCount(), 0U);
        REQUIRE(type.nonceLength().isZero());
        REQUIRE(type.tagLength().isZero());
        REQUIRE(type.ivLength().isZero());
        REQUIRE(type.maximumEncryptedLength(ByteLength{123U}).isZero());
        REQUIRE_EQUAL(selector.status(type), CryptographicStatus::Disallowed);
        REQUIRE_EQUAL(type.security(), CryptographicSecurity::Standard);
        REQUIRE_FALSE(type.isAead());
        REQUIRE_FALSE(type.requiresIv());
        REQUIRE_FALSE(selector.matches(type));
        REQUIRE(type.toString().isEmpty());
    }

    void testAeadMetadata() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto selector = SymmetricEncryptionSelector{};
        const auto aes256 = SymmetricEncryptionType{SymmetricEncryptionType::Aes256Gcm};
        REQUIRE_EQUAL(aes256.cipher(), SymmetricCipher::Aes);
        REQUIRE_EQUAL(aes256.keyLength(), ByteLength{32U});
        REQUIRE_EQUAL(aes256.keyBitCount(), 256U);
        REQUIRE_EQUAL(aes256.nonceLength(), ByteLength{12U});
        REQUIRE_EQUAL(aes256.tagLength(), ByteLength{16U});
        REQUIRE(aes256.ivLength().isZero());
        REQUIRE(aes256.isAead());
        REQUIRE_FALSE(aes256.requiresIv());
        REQUIRE_EQUAL(selector.status(aes256), CryptographicStatus::Acceptable);
        REQUIRE_EQUAL(aes256.security(), CryptographicSecurity::High);
        REQUIRE_EQUAL(aes256.maximumEncryptedLength(ByteLength{31U}), ByteLength{31U});

        const auto chacha = SymmetricEncryptionType{SymmetricEncryptionType::ChaCha20Poly1305};
        REQUIRE_EQUAL(chacha.cipher(), SymmetricCipher::ChaCha20);
        REQUIRE_EQUAL(chacha.keyLength(), ByteLength{32U});
        REQUIRE_EQUAL(chacha.security(), CryptographicSecurity::High);

        const auto aes128 = SymmetricEncryptionType{SymmetricEncryptionType::Aes128Gcm};
        REQUIRE_EQUAL(aes128.cipher(), SymmetricCipher::Aes);
        REQUIRE_EQUAL(aes128.keyLength(), ByteLength{16U});
        REQUIRE_EQUAL(aes128.keyBitCount(), 128U);
        REQUIRE_EQUAL(aes128.security(), CryptographicSecurity::Standard);
    }

    void testLegacyCbcMetadataAndGrowth() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto selector = SymmetricEncryptionSelector{};
        const auto randomFill = SymmetricEncryptionType{SymmetricEncryptionType::Aes256CbcRandomFill};
        const auto method2 = SymmetricEncryptionType{SymmetricEncryptionType::Aes256CbcIso9797Method2};
        for (const auto type : {randomFill, method2}) {
            REQUIRE_EQUAL(type.cipher(), SymmetricCipher::Aes);
            REQUIRE_EQUAL(type.keyLength(), ByteLength{32U});
            REQUIRE(type.nonceLength().isZero());
            REQUIRE(type.tagLength().isZero());
            REQUIRE_EQUAL(type.ivLength(), ByteLength{16U});
            REQUIRE_FALSE(type.isAead());
            REQUIRE(type.requiresIv());
            REQUIRE_EQUAL(selector.status(type), CryptographicStatus::Legacy);
            REQUIRE_EQUAL(type.security(), CryptographicSecurity::High);
        }

        REQUIRE(randomFill.maximumEncryptedLength(ByteLength::zero()).isZero());
        REQUIRE_EQUAL(randomFill.maximumEncryptedLength(ByteLength{1U}), ByteLength{16U});
        REQUIRE_EQUAL(randomFill.maximumEncryptedLength(ByteLength{15U}), ByteLength{16U});
        REQUIRE_EQUAL(randomFill.maximumEncryptedLength(ByteLength{16U}), ByteLength{16U});
        REQUIRE_EQUAL(randomFill.maximumEncryptedLength(ByteLength{17U}), ByteLength{32U});

        REQUIRE_EQUAL(method2.maximumEncryptedLength(ByteLength::zero()), ByteLength{16U});
        REQUIRE_EQUAL(method2.maximumEncryptedLength(ByteLength{1U}), ByteLength{16U});
        REQUIRE_EQUAL(method2.maximumEncryptedLength(ByteLength{15U}), ByteLength{16U});
        REQUIRE_EQUAL(method2.maximumEncryptedLength(ByteLength{16U}), ByteLength{32U});
        REQUIRE_EQUAL(method2.maximumEncryptedLength(ByteLength{17U}), ByteLength{32U});
        REQUIRE(randomFill.maximumEncryptedLength(ByteLength::infinite()).isInfinite());
        REQUIRE(method2.maximumEncryptedLength(ByteLength::maximum()).isMaximum());
    }

    void testConversionAndEnumeration() {
        const auto applicationScope = ApplicationTestScope<>{};
        REQUIRE_EQUAL(SymmetricEncryptionType{SymmetricEncryptionType::Aes256Gcm}.toString(), "aes-256-gcm"_el);
        REQUIRE_EQUAL(
            SymmetricEncryptionType{SymmetricEncryptionType::ChaCha20Poly1305}.toString(), "chacha20-poly1305"_el);
        REQUIRE_EQUAL(SymmetricEncryptionType{SymmetricEncryptionType::Aes128Gcm}.toString(), "aes-128-gcm"_el);
        REQUIRE_EQUAL(
            SymmetricEncryptionType{SymmetricEncryptionType::Aes256CbcRandomFill}.toString(),
            "aes-256-cbc-random-fill"_el);
        REQUIRE_EQUAL(
            SymmetricEncryptionType{SymmetricEncryptionType::Aes256CbcIso9797Method2}.toString(),
            "aes-256-cbc-iso9797-method2"_el);
        REQUIRE_EQUAL(std::format("{}", SymmetricEncryptionType{SymmetricEncryptionType::Aes256Gcm}), "aes-256-gcm");

        for (const auto type : SymmetricEncryptionType::all()) {
            REQUIRE_EQUAL(SymmetricEncryptionType::fromString(type.toString()), type);
            REQUIRE_EQUAL(SymmetricEncryptionType::fromStringOrThrow(type.toString()), type);
        }
        REQUIRE_FALSE(SymmetricEncryptionType::fromString("aes256-cbc"_el).has_value());
        REQUIRE_FALSE(SymmetricEncryptionType::fromString("AES-256-GCM"_el).has_value());
        REQUIRE_THROWS_AS(el::err::ParseError, SymmetricEncryptionType::fromStringOrThrow("unsupported"_el));

        const auto all = SymmetricEncryptionType::all();
        REQUIRE_EQUAL(all.size(), 5U);
        REQUIRE_EQUAL(all[0], SymmetricEncryptionType::Aes256Gcm);
        REQUIRE_EQUAL(all[1], SymmetricEncryptionType::ChaCha20Poly1305);
        REQUIRE_EQUAL(all[2], SymmetricEncryptionType::Aes128Gcm);
        REQUIRE_EQUAL(all[3], SymmetricEncryptionType::Aes256CbcRandomFill);
        REQUIRE_EQUAL(all[4], SymmetricEncryptionType::Aes256CbcIso9797Method2);

        const auto accepted = SymmetricEncryptionSelector{}.allAccepted().toStdVector();
        REQUIRE_EQUAL(accepted.size(), 3U);
        REQUIRE_EQUAL(accepted[0], SymmetricEncryptionType::Aes256Gcm);
        REQUIRE_EQUAL(accepted[1], SymmetricEncryptionType::ChaCha20Poly1305);
        REQUIRE_EQUAL(accepted[2], SymmetricEncryptionType::Aes128Gcm);
    }

    void testRequirementsAndRecommendation() {
        const auto applicationScope = ApplicationTestScope<>{};
        REQUIRE_EQUAL(SymmetricEncryptionSelector{}.recommended(), SymmetricEncryptionType::Aes256Gcm);

        const auto standardAes = SymmetricEncryptionRequirements{
            .minimumSecurity = CryptographicSecurity::Standard,
            .requiredCipher = SymmetricCipher::Aes,
        };
        const auto aesMatches = SymmetricEncryptionSelector{standardAes}.matching().toStdVector();
        REQUIRE_EQUAL(aesMatches.size(), 2U);
        REQUIRE_EQUAL(aesMatches[0], SymmetricEncryptionType::Aes256Gcm);
        REQUIRE_EQUAL(aesMatches[1], SymmetricEncryptionType::Aes128Gcm);

        const auto chacha = SymmetricEncryptionRequirements{
            .minimumSecurity = CryptographicSecurity::High,
            .requiredCipher = SymmetricCipher::ChaCha20,
        };
        REQUIRE_EQUAL(SymmetricEncryptionSelector{chacha}.recommended(), SymmetricEncryptionType::ChaCha20Poly1305);

        const auto legacy = SymmetricEncryptionRequirements{
            .requiredStatus = CryptographicStatus::Legacy,
            .minimumSecurity = CryptographicSecurity::High,
            .requireAead = false,
        };
        const auto legacyMatches = SymmetricEncryptionSelector{legacy}.matching().toStdVector();
        REQUIRE_EQUAL(legacyMatches.size(), 2U);
        REQUIRE_EQUAL(legacyMatches[0], SymmetricEncryptionType::Aes256CbcRandomFill);
        REQUIRE_EQUAL(legacyMatches[1], SymmetricEncryptionType::Aes256CbcIso9797Method2);

        const auto impossible = SymmetricEncryptionRequirements{
            .requiredStatus = CryptographicStatus::Legacy,
            .requireAead = true,
        };
        REQUIRE(SymmetricEncryptionSelector{impossible}.matching().isEmpty());
        REQUIRE_FALSE(SymmetricEncryptionSelector{impossible}.recommended().has_value());
    }
};
