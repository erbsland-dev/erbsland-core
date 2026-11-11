// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/syntax/Formatter.hpp>
#include <erbsland/conf/syntax/Parser.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <utility>

using namespace el::text::literals;
using namespace el::conf::syntax;

TESTED_TARGETS(FormatStyle FormatStylePatch Formatter)
class SyntaxLiteralFormattingTest final : public el::UnitTest {
private:
    static auto simple() -> Format {
        auto format = Format{};
        auto style = format.baseStyle();
        style.setSectionIndent(el::CpCount{})
            .setSectionSuffix(el::CpCount{})
            .setSeparatorPositionMode(PositionMode::Spaces)
            .setSeparatorPosition(el::CpCount{});
        format.setBaseStyle(style);
        return format;
    }
    static auto render(const el::String &source, const Format &format) -> el::String {
        return Formatter{format}.format(el::conf::syntax::Parser{}.parseText(source)).text();
    }

public:
    void testAbbreviationWordsAndCases() {
        auto format = simple();
        const auto source = "[http_server]\nhttps_ftp_ftps_ssh_vr: 1\nhttp2_httpd: 2\n"_el;
        const auto expected = "[ HTTP Server ]\nHTTPS FTP FTPS SSH VR: 1\nHttp2 Httpd: 2\n"_el;
        REQUIRE_EQUAL(render(source, format), expected);
        REQUIRE_EQUAL(render(expected, format), expected);
        auto style = format.baseStyle();
        style.setValueNameCase(WordCase::Lower);
        format.setBaseStyle(style);
        REQUIRE(render(source, format).contains("https ftp ftps ssh vr: 1"_el));
        style.setValueNameCase(WordCase::Keep);
        format.setBaseStyle(style);
        REQUIRE(render(source, format).contains("https ftp ftps ssh vr: 1"_el));
        style.setValueNameCase(WordCase::Upper);
        format.setBaseStyle(style);
        REQUIRE(render(source, format).contains("HTTP2 HTTPD: 2"_el));
    }
    void testAbbreviationNormalizationAndCascade() {
        auto format = simple();
        auto style = format.baseStyle();
        style.setAbbreviations(el::StringList{"cpu"_el, "CPU"_el, "Ipv6"_el});
        REQUIRE_EQUAL(style.abbreviations().count(), el::ItemCount{2});
        REQUIRE_EQUAL(style.abbreviations().first(), "CPU"_el);
        format.setBaseStyle(style);
        format.addRule({Selector{}.addPath("app.http_cpu"_el), FormatStylePatch{}.setAbbreviations(el::StringList{})});
        format.addRule({Selector{}.addPath("app.ipv6_http"_el), FormatStylePatch{}.setBooleanCase(WordCase::Lower)});
        REQUIRE_EQUAL(
            render("[app]\nhttp_cpu: 1\nipv6_http: 2\n"_el, format), "[ App ]\nHttp Cpu: 1\nIPV6 Http: 2\n"_el);
        for (const auto word : {""_el, "two words"_el, "two_words"_el, "1http"_el, "http!"_el}) {
            REQUIRE_THROWS_AS(el::err::ParameterFailure, style.setAbbreviations(el::StringList{word}));
            REQUIRE_THROWS_AS(el::err::ParameterFailure, FormatStylePatch{}.setAbbreviations(el::StringList{word}));
        }
    }
    void testOptInQuotedIdentifiers() {
        auto format = simple();
        const auto source = "[app]\n\"http_name\": \"ssh_key\"\n"_el;
        REQUIRE_EQUAL(render(source, format), "[ App ]\n\"http_name\": \"ssh_key\"\n"_el);
        auto style = format.baseStyle();
        style.setTextNameCase(WordCase::Title).setTextIdentifierCase(WordCase::Title);
        format.setBaseStyle(style);
        REQUIRE_EQUAL(render(source, format), "[ App ]\n\"HTTP Name\": \"SSH Key\"\n"_el);
    }
    void testAllBooleanCasesAndForms() {
        struct Example {
            BooleanForm form;
            el::String lower;
            el::String title;
            el::String upper;
            el::String keep;
        };
        const auto source = "[app]\nvalues: tRuE, FaLsE, YES, nO, On, oFF, ENABLED, dIsAbLeD\n"_el;
        const auto cases = std::array{
            Example{
                BooleanForm::Keep,
                "true, false, yes, no, on, off, enabled, disabled"_el,
                "True, False, Yes, No, On, Off, Enabled, Disabled"_el,
                "TRUE, FALSE, YES, NO, ON, OFF, ENABLED, DISABLED"_el,
                "tRuE, FaLsE, YES, nO, On, oFF, ENABLED, dIsAbLeD"_el},
            Example{
                BooleanForm::TrueFalse,
                "true, false, true, false, true, false, true, false"_el,
                "True, False, True, False, True, False, True, False"_el,
                "TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, TRUE, FALSE"_el,
                "tRuE, FaLsE, true, false, true, false, true, false"_el},
            Example{
                BooleanForm::YesNo,
                "yes, no, yes, no, yes, no, yes, no"_el,
                "Yes, No, Yes, No, Yes, No, Yes, No"_el,
                "YES, NO, YES, NO, YES, NO, YES, NO"_el,
                "yes, no, YES, nO, yes, no, yes, no"_el},
            Example{
                BooleanForm::OnOff,
                "on, off, on, off, on, off, on, off"_el,
                "On, Off, On, Off, On, Off, On, Off"_el,
                "ON, OFF, ON, OFF, ON, OFF, ON, OFF"_el,
                "on, off, on, off, On, oFF, on, off"_el},
            Example{
                BooleanForm::EnabledDisabled,
                "enabled, disabled, enabled, disabled, enabled, disabled, enabled, disabled"_el,
                "Enabled, Disabled, Enabled, Disabled, Enabled, Disabled, Enabled, Disabled"_el,
                "ENABLED, DISABLED, ENABLED, DISABLED, ENABLED, DISABLED, ENABLED, DISABLED"_el,
                "enabled, disabled, enabled, disabled, enabled, disabled, ENABLED, dIsAbLeD"_el}};
        REQUIRE_EQUAL(FormatStyle{}.booleanCase(), WordCase::Title);
        REQUIRE_EQUAL(FormatStyle{}.booleanForm(), BooleanForm::Keep);
        for (const auto &example : cases) {
            for (
                const auto &[casing, expected] : std::array{
                    std::pair{WordCase::Lower, example.lower},
                    std::pair{WordCase::Title, example.title},
                    std::pair{WordCase::Upper, example.upper},
                    std::pair{WordCase::Keep, example.keep}}) {
                auto format = simple();
                auto style = format.baseStyle();
                style.setBooleanForm(example.form).setBooleanCase(casing);
                format.setBaseStyle(style);
                const auto output = render(source, format);
                REQUIRE_EQUAL(output, el::text::join("[ App ]\nValues: "_el, expected, "\n"_el));
                REQUIRE_EQUAL(render(output, format), output);
                const auto document = el::conf::Parser{}.parseTextOrThrow(output);
                const auto values = document->valueOrThrow("app.values"_el)->asList<bool>();
                REQUIRE_EQUAL(values.count(), el::ItemCount{8});
                auto expectedValue = true;
                for (const auto value : values) {
                    REQUIRE_EQUAL(value, expectedValue);
                    expectedValue = !expectedValue;
                }
            }
        }
    }
    void testBooleanLayoutAndPreservation() {
        auto format = simple();
        auto style = format.baseStyle();
        style.setBooleanForm(BooleanForm::EnabledDisabled)
            .setMaximumLineLength(el::CpCount{17})
            .setMatrixStyle(MatrixStyle::Align)
            .setVectorStyle(VectorStyle::Compact);
        format.setBaseStyle(style);
        REQUIRE_EQUAL(render("[a]\nlong_name: ON\n"_el, format), "[ A ]\nLong Name:\n    Enabled\n"_el);
        const auto matrix = render("[a]\nm:\n    * ON, 1\n    * OFF, 2\n"_el, format);
        REQUIRE(matrix.contains("* Enabled,  1"_el));
        REQUIRE(matrix.contains("* Disabled, 2"_el));
        REQUIRE_EQUAL(render(matrix, format), matrix);
        const auto vector = render("[a]\nv: ON, OFF\n"_el, format);
        REQUIRE(vector.contains("* Enabled\n"_el));
        REQUIRE(vector.contains("* Disabled\n"_el));
        const auto preserved =
            render("[a]\nx: YES # ON stays\nt: \"ON\"\nbad: ON, ???\ntext: \"\"\"\n    OFF\n    \"\"\"\n"_el, format);
        REQUIRE(preserved.contains("Enabled # ON stays"_el));
        REQUIRE(preserved.contains("\"ON\""_el));
        REQUIRE(preserved.contains("bad: ON, ???\n"_el));
        REQUIRE(preserved.contains("    OFF\n"_el));
    }
};
