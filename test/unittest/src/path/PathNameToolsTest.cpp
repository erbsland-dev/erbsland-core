// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathTestHelper.hpp"

#include <erbsland/path/impl/PathNameTools.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;
using namespace el::unit;
using el::text::String;
using erbsland::test::pathtest::toStdString;

TESTED_TARGETS(Path)
class PathNameToolsTest final : public el::UnitTest {
public:
    void testSuffixPositions() {
        struct TestCase {
            String name;
            ByteIndex first;
            ByteIndex last;
        };
        const auto testCases = std::vector<TestCase>{
            {"ceu.claro.final.txt"_el, ByteIndex{3U}, ByteIndex{15U}},
            {".perfil"_el, ByteIndex::noIndex(), ByteIndex::noIndex()},
            {"relatorio."_el, ByteIndex{9U}, ByteIndex{9U}},
            {"..perfil"_el, ByteIndex::noIndex(), ByteIndex::noIndex()},
            {"...perfil"_el, ByteIndex::noIndex(), ByteIndex::noIndex()},
            {"....perfil"_el, ByteIndex::noIndex(), ByteIndex::noIndex()},
            {"."_el, ByteIndex::noIndex(), ByteIndex::noIndex()},
            {".."_el, ByteIndex::noIndex(), ByteIndex::noIndex()},
            {"..."_el, ByteIndex::noIndex(), ByteIndex::noIndex()},
            {"...."_el, ByteIndex::noIndex(), ByteIndex::noIndex()},
            {"éclair.final"_el, ByteIndex{7U}, ByteIndex{7U}},
        };

        for (const auto &testCase : testCases) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void {
                    const auto firstSuffix = el::path::impl::firstSuffixPosition(testCase.name);
                    const auto lastSuffix = el::path::impl::lastSuffixPosition(testCase.name);
                    REQUIRE_EQUAL(firstSuffix, testCase.first);
                    REQUIRE_EQUAL(lastSuffix, testCase.last);
                },
                [&]() -> std::string { return std::format("name: {}", toStdString(testCase.name)); });
        }
    }

    void testSuffixViews() {
        REQUIRE_EQUAL(el::path::impl::stem("ceu.claro.final.txt"_el), "ceu"_el);
        REQUIRE_EQUAL(el::path::impl::suffixes("ceu.claro.final.txt"_el), ".claro.final.txt"_el);
        REQUIRE_EQUAL(el::path::impl::lastSuffix("ceu.claro.final.txt"_el), ".txt"_el);
        REQUIRE_EQUAL(el::path::impl::stem(".perfil"_el), ".perfil"_el);
        REQUIRE(el::path::impl::suffixes(".perfil"_el).isEmpty());
        REQUIRE(el::path::impl::lastSuffix(".perfil"_el).isEmpty());
        REQUIRE_EQUAL(el::path::impl::stem("éclair.final"_el), "éclair"_el);
        REQUIRE_EQUAL(el::path::impl::normalizedSuffixReplacement("txt"_el), ".txt"_el);
        REQUIRE_EQUAL(el::path::impl::normalizedSuffixReplacement(".txt"_el), ".txt"_el);
        REQUIRE(el::path::impl::normalizedSuffixReplacement({}).isEmpty());
    }
};
