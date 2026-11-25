// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UcdDatasetFixture.hpp"

#include <erbsland/PathInfo.hpp>
#include <tool/test_data/CategoryTestDataTool.hpp>
#include <tool/test_data/impl/CategoryTestData.hpp>

TESTED_TARGETS(CategoryTestData CategoryTestDataTool)
class CategoryTestDataTest final : public UNITTEST_SUBCLASS(UcdDatasetFixture) {
public:
    void testExpressionsLight() {
        using tool::test_data::impl::CategoryTestData;
        REQUIRE_EQUAL(
            CategoryTestData::expression(0x30, "Nd"_el),
            "Any | AnyDotAll | DigitAscii | DigitUnicode | Nd | WordAscii | WordUnicode"_el);
        REQUIRE_EQUAL(
            CategoryTestData::expression(0x660, "Nd"_el), "Any | AnyDotAll | DigitUnicode | Nd | WordUnicode"_el);
        REQUIRE_EQUAL(CategoryTestData::expression(0x41, "Lu"_el), "Any | AnyDotAll | Lu | WordAscii | WordUnicode"_el);
        REQUIRE_EQUAL(CategoryTestData::expression(0x5F, "Pc"_el), "Any | AnyDotAll | Pc | WordAscii | WordUnicode"_el);
        REQUIRE_EQUAL(CategoryTestData::expression(0x100, "Lu"_el), "Any | AnyDotAll | Lu | WordUnicode"_el);
        REQUIRE_EQUAL(
            CategoryTestData::expression(9, "Cc"_el),
            "Any | AnyDotAll | Cc | HorizontalSpaceAscii | SpaceAscii | SpaceAsciiDotAll | SpaceUnicode | SpaceUnicodeDotAll"_el);
        REQUIRE_EQUAL(
            CategoryTestData::expression(10, "Cc"_el),
            "AnyDotAll | Cc | SpaceAsciiDotAll | SpaceUnicodeDotAll | VerticalSpaceAscii | VerticalSpaceUnicode"_el);
        for (const auto codePoint : {11U, 12U, 13U}) {
            REQUIRE_EQUAL(
                CategoryTestData::expression(codePoint, "Cc"_el),
                "Any | AnyDotAll | Cc | SpaceAsciiDotAll | SpaceUnicodeDotAll | VerticalSpaceAscii | VerticalSpaceUnicode"_el);
        }
        REQUIRE_EQUAL(
            CategoryTestData::expression(0x20, "Zs"_el),
            "Any | AnyDotAll | HorizontalSpaceAscii | HorizontalSpaceUnicode | SpaceAscii | SpaceAsciiDotAll | SpaceUnicode | SpaceUnicodeDotAll | Zs"_el);
        REQUIRE_EQUAL(
            CategoryTestData::expression(0xA0, "Zs"_el),
            "Any | AnyDotAll | HorizontalSpaceAscii | HorizontalSpaceUnicode | SpaceUnicode | SpaceUnicodeDotAll | Zs"_el);
        REQUIRE_EQUAL(CategoryTestData::expression(0x85, "Cc"_el), "Any | AnyDotAll | Cc | VerticalSpaceUnicode"_el);
        REQUIRE_EQUAL(
            CategoryTestData::expression(0x2028, "Zl"_el),
            "Any | AnyDotAll | SpaceUnicodeDotAll | VerticalSpaceUnicode | Zl"_el);
        REQUIRE_EQUAL(CategoryTestData::expression(0xD800, "Cs"_el), "Any | AnyDotAll | Cs"_el);
        REQUIRE_EQUAL(CategoryTestData::expression(0x10FFFF, "Cn"_el), "Any | AnyDotAll | Cn"_el);
    }

