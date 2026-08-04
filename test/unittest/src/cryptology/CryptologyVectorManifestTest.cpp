// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/Hasher.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/unittest/FileHelper.hpp>

#include <span>
#include <sstream>
#include <string>

using namespace el::cryptology;

TESTED_TARGETS(Hasher)
class CryptologyVectorManifestTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    void testFixtureCount() {
        const auto lines = el::unittest::fh::readDataLines("data/cryptology/manifest.txt");
        REQUIRE_EQUAL(lines.size(), 61U);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testFixtureChecksums() {
        const auto lines = el::unittest::fh::readDataLines("data/cryptology/manifest.txt");
        REQUIRE_EQUAL(lines.front(), "# path sha256 records groups");

        for (const auto &line : std::span{lines}.subspan(1U)) {
            auto input = std::istringstream{line};
            auto path = std::string{};
            auto digestText = std::string{};
            auto recordCount = std::size_t{};
            auto groupCount = std::size_t{};
            auto trailing = std::string{};
            REQUIRE(input >> path >> digestText >> recordCount >> groupCount);
            REQUIRE_FALSE(input >> trailing);
            REQUIRE(recordCount > 0U);
            REQUIRE(groupCount > 0U);

            const auto fixturePath = std::string{"data/cryptology/"} + path;
            const auto fixture = el::unittest::fh::readDataText(fixturePath, 50'000'000U);
            auto hasher = Hasher{HashAlgorithm::Sha2_256};
            hasher.update(el::mem::ByteBlock::fromSpan(std::span<const char>{fixture}));
            REQUIRE_EQUAL(hasher.finalize(), bytesFromHex(digestText));
        }
    }
};
