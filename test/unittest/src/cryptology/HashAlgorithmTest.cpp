// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/cryptology/HashSelector.hpp>
#include <erbsland/cryptology/StdFormat.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(HashAlgorithm HashSelector HashRequirements CryptographicStatus CryptographicSecurity HashThroughput)
class HashAlgorithmTest final : public el::UnitTest {
public:
    void testMetadata() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto selector = HashSelector{};
        for (const auto algorithm : {HashAlgorithm::Sha3_256, HashAlgorithm::Sha2_256}) {
            const auto hash = HashAlgorithm{algorithm};
            REQUIRE_EQUAL(hash.digestSize(), el::unit::ByteLength{32U});
            REQUIRE_EQUAL(selector.status(hash), CryptographicStatus::Acceptable);
            REQUIRE_EQUAL(hash.security(), CryptographicSecurity::Standard);
            REQUIRE_EQUAL(hash.throughput(), HashThroughput::High);
            REQUIRE(selector.isSafe(hash));
        }
        for (const auto algorithm : {HashAlgorithm::Sha3_384, HashAlgorithm::Sha2_384}) {
            const auto hash = HashAlgorithm{algorithm};
            REQUIRE_EQUAL(hash.digestSize(), el::unit::ByteLength{48U});
            REQUIRE_EQUAL(selector.status(hash), CryptographicStatus::Acceptable);
            REQUIRE_EQUAL(hash.security(), CryptographicSecurity::High);
            REQUIRE_EQUAL(hash.throughput(), HashThroughput::Medium);
            REQUIRE(selector.isSafe(hash));
        }
        for (const auto algorithm : {HashAlgorithm::Sha3_512, HashAlgorithm::Sha2_512}) {
            const auto hash = HashAlgorithm{algorithm};
            REQUIRE_EQUAL(hash.digestSize(), el::unit::ByteLength{64U});
            REQUIRE_EQUAL(selector.status(hash), CryptographicStatus::Acceptable);
            REQUIRE_EQUAL(hash.security(), CryptographicSecurity::High);
            REQUIRE_EQUAL(hash.throughput(), HashThroughput::Low);
            REQUIRE(selector.isSafe(hash));
        }
        REQUIRE_EQUAL(HashAlgorithm{HashAlgorithm::Sha1}.digestSize(), el::unit::ByteLength{20U});
        REQUIRE_EQUAL(HashAlgorithm{HashAlgorithm::Md5}.digestSize(), el::unit::ByteLength{16U});
        const auto sha1 = HashAlgorithm{HashAlgorithm::Sha1};
        const auto md5 = HashAlgorithm{HashAlgorithm::Md5};
        REQUIRE_EQUAL(selector.status(sha1), CryptographicStatus::Disallowed);
        REQUIRE_EQUAL(selector.status(md5), CryptographicStatus::Disallowed);
        REQUIRE_FALSE(selector.isSafe(sha1));
        REQUIRE_FALSE(selector.isSafe(md5));
    }

    void testConversion() {
        REQUIRE_EQUAL(HashAlgorithm{HashAlgorithm::Sha3_256}.toString(), "sha3-256"_el);
        REQUIRE_EQUAL(HashAlgorithm{HashAlgorithm::Sha3_384}.toString(), "sha3-384"_el);
        REQUIRE_EQUAL(HashAlgorithm{HashAlgorithm::Sha3_512}.toString(), "sha3-512"_el);
        REQUIRE_EQUAL(HashAlgorithm{HashAlgorithm::Sha2_256}.toString(), "sha-256"_el);
        REQUIRE_EQUAL(HashAlgorithm{HashAlgorithm::Sha2_384}.toString(), "sha-384"_el);
        REQUIRE_EQUAL(HashAlgorithm{HashAlgorithm::Sha2_512}.toString(), "sha-512"_el);
        REQUIRE_EQUAL(HashAlgorithm{HashAlgorithm::Sha1}.toString(), "sha-1"_el);
        REQUIRE_EQUAL(HashAlgorithm{HashAlgorithm::Md5}.toString(), "md5"_el);
        const auto sha3_256 = HashAlgorithm::fromString("sha3-256"_el);
        const auto sha3_384 = HashAlgorithm::fromString("sha3-384"_el);
        const auto sha3_512 = HashAlgorithm::fromString("sha3-512"_el);
        const auto sha2_256 = HashAlgorithm::fromString("sha-256"_el);
        const auto sha2_384 = HashAlgorithm::fromString("sha-384"_el);
        const auto sha2_512 = HashAlgorithm::fromString("sha-512"_el);
        const auto parsedSha1 = HashAlgorithm::fromString("sha-1"_el);
        const auto parsedMd5 = HashAlgorithm::fromString("md5"_el);
        REQUIRE_EQUAL(sha3_256, HashAlgorithm::Sha3_256);
        REQUIRE_EQUAL(sha3_384, HashAlgorithm::Sha3_384);
        REQUIRE_EQUAL(sha3_512, HashAlgorithm::Sha3_512);
        REQUIRE_EQUAL(sha2_256, HashAlgorithm::Sha2_256);
        REQUIRE_EQUAL(sha2_384, HashAlgorithm::Sha2_384);
        REQUIRE_EQUAL(sha2_512, HashAlgorithm::Sha2_512);
        REQUIRE_EQUAL(parsedSha1, HashAlgorithm::Sha1);
        REQUIRE_EQUAL(parsedMd5, HashAlgorithm::Md5);
        REQUIRE_FALSE(HashAlgorithm::fromString("SHA3-256"_el).has_value());
        REQUIRE_FALSE(HashAlgorithm::fromString("unknown"_el).has_value());
        REQUIRE_EQUAL(HashAlgorithm::fromStringOrThrow("sha3-256"_el), HashAlgorithm::Sha3_256);
        REQUIRE_THROWS_AS(el::err::ParseError, HashAlgorithm::fromStringOrThrow("unknown"_el));
    }

    void testEnumeration() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto all = HashAlgorithm::all();
        REQUIRE_EQUAL(all.size(), 8U);
        REQUIRE_EQUAL(all[0], HashAlgorithm::Sha3_256);
        REQUIRE_EQUAL(all[1], HashAlgorithm::Sha3_384);
        REQUIRE_EQUAL(all[2], HashAlgorithm::Sha3_512);
        REQUIRE_EQUAL(all[3], HashAlgorithm::Sha2_256);
        REQUIRE_EQUAL(all[4], HashAlgorithm::Sha2_384);
        REQUIRE_EQUAL(all[5], HashAlgorithm::Sha2_512);
        REQUIRE_EQUAL(all[6], HashAlgorithm::Sha1);
        REQUIRE_EQUAL(all[7], HashAlgorithm::Md5);

        const auto accepted = HashSelector{}.allAccepted().toStdVector();
        REQUIRE_EQUAL(accepted.size(), 6U);
        REQUIRE_EQUAL(accepted[0], HashAlgorithm::Sha3_256);
        REQUIRE_EQUAL(accepted[1], HashAlgorithm::Sha3_384);
        REQUIRE_EQUAL(accepted[2], HashAlgorithm::Sha3_512);
        REQUIRE_EQUAL(accepted[3], HashAlgorithm::Sha2_256);
        REQUIRE_EQUAL(accepted[4], HashAlgorithm::Sha2_384);
        REQUIRE_EQUAL(accepted[5], HashAlgorithm::Sha2_512);
    }

    void testRequirements() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto defaults = HashRequirements{};
        const auto defaultSelector = HashSelector{defaults};
        REQUIRE(defaultSelector.matches(HashAlgorithm::Sha3_256));
        REQUIRE(defaultSelector.matches(HashAlgorithm::Sha3_384));
        REQUIRE(defaultSelector.matches(HashAlgorithm::Sha3_512));
        REQUIRE(defaultSelector.matches(HashAlgorithm::Sha2_256));
        REQUIRE(defaultSelector.matches(HashAlgorithm::Sha2_384));
        REQUIRE(defaultSelector.matches(HashAlgorithm::Sha2_512));
        REQUIRE_FALSE(defaultSelector.matches(HashAlgorithm::Sha1));
        REQUIRE_FALSE(defaultSelector.matches(HashAlgorithm::Md5));
        const auto recommendedDefault = defaultSelector.recommended();
        REQUIRE_EQUAL(recommendedDefault, HashAlgorithm::Sha3_256);

        const auto highSecurity = HashRequirements{
            .minimumSecurity = CryptographicSecurity::High,
            .minimumThroughput = HashThroughput::Low,
        };
        const auto highSelector = HashSelector{highSecurity};
        const auto highMatches = highSelector.matching().toStdVector();
        REQUIRE_EQUAL(highMatches.size(), 4U);
        REQUIRE_EQUAL(highMatches[0], HashAlgorithm::Sha3_384);
        REQUIRE_EQUAL(highMatches[1], HashAlgorithm::Sha3_512);
        REQUIRE_EQUAL(highMatches[2], HashAlgorithm::Sha2_384);
        REQUIRE_EQUAL(highMatches[3], HashAlgorithm::Sha2_512);
        const auto recommendedHighSecurity = highSelector.recommended();
        REQUIRE_EQUAL(recommendedHighSecurity, HashAlgorithm::Sha3_384);

        const auto legacyOnly = HashRequirements{
            .requiredStatus = CryptographicStatus::Legacy,
        };
        REQUIRE(HashSelector{legacyOnly}.matching().isEmpty());
        REQUIRE_FALSE(HashSelector{legacyOnly}.recommended().has_value());

        const auto disallowed = HashRequirements{
            .requiredStatus = CryptographicStatus::Disallowed,
        };
        const auto disallowedMatches = HashSelector{disallowed}.matching().toStdVector();
        REQUIRE_EQUAL(disallowedMatches.size(), 2U);
        REQUIRE_EQUAL(disallowedMatches[0], HashAlgorithm::Sha1);
        REQUIRE_EQUAL(disallowedMatches[1], HashAlgorithm::Md5);

        const auto impossible = HashRequirements{
            .minimumSecurity = CryptographicSecurity::High,
            .minimumThroughput = HashThroughput::High,
        };
        REQUIRE(HashSelector{impossible}.matching().isEmpty());
        REQUIRE_FALSE(HashSelector{impossible}.recommended().has_value());
    }
};