    void testSamplingAndGapsLight() {
        const auto records = el::List<tool::unicode::UcdRecord>{
            {0x100, 0x101, el::StringList{""_el, ""_el, "Lo"_el}, {}},
            {0x102, 0x104, el::StringList{""_el, ""_el, "Lo"_el}, {}},
            {0x106, 0x107, el::StringList{""_el, ""_el, "Cc"_el}, {}},
            {0x109, 0x109, el::StringList{""_el, ""_el, "Cc"_el}, {}}};
        const auto text = tool::test_data::impl::CategoryTestData::render(records, "17.0.0"_el);
        REQUIRE(text.contains("Unicode 17.0.0"_el));
        REQUIRE(text.contains(
            "000100;Any | AnyDotAll | Lo | WordUnicode\n"
            "000104;Any | AnyDotAll | Lo | WordUnicode\n"
            "000102;Any | AnyDotAll | Lo | WordUnicode\n"
            "000105;Any | AnyDotAll | Cn\n"
            "000106;Any | AnyDotAll | Cc\n"
            "000107;Any | AnyDotAll | Cc\n"
            "000108;Any | AnyDotAll | Cn\n"
            "000109;Any | AnyDotAll | Cc\n"_el));
        REQUIRE(text.contains("00010A;Any | AnyDotAll | Cn\n10FFFF;Any | AnyDotAll | Cn\n"_el));
        REQUIRE(text.contains(
            "000000;Any | AnyDotAll | Cn\n000008;Any | AnyDotAll | Cn\n000004;Any | AnyDotAll | Cn\n"_el));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testPinnedSnapshotMatchesFixture() {
        _root = tool::frame::ProjectRoot{el::Path::currentDirectory(), TestToolApplication::coreProject()}.discover();
        runWithSources(el::StringList{tool::unicode::UnicodeSource::sourceIdentifier()}, [&] {
            const auto &source = tool::frame::app().source<tool::unicode::UnicodeSource>();
            REQUIRE_EQUAL(
                tool::test_data::impl::CategoryTestData::render(source.unicodeData().records(), source.version()),
                files().readText(el::Path{"test/unittest/data/re/CategoryContainsTestData.txt"_el}));
        });
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testSourceChangesCheckAndValidation() {
        prepare("Lu"_el);
        REQUIRE(run(el::StringList{"--check"_el}) != 0);
        REQUIRE_FALSE((_root / "output"_el).info().exists());
        REQUIRE_EQUAL(run(), 0);
        const auto path = el::Path{"output/CategoryContainsTestData.txt"_el};
        const auto original = files().readText(path);
        REQUIRE(original.contains("000041;Any | AnyDotAll | Lu | WordAscii | WordUnicode"_el));
        REQUIRE_EQUAL(run(el::StringList{"--check"_el}), 0);
        write(path.toString(), original.replacedAll("\n"_el, "\r\n"_el));
        const auto bytes = files().readData(path);
        REQUIRE_EQUAL(run(), 0);
        REQUIRE_EQUAL(files().readData(path), bytes);
        prepare("Lo"_el);
        REQUIRE(run(el::StringList{"--check"_el}) != 0);
        REQUIRE_EQUAL(files().readData(path), bytes);
        REQUIRE_EQUAL(run(), 0);
        REQUIRE(files().readText(path).contains("000041;Any | AnyDotAll | Lo | WordAscii | WordUnicode"_el));
        write(path.toString(), "sentinel"_el);
        write("ucd/UnicodeData.txt"_el, "corrupt input"_el);
        REQUIRE(run() != 0);
        REQUIRE_EQUAL(files().readText(path), "sentinel"_el);
    }

private:
    void prepare(const el::String &category) {
        prepareDataset(
            el::StringFormat{"0041;TEST LETTER;{};0;L;;;;;N;;;;;\n"_el}.build(category),
            {{"CompositionExclusions.txt"_el, "# no exclusions\n"_el},
                {"IdnaMappingTable.txt"_el, "0000..10FFFF;valid\n"_el}});
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            "[rendering]\n[unicode]\nucd_directory: \"ucd\"\nunicode_version: \"17.0.0\"\n"
            "[generate_category_test_data]\noutput_directory: \"output\"\n"_el);
    }
    [[nodiscard]] auto run(const el::StringList &options = {}) -> int {
        return runTool<tool::test_data::CategoryTestDataTool>({}, {}, "generate-category-test-data"_el, options);
    }
};
