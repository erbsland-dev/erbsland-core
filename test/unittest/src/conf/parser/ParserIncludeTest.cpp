// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserTestHelper.hpp"

#include <erbsland/conf/StdFormat.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Parser)
class ParserIncludeTest final : public UNITTEST_SUBCLASS(ParserTestHelper) {
public:
    class NullSourceResolver final : public SourceResolver {
    public:
        auto resolve(const SourceResolverContext &) -> SourceListPtr override {
            return std::make_shared<SourceList>(SourceList{nullptr});
        }
    };

    void tearDown() override {
        cleanUpTestFileDirectory();
        doc = {};
    }

    void expectParserError(
        const std::filesystem::path &path,
        const ConfErrorCategory errorCategory,
        el::text::String expectedWordInErrorMessage = {}) {
        try {
            Parser parser;
            doc = parser.parseOrThrow(Source::fromFile(el::path::Path{path}));
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), errorCategory);
            if (!expectedWordInErrorMessage.isEmpty()) {
                const auto success = error.description().contains(expectedWordInErrorMessage);
                if (!success) {
                    consoleWriteLine(el::text::StringConverter{error.toString()}.toStdString());
                }
                REQUIRE(success);
            }
        }
    }

    void testBasicInclude() {
        const auto mainFile = createTestFile(
            "config/main.elcl",
            "[main]\n"_el
            "value 01 = 5001\n"_el
            "value 02 = 5002\n\n"_el
            "@include: \"sub_01/config_02.elcl\"\n"_el
            "[second]\n"_el
            "value 03 = 6001\n"_el
            "@include: \"sub_02/*.elcl\"\n"_el);
        createTestFile(
            "config/sub_01/config_02.elcl",
            "[sub 01]\n"_el
            "value 04 = 7001\n"_el
            "value 05 = 7002\n"_el);
        createTestFile(
            "config/sub_02/config_03.elcl",
            "[sub 02]\n"_el
            "value 06 = 8001\n"_el
            "value 07 = 8002\n"_el);
        createTestFile(
            "config/sub_02/config_04.elcl",
            "[sub 03]\n"_el
            "value 08 = 9001\n"
            "value 09 = 9002\n"_el);
        Parser parser;
        REQUIRE_NOTHROW(doc = parser.parseOrThrow(Source::fromFile(el::path::Path{mainFile})));
        REQUIRE(doc != nullptr);
        auto expectedValueMap = ExpectedValueMap{
            {"main"_el, "SectionWithNames()"_el},
            {"main.value_01"_el, "Integer(5001)"_el},
            {"main.value_02"_el, "Integer(5002)"_el},
            {"second"_el, "SectionWithNames()"_el},
            {"second.value_03"_el, "Integer(6001)"_el},
            {"sub_01"_el, "SectionWithNames()"_el},
            {"sub_01.value_04"_el, "Integer(7001)"_el},
            {"sub_01.value_05"_el, "Integer(7002)"_el},
            {"sub_02"_el, "SectionWithNames()"_el},
            {"sub_02.value_06"_el, "Integer(8001)"_el},
            {"sub_02.value_07"_el, "Integer(8002)"_el},
            {"sub_03"_el, "SectionWithNames()"_el},
            {"sub_03.value_08"_el, "Integer(9001)"_el},
            {"sub_03.value_09"_el, "Integer(9002)"_el},
        };
        WITH_CONTEXT(verifyValueMap(expectedValueMap));
    }

    void testRecursiveIncludeAndCorrectOrder() {
        const auto mainFile = createTestFile(
            "config/main.elcl",
            "*[block]\n"_el
            "value 01 = 123\n"_el
            "@include: \"sub/**/*.elcl\"\n"_el);
        createTestFile(
            "config/sub/a.elcl",
            "*[block]\n"_el
            "value 02 = 123\n"_el);
        createTestFile(
            "config/sub/b.elcl",
            "*[block]\n"_el
            "value 03 = 123\n"_el);
        createTestFile(
            "config/sub/a/a.elcl",
            "*[block]\n"_el
            "value 04 = 123\n"_el);
        createTestFile(
            "config/sub/a/b.elcl",
            "*[block]\n"_el
            "value 05 = 123\n"_el);
        createTestFile(
            "config/sub/b/a.elcl",
            "*[block]\n"_el
            "value 06 = 123\n"_el);
        createTestFile(
            "config/sub/b/b.elcl",
            "*[block]\n"_el
            "value 07 = 123\n"_el);
        Parser parser;
        REQUIRE_NOTHROW(doc = parser.parseOrThrow(Source::fromFile(el::path::Path{mainFile})));
        REQUIRE(doc != nullptr);
        auto expectedValueMap = ExpectedValueMap{
            {"block"_el, "SectionList()"_el},
            {"block[0]"_el, "SectionWithNames()"_el},
            {"block[0].value_01"_el, "Integer(123)"_el},
            {"block[1]"_el, "SectionWithNames()"_el},
            {"block[1].value_02"_el, "Integer(123)"_el},
            {"block[2]"_el, "SectionWithNames()"_el},
            {"block[2].value_03"_el, "Integer(123)"_el},
            {"block[3]"_el, "SectionWithNames()"_el},
            {"block[3].value_04"_el, "Integer(123)"_el},
            {"block[4]"_el, "SectionWithNames()"_el},
            {"block[4].value_05"_el, "Integer(123)"_el},
            {"block[5]"_el, "SectionWithNames()"_el},
            {"block[5].value_06"_el, "Integer(123)"_el},
            {"block[6]"_el, "SectionWithNames()"_el},
            {"block[6].value_07"_el, "Integer(123)"_el},
        };
        WITH_CONTEXT(verifyValueMap(expectedValueMap));
    }

    void testErrorIncludeNotFound() {
        const auto mainFile = createTestFile("config/main.elcl", "@include: \"config02.elcl\"\n"_el);
        WITH_CONTEXT(expectParserError(mainFile, ConfErrorCategory::Syntax, "not find"_el));
    }

    void testNoWildcardMatches1() {
        const auto mainFile = createTestFile("config/main.elcl", "@include: \"none*.elcl\"\n"_el);
        Parser parser;
        REQUIRE_NOTHROW(doc = parser.parseOrThrow(Source::fromFile(el::path::Path{mainFile})));
        REQUIRE(doc != nullptr);
    }

    void testNoWildcardMatches2() {
        const auto mainFile = createTestFile("config/main.elcl", "@include: \"**/none.elcl\"\n"_el);
        Parser parser;
        REQUIRE_NOTHROW(doc = parser.parseOrThrow(Source::fromFile(el::path::Path{mainFile})));
        REQUIRE(doc != nullptr);
    }

    void testNullSourceFromResolver() {
        Parser parser;
        parser.setSourceResolver(std::make_shared<NullSourceResolver>());
        try {
            doc = parser.parseTextOrThrow("@include: \"anything\"\n"_el);
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), ConfErrorCategory::Syntax);
            REQUIRE(error.description().contains("null source"_el));
        }
    }

    void testErrorLoop() {
        const auto mainFile = createTestFile("config/main.elcl", "@include: \"config02.elcl\"\n"_el);
        createTestFile("config/config02.elcl", "@include: \"config03.elcl\"\n"_el);
        createTestFile("config/config03.elcl", "@include: \"main.elcl\"\n"_el);
        WITH_CONTEXT(expectParserError(mainFile, ConfErrorCategory::Syntax, "loop"_el));
    }

    void testErrorNestingLimit() {
        const auto mainFile = createTestFile("config/main.elcl", "@include: \"config02.elcl\"\n"_el);
        createTestFile("config/config02.elcl", "@include: \"config03.elcl\"\n"_el);
        createTestFile("config/config03.elcl", "@include: \"config04.elcl\"\n"_el);
        createTestFile("config/config04.elcl", "@include: \"config05.elcl\"\n"_el);
        createTestFile("config/config05.elcl", "@include: \"config06.elcl\"\n"_el);
        createTestFile("config/config06.elcl", "@include: \"config07.elcl\"\n"_el);
        createTestFile("config/config07.elcl", "[main]\n"_el);
        WITH_CONTEXT(expectParserError(mainFile, ConfErrorCategory::LimitExceeded, "nesting"_el));
    }
};
