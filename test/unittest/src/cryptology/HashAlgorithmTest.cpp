// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/cryptology/StdFormatForCryptology.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(HashAlgorithm HashRequirements CryptographicStatus CryptographicSecurity HashThroughput)
class HashAlgorithmTest final : public el::UnitTest {
public:
    void testMetadata() {
        const auto sha256 = HashAlgorithm{HashAlgorithm::Sha3_256};
        REQUIRE_EQUAL(sha256.digestSize(), el::unit::ByteLength{32U});
        REQUIRE(sha256.status() == CryptographicStatus::Acceptable);
        REQUIRE(sha256.security() == CryptographicSecurity::Standard);
        REQUIRE(sha256.throughput() == HashThroughput::High);
        REQUIRE(sha256.isSafe());

        const auto sha384 = HashAlgorithm{HashAlgorithm::Sha3_384};
        REQUIRE_EQUAL(sha384.digestSize(), el::unit::ByteLength{48U});
        REQUIRE(sha384.status() == CryptographicStatus::Acceptable);
        REQUIRE(sha384.security() == CryptographicSecurity::High);
        REQUIRE(sha384.throughput() == HashThroughput::Medium);
        REQUIRE(sha384.isSafe());

        const auto sha512 = HashAlgorithm{HashAlgorithm::Sha3_512};
        REQUIRE_EQUAL(sha512.digestSize(), el::unit::ByteLength{64U});
        REQUIRE(sha512.status() == CryptographicStatus::Acceptable);
        REQUIRE(sha512.security() == CryptographicSecurity::High);
        REQUIRE(sha512.throughput() == HashThroughput::Low);
        REQUIRE(sha512.isSafe());
    }

    void testConversion() {
        REQUIRE_EQUAL(HashAlgorithm{HashAlgorithm::Sha3_256}.toString(), "sha3-256"_el);
        REQUIRE_EQUAL(HashAlgorithm{HashAlgorithm::Sha3_384}.toString(), "sha3-384"_el);
        REQUIRE_EQUAL(HashAlgorithm{HashAlgorithm::Sha3_512}.toString(), "sha3-512"_el);
        REQUIRE(HashAlgorithm::fromString("sha3-256"_el) == HashAlgorithm::Sha3_256);
        REQUIRE(HashAlgorithm::fromString("sha3-384"_el) == HashAlgorithm::Sha3_384);
        REQUIRE(HashAlgorithm::fromString("sha3-512"_el) == HashAlgorithm::Sha3_512);
        REQUIRE_FALSE(HashAlgorithm::fromString("SHA3-256"_el).has_value());
        REQUIRE_FALSE(HashAlgorithm::fromString("unknown"_el).has_value());
        REQUIRE_EQUAL(HashAlgorithm::fromStringOrThrow("sha3-256"_el), HashAlgorithm::Sha3_256);
        REQUIRE_THROWS_AS(el::err::ParseError, HashAlgorithm::fromStringOrThrow("unknown"_el));
    }

    void testEnumeration() {
        const auto all = HashAlgorithm::all();
        REQUIRE_EQUAL(all.size(), 3U);
        REQUIRE_EQUAL(all[0], HashAlgorithm::Sha3_256);
        REQUIRE_EQUAL(all[1], HashAlgorithm::Sha3_384);
        REQUIRE_EQUAL(all[2], HashAlgorithm::Sha3_512);

        const auto safe = HashAlgorithm::allSafe();
        REQUIRE_EQUAL(safe.size(), 3U);
        REQUIRE_EQUAL(safe[0], HashAlgorithm::Sha3_256);
        REQUIRE_EQUAL(safe[1], HashAlgorithm::Sha3_384);
        REQUIRE_EQUAL(safe[2], HashAlgorithm::Sha3_512);
    }

    void testRequirements() {
        const auto defaults = HashRequirements{};
        REQUIRE(HashAlgorithm{HashAlgorithm::Sha3_256}.matches(defaults));
        REQUIRE(HashAlgorithm{HashAlgorithm::Sha3_384}.matches(defaults));
        REQUIRE(HashAlgorithm{HashAlgorithm::Sha3_512}.matches(defaults));
        REQUIRE(HashAlgorithm::recommended(defaults) == HashAlgorithm::Sha3_256);

        const auto highSecurity = HashRequirements{
            .minimumSecurity = CryptographicSecurity::High,
            .minimumThroughput = HashThroughput::Low,
        };
        const auto highMatches = HashAlgorithm::matching(highSecurity).toStdVector();
        REQUIRE_EQUAL(highMatches.size(), 2U);
        REQUIRE_EQUAL(highMatches[0], HashAlgorithm::Sha3_384);
        REQUIRE_EQUAL(highMatches[1], HashAlgorithm::Sha3_512);
        REQUIRE(HashAlgorithm::recommended(highSecurity) == HashAlgorithm::Sha3_384);

        const auto legacyOnly = HashRequirements{
            .requiredStatus = CryptographicStatus::Legacy,
        };
        REQUIRE(HashAlgorithm::matching(legacyOnly).isEmpty());
        REQUIRE_FALSE(HashAlgorithm::recommended(legacyOnly).has_value());

        const auto impossible = HashRequirements{
            .minimumSecurity = CryptographicSecurity::High,
            .minimumThroughput = HashThroughput::High,
        };
        REQUIRE(HashAlgorithm::matching(impossible).isEmpty());
        REQUIRE_FALSE(HashAlgorithm::recommended(impossible).has_value());
    }
};
