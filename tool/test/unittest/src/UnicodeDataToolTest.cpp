// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UcdDatasetFixture.hpp"

#include <tool/unicode/UnicodeLightTables.hpp>
#include <tool/unicode/UnicodeNormalizationTables.hpp>

TESTED_TARGETS(UnicodeLightTables UnicodeNormalizationTables UnicodeDataTool)
class UnicodeDataToolTest final : public UNITTEST_SUBCLASS(UcdDatasetFixture) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testSourceChangesBothGeneratedTables() {
        auto original = el::String{};
        prepare(
            "0041;LATIN CAPITAL LETTER A;Lu;0;L;;;;;N;;;;0061;\n"
            "0061;LATIN SMALL LETTER A;Ll;0;L;;;;;N;;;0041;;\n"
            "00C1;LATIN CAPITAL LETTER A WITH ACUTE;Lu;0;L;0041 0301;;;;N;;;;;\n"
            "0301;COMBINING ACUTE ACCENT;Mn;230;NSM;;;;;N;;;;;\n"_el,
            "0041; C; 0061; # simple fold\n"_el);
        runWithSources(el::StringList{"unicode"_el}, [&] {
            const auto &source = tool::frame::app().source<tool::unicode::UnicodeSource>();
            const auto light = tool::unicode::UnicodeLightTables{source};
            const auto normalization = tool::unicode::UnicodeNormalizationTables{source};
            original = light.renderData();
            REQUIRE(original.contains("UnicodeCategory::UppercaseLetter"_el));
            REQUIRE(normalization.render().contains("unicodeNormalizationDecompositionData"_el));
            REQUIRE_EQUAL(
                normalization.decompositionData(), (std::vector<uint8_t>{0xC1U, 0x01U, 0x01U, 0x41U, 0x81U, 0x06U}));
            REQUIRE_EQUAL(normalization.combiningData(), (std::vector<uint8_t>{0x81U, 0x06U, 0xE6U}));
            REQUIRE_EQUAL(
                normalization.compositionData(), (std::vector<uint8_t>{0x41U, 0x01U, 0x81U, 0x06U, 0xC1U, 0x01U}));
        });
        prepare(
            "0041;LATIN CAPITAL LETTER A;Lu;0;L;;;;;N;;;;0061;\n"
            "0061;LATIN SMALL LETTER A;Ll;0;L;;;;;N;;;0041;;\n"
            "00C1;LATIN CAPITAL LETTER A WITH ACUTE;Lu;0;L;0041 0301;;;;N;;;;;\n"
            "0301;COMBINING ACUTE ACCENT;Mn;230;NSM;;;;;N;;;;;\n"_el,
            "0041; C; 0062; # changed simple fold\n"_el);
        runWithSources(el::StringList{"unicode"_el}, [&] {
            const auto &source = tool::frame::app().source<tool::unicode::UnicodeSource>();
            const auto light = tool::unicode::UnicodeLightTables{source};
            REQUIRE(light.renderData() != original);
        });
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testMalformedFoldingRecord() {
        prepare("0041;LATIN CAPITAL LETTER A;Lu;0;L;;;;;N;;;;0061;\n"_el, "0041; C; ZZZZ; # malformed scalar\n"_el);
        runWithSources(el::StringList{"unicode"_el}, [&] {
            const auto &source = tool::frame::app().source<tool::unicode::UnicodeSource>();
            REQUIRE_THROWS(tool::unicode::UnicodeLightTables{source});
        });
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testCaseDeltaTableLimit() {
        auto unicodeData = el::StringEditor{};
        auto caseFolding = el::StringEditor{};
        for (auto index = 0U; index < 257U; ++index) {
            const auto source = el::StringFormat{"{:number:hex,precision=4,lc=uppercase}"_el}.build(0x0100U + index);
            const auto destination =
                el::StringFormat{"{:number:hex,precision=4,lc=uppercase}"_el}.build(0x0500U + index * 2U);
            unicodeData.append(el::StringFormat{"{};TEST LETTER;Ll;0;L;;;;;N;;;;;\n"_el}.build(source));
            caseFolding.append(el::StringFormat{"{}; C; {}; # varying delta\n"_el}.build(source, destination));
        }
        prepare(unicodeData, caseFolding);
        runWithSources(el::StringList{"unicode"_el}, [&] {
            const auto &source = tool::frame::app().source<tool::unicode::UnicodeSource>();
            REQUIRE_THROWS(tool::unicode::UnicodeLightTables{source});
        });
    }

private:
    void prepare(const el::String &unicodeData, const el::String &caseFolding) {
        prepareDataset(
            unicodeData,
            {{"CaseFolding.txt"_el, caseFolding},
                {"CompositionExclusions.txt"_el, "# no exclusions\n"_el},
                {"IdnaMappingTable.txt"_el, "0000..10FFFF;valid\n"_el}});
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            "[rendering]\n[unicode]\nucd_directory: \"ucd\"\nunicode_version: \"17.0.0\"\n"_el);
    }
};
