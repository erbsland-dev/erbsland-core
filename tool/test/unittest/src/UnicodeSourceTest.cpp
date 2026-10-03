// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "UcdDatasetFixture.hpp"

#include <tool/idna/LabelProcessor.hpp>
#include <tool/unicode/UnicodeData.hpp>

TESTED_TARGETS(UnicodeSource NfcNormalizer IdnaMapping UnicodeData LabelProcessor)
class UnicodeSourceTest final : public UNITTEST_SUBCLASS(UcdDatasetFixture) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testNewerSourcesControlEveryDecision() {
        prepareDataset(
            "0061;A;Ll;0;L;;;;;N;;;;;\n0062;B;Ll;0;L;;;;;N;;;;;\n0300;MARK;Mn;230;NSM;;;;;N;;;;;\n"
            "0378;NEW LETTER;Ll;0;L;0061 0300;;;;N;;;;;\n0379;NEW MARK;Mn;0;NSM;;;;;N;;;;;\n"
            "0380;NEW RTL LETTER;Ll;0;AL;;;;;N;;;;;\n"_el,
            el::StringMap<el::String>{
                {"CompositionExclusions.txt"_el, "# empty exclusions\n"_el},
                {"IdnaMappingTable.txt"_el, "0000..10FFFF;valid\n"_el}},
            "18.0.0"_el,
            "18.0.0"_el);
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            "[rendering]\n[unicode]\nucd_directory: \"ucd\"\nunicode_version: \"18.0.0\"\n"_el);
        runWithSources(el::StringList{tool::unicode::UnicodeSource::sourceIdentifier()}, [&] {
            const auto &source = tool::frame::app().source<tool::unicode::UnicodeSource>();
            REQUIRE_EQUAL(source.version(), "18.0.0"_el);
            const auto &attributes = source.idnaData();
            const auto &normalizer = source.normalizer();
            const auto &mapping = source.mapping();
            const auto processor = tool::idna::LabelProcessor{attributes, normalizer, mapping};
            REQUIRE_EQUAL(normalizer.normalize("à"_el), "͸"_el);
            auto longText = el::StringEditor{"a"_el};
            for (auto index = 0U; index < 2000U; ++index) {
                longText.append("̀"_el);
            }
            const auto longNormalized = normalizer.normalize(longText);
            REQUIRE_EQUAL(longNormalized.characterLength().toSizeT(), 2000U);
            REQUIRE(longNormalized.startsWith("͸"_el));
            REQUIRE_EQUAL(processor.toAscii("à"_el), "xn--zva"_el);
            REQUIRE_EQUAL(processor.toAscii("xn--zva"_el), "xn--zva"_el);
            REQUIRE_EQUAL(attributes.attributes(0x0378U) & 3U, 1U);
            REQUIRE(normalizer.isMark(0x0379U));
            REQUIRE_THROWS(processor.toAscii("͹a"_el));
            REQUIRE_THROWS(processor.toAscii("a΀"_el));
            REQUIRE_THROWS(processor.toAscii("c"_el));
        });
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testMalformedNormalizationAndMappingSources() {
        for (
            const auto &data : el::StringList{
                "0061;A;Ll;0;L;0062;;;;N;;;;;\n0062;B;Ll;0;L;0061;;;;N;;;;;\n"_el,
                "0061;A;Ll;999;L;;;;;N;;;;;\n"_el,
                "0061;A;Ll;0;L;<compat>;;;;N;;;;;\n"_el}) {
            prepareDataset(data, el::StringMap<el::String>{{"CompositionExclusions.txt"_el, "# none\n"_el}});
            runInsideApplication([&] {
                const auto dataset = tool::unicode::UcdDataset{
                    el::Path{"ucd"_el},
                    "17.0.0"_el,
                    el::StringList{"UnicodeData.txt"_el, "CompositionExclusions.txt"_el}};
                REQUIRE_THROWS(tool::unicode::NfcNormalizer{dataset});
            });
        }
        for (
            const auto &mapping : el::StringList{
                "0001..10FFFF;valid\n"_el,
                "0000..10FFFF;unknown\n"_el,
                "0000..10FFFF;mapped\n"_el,
                "0000;valid\n0000..10FFFF;valid\n"_el}) {
            prepareDataset(
                "0061;A;Ll;0;L;;;;;N;;;;;\n"_el, el::StringMap<el::String>{{"IdnaMappingTable.txt"_el, mapping}});
            runInsideApplication([&] {
                const auto dataset = tool::unicode::UcdDataset{
                    el::Path{"ucd"_el}, "17.0.0"_el, el::StringList{"IdnaMappingTable.txt"_el}};
                REQUIRE_THROWS(tool::unicode::IdnaMapping{dataset});
            });
        }
    }

    void testMappingStatuses() {
        prepareDataset(
            "0061;A;Ll;0;L;;;;;N;;;;;\n"_el,
            el::StringMap<el::String>{
                {"IdnaMappingTable.txt"_el,
                    "0000..0040;disallowed\n0041;mapped;0061\n0042;ignored\n0043;deviation;0061\n"
                    "0044;disallowed_STD3_valid\n0045;disallowed_STD3_mapped;0061\n0046..10FFFF;valid\n"_el}});
        runInsideApplication([&] {
            const auto dataset =
                tool::unicode::UcdDataset{el::Path{"ucd"_el}, "17.0.0"_el, el::StringList{"IdnaMappingTable.txt"_el}};
            const auto mapping = tool::unicode::IdnaMapping{dataset};
            REQUIRE_EQUAL(mapping.map("ABCDEa"_el), "aCDaa"_el);
            REQUIRE_THROWS(mapping.map("!"_el));
        });
    }
};
