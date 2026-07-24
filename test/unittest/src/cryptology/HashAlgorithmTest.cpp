// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/cryptology/StdFormat.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(HashAlgorithm HashRequirements CryptographicStatus CryptographicSecurity HashThroughput)
class HashAlgorithmTest final : public el::UnitTest {
public:
    void testMetadata() {
        for (const auto algorithm : {HashAlgorithm::Sha3_256, HashAlgorithm::Sha2_256}) {
            const auto hash = HashAlgorithm{algorithm};
            REQUIRE_EQUAL(hash.digestSize(), el::unit::ByteLength{32U});
            REQUIRE(hash.status() == CryptographicStatus::Acceptable);
            REQUIRE(hash.security() == CryptographicSecurity::Standard);
            REQUIRE(hash.throughput() == HashThroughput::High);
            REQUIRE(hash.isSafe());
        }
        for (const auto algorithm : {HashAlgorithm::Sha3_384, HashAlgorithm::Sha2_384}) {
            const auto hash = HashAlgorithm{algorithm};
            REQUIRE_EQUAL(hash.digestSize(), el::unit::ByteLength{48U});
            REQUIRE(hash.status() == CryptographicStatus::Acceptable);
            REQUIRE(hash.security() == CryptographicSecurity::High);
            REQUIRE(hash.throughput() == HashThroughput::Medium);
            REQUIRE(hash.isSafe());
        }
        for (const auto algorithm : {HashAlgorithm::Sha3_512, HashAlgorithm::Sha2_512}) {
            const auto hash = HashAlgorithm{algorithm};
            REQUIRE_EQUAL(hash.digestSize(), el::unit::ByteLength{64U});
            REQUIRE(hash.status() == CryptographicStatus::Acceptable);
            REQUIRE(hash.security() == CryptographicSecurity::High);
            REQUIRE(hash.throughput() == HashThroughput::Low);
            REQUIRE(hash.isSafe());
        }
        REQUIRE_EQUAL(HashAlgorithm{HashAlgorithm::Sha1}.digestSize(), el::unit::ByteLength{20U});
        REQUIRE_EQUAL(HashAlgorithm{HashAlgorithm::Md5}.digestSize(), el::unit::ByteLength{16U});
        REQUIRE(HashAlgorithm{HashAlgorithm::Sha1}.status() == CryptographicStatus::Disallowed);
        REQUIRE(HashAlgorithm{HashAlgorithm::Md5}.status() == CryptographicStatus::Disallowed);
        REQUIRE_FALSE(HashAlgorithm{HashAlgorithm::Sha1}.isSafe());
        REQUIRE_FALSE(HashAlgorithm{HashAlgorithm::Md5}.isSafe());
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
        REQUIRE(HashAlgorithm::fromString("sha3-256"_el) == HashAlgorithm::Sha3_256);
        REQUIRE(HashAlgorithm::fromString("sha3-384"_el) == HashAlgorithm::Sha3_384);
        REQUIRE(HashAlgorithm::fromString("sha3-512"_el) == HashAlgorithm::Sha3_512);
        REQUIRE(HashAlgorithm::fromString("sha-256"_el) == HashAlgorithm::Sha2_256);
        REQUIRE(HashAlgorithm::fromString("sha-384"_el) == HashAlgorithm::Sha2_384);
        REQUIRE(HashAlgorithm::fromString("sha-512"_el) == HashAlgorithm::Sha2_512);
        REQUIRE(HashAlgorithm::fromString("sha-1"_el) == HashAlgorithm::Sha1);
        REQUIRE(HashAlgorithm::fromString("md5"_el) == HashAlgorithm::Md5);
        REQUIRE_FALSE(HashAlgorithm::fromString("SHA3-256"_el).has_value());
        REQUIRE_FALSE(HashAlgorithm::fromString("unknown"_el).has_value());
        REQUIRE_EQUAL(HashAlgorithm::fromStringOrThrow("sha3-256"_el), HashAlgorithm::Sha3_256);
        REQUIRE_THROWS_AS(el::err::ParseError, HashAlgorithm::fromStringOrThrow("unknown"_el));
    }

