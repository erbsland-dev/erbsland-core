// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "UcdDatasetFixture.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ParseError.hpp>
#include <tool/idna/IdnaData.hpp>

TESTED_TARGETS(IdnaData)
class IdnaDataTest final : public UNITTEST_SUBCLASS(UcdDatasetFixture) {
public:
    void testRfcRulePrecedence() {
        using Data = tool::idna::IdnaData;
        constexpr auto letters = Data::cAssigned | Data::cLetterDigits;
        REQUIRE_EQUAL(Data::deriveStatus(0x00DF, letters | Data::cUnstable), 1U);
        REQUIRE_EQUAL(Data::deriveStatus(0x03C2, letters | Data::cUnstable), 1U);
        REQUIRE_EQUAL(Data::deriveStatus(0x00B7, Data::cAssigned), 3U);
        REQUIRE_EQUAL(Data::deriveStatus(0x0660, letters), 3U);
        REQUIRE_EQUAL(Data::deriveStatus(0x0640, letters), 0U);
        REQUIRE_EQUAL(Data::deriveStatus(U'a', letters | Data::cUnstable), 1U);
        REQUIRE_EQUAL(Data::deriveStatus(0x200C, Data::cAssigned | Data::cIgnorable), 2U);
        REQUIRE_EQUAL(Data::deriveStatus(U'A', letters | Data::cUnstable), 0U);
        REQUIRE_EQUAL(Data::deriveStatus(0x1100, letters | Data::cOldHangul), 0U);
        REQUIRE_EQUAL(Data::deriveStatus(0xFDD0, Data::cNoncharacter | Data::cIgnorable), 0U);
        REQUIRE_EQUAL(Data::deriveStatus(0x0378, 0), 0U);
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testMalformedUnicodeDataLight() {
        runInsideApplication([&]() -> void {
            prepareDataset("3400;<CJK, Last>;Lo;0;L;;;;;N;;;;;\n"_el);
            const auto dataset = tool::unicode::UcdDataset{el::Path{"ucd"_el}, "17.0.0"_el};
            REQUIRE_THROWS((tool::idna::IdnaData{dataset}));
        });
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testRecordErrorLocationAndCause() {
        runInsideApplication([&]() -> void {
            prepareDataset("\n# ignored line\n0041;LETTER A;Lu;invalid;L;;;;;N;;;;;\n"_el);
            const auto dataset = tool::unicode::UcdDataset{el::Path{"ucd"_el}, "17.0.0"_el};
            try {
                [[maybe_unused]] const auto data = tool::idna::IdnaData{dataset};
                REQUIRE(false);
            } catch (const el::ApplicationError &error) {
                REQUIRE_FALSE(error.title().isEmpty());
                REQUIRE(error.description().contains("canonical combining class"_el));
                REQUIRE_EQUAL(error.sourcePath(), (_root / "ucd/UnicodeData.txt"_el).toString());
                REQUIRE_EQUAL(error.codeLocation().line(), el::LineIndex{3});
                REQUIRE(error.cause());
                REQUIRE_THROWS_AS(el::ParseError, std::rethrow_exception(error.cause()));
            }
        });
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testMalformedUnicodeData() {
        runInsideApplication([&]() -> void {
            const auto malformed = el::StringList{
                "0041;LETTER A;INVALID;0;L;;;;;N;;;;;\n"_el,
                ""_el,
                "0041..0042;LETTER A;Lu;0;L;;;;;N;;;;;\n"_el,
                "3400;<CJK, First>;Lo;0;L;;;;;N;;;;;\n3401;<CJK, First>;Lo;0;L;;;;;N;;;;;\n"_el,
                "3400;<CJK, First>;Lo;0;L;;;;;N;;;;;\n3401;LETTER;Lo;0;L;;;;;N;;;;;\n"_el,
                "3400;<CJK, First>;Lo;0;L;;;;;N;;;;;\n4DBF;<OTHER, Last>;Lo;0;L;;;;;N;;;;;\n"_el,
                "3400;<CJK, First>;Lo;0;L;;;;;N;;;;;\n4DBF;<CJK, Last>;Ll;0;L;;;;;N;;;;;\n"_el,
                "3400;<CJK, First>;Lo;0;L;;;;;N;;;;;\n4DBF;<CJK, Last>;Lo;0;R;;;;;N;;;;;\n"_el,
                "0041;LETTER A;Lu;256;L;;;;;N;;;;;\n"_el,
                "0041;LETTER A;Lu;invalid;L;;;;;N;;;;;\n"_el,
                "0041;LETTER A;Lu;0;UNKNOWN;;;;;N;;;;;\n"_el,
                "0062;LETTER B;Ll;0;L;;;;;N;;;;;\n0061;LETTER A;Ll;0;L;;;;;N;;;;;\n"_el,
                "3400;<CJK, Last>;Lo;0;L;;;;;N;;;;;\n"_el,
                "3400;<CJK, First>;Lo;0;L;;;;;N;;;;;\n"_el,
                "3400;<CJK, First>;Lo;0;L;;;;;N;;;;;\n4DBF;<CJK, Last>;Lo;9;L;;;;;N;;;;;\n"_el,
                "0061;LETTER A;Ll;0;L;;;;;N;;;;;\n0061;LETTER A;Ll;0;L;;;;;N;;;;;\n"_el};
            for (const auto &text : malformed) {
                prepareDataset(text);
                const auto dataset = tool::unicode::UcdDataset{el::Path{"ucd"_el}, "17.0.0"_el};
                try {
                    [[maybe_unused]] const auto data = tool::idna::IdnaData{dataset};
                    REQUIRE(false);
                } catch (const el::ApplicationError &error) {
                    REQUIRE_FALSE(error.title().isEmpty());
                    REQUIRE_FALSE(error.description().isEmpty());
                    REQUIRE_EQUAL(error.sourcePath(), (_root / "ucd/UnicodeData.txt"_el).toString());
                    if (!text.isEmpty()) {
                        REQUIRE(error.codeLocation().line().isValid());
                    }
                }
            }
        });
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testMissingPropertiesAndUnknownJoiningType() {
        runInsideApplication([&]() -> void {
            for (
                const auto &filename : el::StringList{
                    "DerivedNormalizationProps.txt"_el,
                    "DerivedCoreProperties.txt"_el,
                    "PropList.txt"_el,
                    "Blocks.txt"_el,
                    "HangulSyllableType.txt"_el}) {
                auto overrides = el::StringMap<el::String>{};
                overrides.set(filename, "0041;Unrelated_Property\n"_el);
                prepareDataset("0061;LETTER A;Ll;0;L;;;;;N;;;;;\n"_el, overrides);
                const auto dataset = tool::unicode::UcdDataset{el::Path{"ucd"_el}, "17.0.0"_el};
                REQUIRE_THROWS((tool::idna::IdnaData{dataset}));
            }
            auto overrides = el::StringMap<el::String>{};
            overrides.set("DerivedJoiningType.txt"_el, "0061;Unknown\n"_el);
            prepareDataset("0061;LETTER A;Ll;0;L;;;;;N;;;;;\n"_el, overrides);
            const auto dataset = tool::unicode::UcdDataset{el::Path{"ucd"_el}, "17.0.0"_el};
            REQUIRE_THROWS((tool::idna::IdnaData{dataset}));
        });
    }
};
