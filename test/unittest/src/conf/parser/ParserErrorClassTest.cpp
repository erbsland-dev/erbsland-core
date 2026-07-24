// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/re/RegExError.hpp>

#include <algorithm>
#include <map>

using namespace el::conf;
using namespace el::text::literals;

TESTED_TARGETS(Parser)
class ParserErrorClassTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    using TestCases = std::vector<std::pair<el::text::String, ConfErrorCategory>>;

    void verifyTestCases(const TestCases &testCases) {
        for (const auto &testCase : testCases) {
            ConfErrorCategory actualCategory;
            el::text::String actualMessage;
            ConfErrorCategory expectedCategory;
            runWithContext(
                SOURCE_LOCATION(),
                [&] {
                    expectedCategory = testCase.second;
                    auto source = createTestMemorySource(testCase.first);
                    auto parser = Parser{};
                    try {
                        auto doc = parser.parseOrThrow(source);
                        REQUIRE(false); // must not succeed.
                    } catch (const ConfError &error) {
                        actualCategory = error.category();
                        actualMessage = error.description();
                        REQUIRE_EQUAL(actualCategory, expectedCategory);
                    } catch (const erbsland::AssertFailed &) {
                        actualMessage = "Parsing succeeded, but should have failed."_el;
                        throw;
                    } catch (const std::exception &error) {
                        actualMessage = el::text::String{
                            std::format("Unexpected exception: type={}, what={}", typeid(error).name(), error.what())};
                        REQUIRE(false);
                    }
                },
                [&]() -> std::string {
                    return std::format(
                        "Failed for text: \"{}\"\nExpected {}, got {}.\nError message: {}",
                        el::text::StringConverter{testCase.first.toEscaped(el::text::EscapeFormat::Display)}
                            .toStdString(),
                        expectedCategory,
                        actualCategory,
                        el::text::StringConverter{actualMessage}.toStdString());
                });
        }
    }

    void testInvalidRegExIsReportedOnFirstUse() {
        auto parser = Parser{};
        DocumentPtr document;
        REQUIRE_NOTHROW(document = parser.parseOrThrow(createTestMemorySource("[main]\nvalue: /(/\n"_el)));
        REQUIRE(document != nullptr);

        const auto regex = document->getRegExOrThrow(NamePath::fromText("main.value"_el));
        REQUIRE(regex != nullptr);
        REQUIRE_FALSE(regex->isCompiled());
        REQUIRE_THROWS_AS(el::re::RegExError, regex->compileNow());
        REQUIRE_FALSE(regex->isCompiled());
    }

    void testUnexpectedEndVsSyntaxError() {
        // Test situations where the parser should detect an unexpected end of the document and not just
        // a syntax error.
        const auto testCases = TestCases{
            {"#comment\r"_el, ConfErrorCategory::UnexpectedEnd},
            {"["_el, ConfErrorCategory::UnexpectedEnd},
            {"[\n"_el, ConfErrorCategory::Syntax},
            {"[\r\n"_el, ConfErrorCategory::Syntax},
            {"[main"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main\n"_el, ConfErrorCategory::Syntax},
            {"[main\r\n"_el, ConfErrorCategory::Syntax},
            {"[main "_el, ConfErrorCategory::UnexpectedEnd},
            {"[main \n"_el, ConfErrorCategory::Syntax},
            {"[main \r\n"_el, ConfErrorCategory::Syntax},
            {"[main."_el, ConfErrorCategory::UnexpectedEnd},
            {"[main.\n"_el, ConfErrorCategory::Syntax},
            {"[main.\r\n"_el, ConfErrorCategory::Syntax},
            {"[main. "_el, ConfErrorCategory::UnexpectedEnd},
            {"[main. \n"_el, ConfErrorCategory::Syntax},
            {"[main. \r\n"_el, ConfErrorCategory::Syntax},
            {"[main.sub"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main.sub\n"_el, ConfErrorCategory::Syntax},
            {"[main.sub\r\n"_el, ConfErrorCategory::Syntax},
            {"[main.sub "_el, ConfErrorCategory::UnexpectedEnd},
            {"[main.sub \n"_el, ConfErrorCategory::Syntax},
            {"[main.sub \r\n"_el, ConfErrorCategory::Syntax},
            {"[main.sub."_el, ConfErrorCategory::UnexpectedEnd},
            {"[main.sub.\n"_el, ConfErrorCategory::Syntax},
            {"[main.sub.\r\n"_el, ConfErrorCategory::Syntax},
            {"[main.sub. "_el, ConfErrorCategory::UnexpectedEnd},
            {"[main.sub. \n"_el, ConfErrorCategory::Syntax},
            {"[main.sub. \r\n"_el, ConfErrorCategory::Syntax},
            {"[main.\""_el, ConfErrorCategory::UnexpectedEnd},
            {"[main.\"\n"_el, ConfErrorCategory::Syntax},
            {"[main.\"\r\n"_el, ConfErrorCategory::Syntax},
            {"[main.\"sub"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main.\"sub\n"_el, ConfErrorCategory::Syntax},
            {"[main.\"sub\r\n"_el, ConfErrorCategory::Syntax},
            {"[main.\"sub\""_el, ConfErrorCategory::UnexpectedEnd},
            {"[main.\"sub\"\n"_el, ConfErrorCategory::Syntax},
            {"[main.\"sub\"\r\n"_el, ConfErrorCategory::Syntax},
            {"[main.\"sub\" "_el, ConfErrorCategory::UnexpectedEnd},
            {"[main.\"sub\" \n"_el, ConfErrorCategory::Syntax},
            {"[main.\"sub\" \r\n"_el, ConfErrorCategory::Syntax},
            {"[main.\"sub\"."_el, ConfErrorCategory::UnexpectedEnd},
            {"[main.\"sub\".\n"_el, ConfErrorCategory::Syntax},
            {"[main.\"sub\".\r\n"_el, ConfErrorCategory::Syntax},
            {"[main.\"sub\". "_el, ConfErrorCategory::UnexpectedEnd},
            {"[main.\"sub\". \n"_el, ConfErrorCategory::Syntax},
            {"[main.\"sub\". \r\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue    "_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue # comment"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue:"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: # comment"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue="_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue= # comment"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue   :"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue   : # comment"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue:\n"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: # comment\n"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue=\n"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue= # comment\n"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\n\""_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\n\"text "_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\n\"text value"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\n\"text value\""_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\n\"text value\"\n"_el, ConfErrorCategory::Syntax},
            {"[main]\n\"text value\"    "_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\n\"text value\" # comment"_el, ConfErrorCategory::Syntax},
            {"[main]\n\"text value\":"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\n\"text value\": # comment"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\n\"text value\"   :"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\n\"text value\"   : # comment"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\n\"text value\":\n"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\n\"text value\": # comment\n"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: \""_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: \"\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: \"text"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: \"text\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: `"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: `\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: `text"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: `text\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: /"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: /\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: /text"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: /text\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: <"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: <\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: <hex"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: <hex\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: <hex:"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: <hex:\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: <0102"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: <0102\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: \"\"\""_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: \"\"\"\n    text"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: \"\"\"\n    text\n"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: ```"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: ```\n    text"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: ```\n    text\n"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: ///"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: ///\n    text"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: ///\n    text\n"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: <<<"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: <<<\n    0102"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: <<<\n    0102\n"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 100'"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 0x"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 0x1000'"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 0x\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 0b"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 0b1111'"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 0b\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 1, 2,"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 1, 2, "_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 1, 2,\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 100e"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 0.1e+"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 100e\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 0.1e+\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 2025-"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 2025-0"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 2025-08"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 2025-08-"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 2025-08-0"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 2025-08-01t"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 2025-08-01 12:"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 2025-08-01 12:0"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 2025-08-01 12:05:"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 2025-08-01 12:05:3"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 2025-08-01 12:05:34+"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 2025-08-01 12:05:34+0"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 2025-08-01 12:05:34+01:"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 2025-08-01 12:05:34+01:3"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 12:"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 12:0"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 12:05:"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 12:05:3"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 12:05:34+"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 12:05:34+0"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 12:05:34+01:"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 12:05:34+01:3"_el, ConfErrorCategory::UnexpectedEnd},
            {"[main]\nvalue: 2025-\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 2025-0\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 2025-08\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 2025-08-\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 2025-08-0\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 2025-08-01t\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 2025-08-01 12:\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 2025-08-01 12:0\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 2025-08-01 12:05:\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 2025-08-01 12:05:3\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 2025-08-01 12:05:34+\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 2025-08-01 12:05:34+0\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 2025-08-01 12:05:34+01:\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 2025-08-01 12:05:34+01:3\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 12:\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 12:0\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 12:05:\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 12:05:3\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 12:05:34+\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 12:05:34+0\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 12:05:34+01:\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: 12:05:34+01:3\n"_el, ConfErrorCategory::Syntax},
        };
        WITH_CONTEXT(verifyTestCases(testCases));
    }

    void testUnsupportedVsSyntaxError() {
        // Test situations where the parser should detect an unsupported error or a syntax error.
        const auto testCases = std::vector<std::pair<el::text::String, ConfErrorCategory>>{
            {"@version: \"0.9\"\n"_el, ConfErrorCategory::Unsupported},
            {"@version: `0.9`\n"_el, ConfErrorCategory::Syntax},
            {"@version: `1.0`\n"_el, ConfErrorCategory::Syntax},
            {"@version: \"\"\"\n    1.0\n    \"\"\"\n"_el, ConfErrorCategory::Syntax},
            {"@version: 1\n"_el, ConfErrorCategory::Syntax},
            {"@version: 2\n"_el, ConfErrorCategory::Syntax},
            {"@features: \"abcde\"\n"_el, ConfErrorCategory::Unsupported},
            {"@features: \"core abcde\"\n"_el, ConfErrorCategory::Unsupported},
            {"@features: `core`\n"_el, ConfErrorCategory::Syntax},
            {"@features: \"\"\"\n    core\n    \"\"\"\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: <base64: 01234>\n"_el, ConfErrorCategory::Unsupported},
            {"[main]\nvalue: <none$: 01234>\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nvalue: <<<base64\n    01234\n    >>>\n"_el, ConfErrorCategory::Unsupported},
            {"[main]\nvalue: <<<none$\n    01234>\n    >>>\n"_el, ConfErrorCategory::Syntax},
        };
        WITH_CONTEXT(verifyTestCases(testCases));
    }

    void testIndentationVsSyntaxError() {
        // Test situations where the parser should detect an indentation error and not a syntax error.
        const auto testCases = std::vector<std::pair<el::text::String, ConfErrorCategory>>{
            {"[main]\nv: \"\"\"\n  t\n t\n  \"\"\"\n"_el, ConfErrorCategory::Indentation},
            {"[main]\nv: \"\"\"\n  t\n_ t\n  \"\"\"\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nv: \"\"\"\n\tt\n        t\n\t\"\"\"\n"_el, ConfErrorCategory::Indentation},
            {"[main]\nv:\n  \"\"\"\n t  \"\"\"\n"_el, ConfErrorCategory::Indentation},
            {"[main]\nv: ```\n  t\n t\n  ```\n"_el, ConfErrorCategory::Indentation},
            {"[main]\nv: ```\n  t\n_ t\n  ```\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nv: ```\n\tt\n        t\n\t```\n"_el, ConfErrorCategory::Indentation},
            {"[main]\nv:\n  ```\n t  ```\n"_el, ConfErrorCategory::Indentation},
            {"[main]\nv: <<<\n  00\n 00\n  >>>\n"_el, ConfErrorCategory::Indentation},
            {"[main]\nv: <<<\n  00\n_ 00\n  >>>\n"_el, ConfErrorCategory::Syntax},
            {"[main]\nv: <<<\n\t00\n        00\n\t>>>\n"_el, ConfErrorCategory::Indentation},
            {"[main]\nv:\n  <<<\n 00  >>>\n"_el, ConfErrorCategory::Indentation},
            {"[main]\nv:\n  * 1\n * 2\n"_el, ConfErrorCategory::Indentation},
        };
        WITH_CONTEXT(verifyTestCases(testCases));
    }

    void testCharacterVsSyntaxError() {
        // Test where the more specialized character error should be reported instead of a syntax error.
        // As this parser tests for illegal control characters just after UTF-8 decoding,
        // the question is just if the error gets correctly propagated through the lexer.
        // By inserting a control character into every position of the test document,
        // propagation errors should be sufficiently uncovered.
        const auto testDocument = el::text::String{"# Comment\n"_el
                                                   "[main]\n"
                                                   "v1: true\n"
                                                   "v2: 123'456\n"
                                                   "v3:\n\t0xab'01\n"
                                                   "v4:\n 0b11'00#c\n"
                                                   "v5: 12kb #c\n"
                                                   "v6: 12 kb\t\n"
                                                   "v7: 123'456 \n"
                                                   "v8: \"t\"\n"
                                                   "v9: 0.7e+2\t#c\n"
                                                   "v10: 01:02:03.123\n"
                                                   "v11: 2025-01-02\n"
                                                   "v12: 2025-01-02 01:02:03.123+01:30\n"
                                                   "v13: 2025-01-02t01:02:03.123+01:30\n"
                                                   "v14: 12h\n"
                                                   "v15: 5 weeks\n"
                                                   "v16: `c`\n"
                                                   "v17: <01>\n"
                                                   "v18: \"\"\"\n t\n \"\"\"\n"
                                                   "v19: \"\"\" #c\n t\n \"\"\" #c\n"
                                                   "v20: ```\n c\n ```\n"
                                                   "v21: <<<\n 01\n >>>\n"
                                                   "#c"};
        // make sure the test document parses without errors:
        auto source = createTestMemorySource(testDocument);
        auto parser = Parser{};
        DocumentPtr doc;
        REQUIRE_NOTHROW(doc = parser.parseOrThrow(source))
        // shift a control character through the test file.
        for (std::size_t i = 0; i < testDocument.length().toRawValue() + 1; ++i) {
            auto newDocument = el::text::StringEditor{};
            newDocument.append(testDocument.slice(
                el::unit::ByteRange{el::unit::ByteIndex{static_cast<uint32_t>(0)}, el::unit::ByteLength{i}}));
            newDocument.append("\b"_el);
            newDocument.append(
                testDocument.slice(el::unit::ByteRange{el::unit::ByteIndex{i}, el::unit::ByteLength::infinite()}));
            source = createTestMemorySource(el::text::String{newDocument});
            try {
                parser.parseOrThrow(source);
                REQUIRE(false);
            } catch (const ConfError &error) {
                REQUIRE_EQUAL(error.category(), ConfErrorCategory::Character);
            }
        }
    }

    void testLimitExceededVsSyntaxError() {
        // An overlong line that contains syntax errors should report LimitExceeded first
        const el::text::String overlongLine1 =
            el::text::StringEditor::fromCharacter(el::text::Char{U'0'}, el::unit::CpLength{4000U}).append("\n"_el);
        // If the line length is correct, it's just a syntax error.
        const el::text::String exact4000Line2 =
            el::text::StringEditor::fromCharacter(el::text::Char{U'0'}, el::unit::CpLength{3999U}).append("\n"_el);
        // An overlong name should report LimitExceeded before a Syntax error.
        const el::text::String overLongName1 =
            el::text::StringEditor{"["_el}.append(el::text::Char{U'a'}, el::unit::CpLength{101U}).append("_]\n"_el);
        const el::text::String overLongName2 =
            el::text::StringEditor{"["_el}.append(el::text::Char{U'a'}, el::unit::CpLength{100U}).append("_]\n"_el);
        // With 100 characters, the ending `_` is just a syntax error.
        const el::text::String exact100Name =
            el::text::StringEditor{"["_el}.append(el::text::Char{U'a'}, el::unit::CpLength{99U}).append("_]\n"_el);
        // Oversized name-path
        const el::text::String oversizedNamePath1 = el::text::String{"[a.b.c.d.e.f.g.h.i.j.k]\n"_el};
        // Oversized name-path with syntax error.
        const el::text::String oversizedNamePath2 = el::text::String{"[a.b.c.d.e.f.g.h.i.j.k._]\n"_el};
        // Just a syntax error.
        const el::text::String namePathWithSyntaxError = el::text::String{"[a.b.c.d.e.f.g.h.i.j._]\n"_el};
        const auto testCases = std::vector<std::pair<el::text::String, ConfErrorCategory>>{
            {overlongLine1, ConfErrorCategory::LimitExceeded},
            {exact4000Line2, ConfErrorCategory::Syntax},
            {overLongName1, ConfErrorCategory::LimitExceeded},
            {overLongName2, ConfErrorCategory::LimitExceeded},
            {exact100Name, ConfErrorCategory::Syntax},
            {oversizedNamePath1, ConfErrorCategory::LimitExceeded},
            {oversizedNamePath2, ConfErrorCategory::LimitExceeded},
            {namePathWithSyntaxError, ConfErrorCategory::Syntax},
        };
        WITH_CONTEXT(verifyTestCases(testCases));
    }
};