    void testEnumeration() {
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

        const auto safe = HashAlgorithm::allSafe();
        REQUIRE_EQUAL(safe.size(), 6U);
        REQUIRE_EQUAL(safe[0], HashAlgorithm::Sha3_256);
        REQUIRE_EQUAL(safe[1], HashAlgorithm::Sha3_384);
        REQUIRE_EQUAL(safe[2], HashAlgorithm::Sha3_512);
        REQUIRE_EQUAL(safe[3], HashAlgorithm::Sha2_256);
        REQUIRE_EQUAL(safe[4], HashAlgorithm::Sha2_384);
        REQUIRE_EQUAL(safe[5], HashAlgorithm::Sha2_512);
    }

    void testRequirements() {
        const auto defaults = HashRequirements{};
        REQUIRE(HashAlgorithm{HashAlgorithm::Sha3_256}.matches(defaults));
        REQUIRE(HashAlgorithm{HashAlgorithm::Sha3_384}.matches(defaults));
        REQUIRE(HashAlgorithm{HashAlgorithm::Sha3_512}.matches(defaults));
        REQUIRE(HashAlgorithm{HashAlgorithm::Sha2_256}.matches(defaults));
        REQUIRE(HashAlgorithm{HashAlgorithm::Sha2_384}.matches(defaults));
        REQUIRE(HashAlgorithm{HashAlgorithm::Sha2_512}.matches(defaults));
        REQUIRE_FALSE(HashAlgorithm{HashAlgorithm::Sha1}.matches(defaults));
        REQUIRE_FALSE(HashAlgorithm{HashAlgorithm::Md5}.matches(defaults));
        REQUIRE(HashAlgorithm::recommended(defaults) == HashAlgorithm::Sha3_256);

        const auto highSecurity = HashRequirements{
            .minimumSecurity = CryptographicSecurity::High,
            .minimumThroughput = HashThroughput::Low,
        };
        const auto highMatches = HashAlgorithm::matching(highSecurity).toStdVector();
        REQUIRE_EQUAL(highMatches.size(), 4U);
        REQUIRE_EQUAL(highMatches[0], HashAlgorithm::Sha3_384);
        REQUIRE_EQUAL(highMatches[1], HashAlgorithm::Sha3_512);
        REQUIRE_EQUAL(highMatches[2], HashAlgorithm::Sha2_384);
        REQUIRE_EQUAL(highMatches[3], HashAlgorithm::Sha2_512);
        REQUIRE(HashAlgorithm::recommended(highSecurity) == HashAlgorithm::Sha3_384);

        const auto legacyOnly = HashRequirements{
            .requiredStatus = CryptographicStatus::Legacy,
        };
        REQUIRE(HashAlgorithm::matching(legacyOnly).isEmpty());
        REQUIRE_FALSE(HashAlgorithm::recommended(legacyOnly).has_value());

        const auto disallowed = HashRequirements{
            .requiredStatus = CryptographicStatus::Disallowed,
        };
        const auto disallowedMatches = HashAlgorithm::matching(disallowed).toStdVector();
        REQUIRE_EQUAL(disallowedMatches.size(), 2U);
        REQUIRE_EQUAL(disallowedMatches[0], HashAlgorithm::Sha1);
        REQUIRE_EQUAL(disallowedMatches[1], HashAlgorithm::Md5);

        const auto impossible = HashRequirements{
            .minimumSecurity = CryptographicSecurity::High,
            .minimumThroughput = HashThroughput::High,
        };
        REQUIRE(HashAlgorithm::matching(impossible).isEmpty());
        REQUIRE_FALSE(HashAlgorithm::recommended(impossible).has_value());
    }
};
