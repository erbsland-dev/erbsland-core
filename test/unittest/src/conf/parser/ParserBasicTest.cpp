// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserTestHelper.hpp"

#include <erbsland/conf/SourceIdentifier.hpp>
#include <erbsland/conf/StdFormatForConf.hpp>
#include <erbsland/err/ParameterError.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Parser)
class ParserBasicTest final : public UNITTEST_SUBCLASS(ParserTestHelper) {
public:
    MockSourcePtr source;
    std::shared_ptr<Parser> parser;

    void setUp() override {
        source = std::make_shared<MockSource>();
        parser = {};
        doc = {};
    }

    void tearDown() override {
        source = {};
        parser = {};
        doc = {};
    }

    /// Test if the source was used as expected, with one open and one close and no unnecessary reads.
    void verifySequentialRead() {
        REQUIRE_EQUAL(source->actions.count().toSizeT(), source->lines.size() + 2);
        REQUIRE(source->actions.first() == "open"_el);
        REQUIRE(source->actions.last() == "close"_el);
        auto actionIndex = std::size_t{0};
        for (const auto &action : source->actions) {
            if (actionIndex > 0 && actionIndex + 1 < source->actions.count().toSizeT()) {
                REQUIRE(action == "readLine"_el);
            }
            ++actionIndex;
        }
    }

    void testDoNothing() {
        parser = std::make_shared<Parser>();
        parser = {};
    }

    void testNullSource() {
        parser = std::make_shared<Parser>();
        REQUIRE_THROWS_AS(el::err::ParameterError, doc = parser->parseOrThrow({}));
        REQUIRE_THROWS_AS(el::err::ParameterError, doc = parser->parse({}));
    }

    void testEmptyDocument() {
        parser = std::make_shared<Parser>();
        REQUIRE_NOTHROW(doc = parser->parseOrThrow(source));
        WITH_CONTEXT(verifySequentialRead());
        REQUIRE(doc != nullptr);
        REQUIRE(doc->empty());
        const auto location = doc->location();
        REQUIRE_FALSE(location.isUndefined());
        REQUIRE(SourceIdentifier::areEqual(location.sourceIdentifier(), source->identifier()));
        REQUIRE_EQUAL(location.codeLocation().isUndefined(), true);
    }

    void testEmptyWithComments() {
        source->lines = {"# comment\n"_el, "\n"_el, "  \n"_el, "    # comment at end\n"_el};
        parser = std::make_shared<Parser>();
        REQUIRE_NOTHROW(doc = parser->parseOrThrow(source));
        WITH_CONTEXT(verifySequentialRead());
        REQUIRE(doc != nullptr);
        REQUIRE(doc->empty());
    }

    void testEmptyWithMeta() {
        source->lines = {"@version: \"1.0\"\n"_el, "@features: \"core float\"\n"_el, "\n"_el, "# comment at end\n"_el};
        parser = std::make_shared<Parser>();
        REQUIRE_NOTHROW(doc = parser->parseOrThrow(source));
        WITH_CONTEXT(verifySequentialRead());
        REQUIRE(doc != nullptr);
        REQUIRE(doc->empty());
    }

    void testSmallDocument() {
        source->lines = {
            "# A realistic configuration example for ELCL\n"_el,
            "@version: \"1.0\"\n"_el,
            "\n"_el,
            "# a small document\n"_el,
            "--[ main ]--\n"_el,
            "Connect = \"host01.example.com\"\n"_el,
            "Server Port = 1234\n"_el,
            "\n"_el,
            "[ main . Client ]\n"_el,
            "name: \" example client \"\n"_el,
            "Welcome Message: \"\"\"    # The welcome message\n"_el,
            "    Hello user!  \n"_el,
            "    This is the welcome message...\n"_el,
            "    \"\"\"\n"_el,
            "\n"_el,
            "--*[server]*--\n"_el,
            "host: \"host02.example.com\"\n"_el,
            "--*[server]*--\n"_el,
            "host: \"host03.example.com\"\n"_el,
            "port: 0xfffe\n"_el
            "\n"_el,
        };
        parser = std::make_shared<Parser>();
        REQUIRE_NOTHROW(doc = parser->parseOrThrow(source));
        REQUIRE_FALSE(doc->empty());
        auto expectedValueMap = ExpectedValueMap{
            {"main"_el, "SectionWithNames()"_el},
            {"main.connect"_el, "Text(\"host01\\u{2e}example\\u{2e}com\")"_el},
            {"main.server_port"_el, "Integer(1234)"_el},
            {"main.client"_el, "SectionWithNames()"_el},
            {"main.client.name"_el, "Text(\" example client \")"_el},
            {"main.client.welcome_message"_el,
                "Text(\"Hello user!\\u{a}This is the welcome message\\u{2e}\\u{2e}\\u{2e}\")"_el},
            {"server"_el, "SectionList()"_el},
            {"server[0]"_el, "SectionWithNames()"_el},
            {"server[0].host"_el, "Text(\"host02\\u{2e}example\\u{2e}com\")"_el},
            {"server[1]"_el, "SectionWithNames()"_el},
            {"server[1].host"_el, "Text(\"host03\\u{2e}example\\u{2e}com\")"_el},
            {"server[1].port"_el, "Integer(65534)"_el},
        };
        WITH_CONTEXT(verifyValueMap(expectedValueMap));
    }

    void testCharacterEncodingError() {
        source->lines = {
            bytesFromHex(
                "2320 4572 6273 6C61 6E64 2043 6F6E 6669 6775 7261 7469 6F6E 204C 616E 6775 6167 6520 5465 7374 2046 "
                "696C 650A"_el),
            bytesFromHex("5B6D 6169 6E5D 0A"_el),
            bytesFromHex("7661 6C75 653A 2060 EDA080 60"_el), // error: EDA080 = U+D800 = surrogate!
        };
        parser = std::make_shared<Parser>();
        REQUIRE_THROWS_AS(ConfError, doc = parser->parseOrThrow(source));
    }

    void testMixedTextAndRegular() {
        source->lines = {
            "# Erbsland Configuration Language Test File\n"_el,
            "[main]\n"_el,
            "value1 = 1\n"_el,
            "value2 = 2\n"_el,
            "value3 = 3\n"_el,
            "[main.sub_text.\"one\"]\n"_el,
            "value = 10\n"_el,
            "[main.sub_text.\"two\"]\n"_el,
            "value = 20\n"_el,
            "[main.sub_text.\"three\"]\n"_el,
            "value = 30\n"_el,
            "[sub.sub.sub.\"one\"]\n"_el,
            "value = 101\n"_el,
            "[sub.sub.sub.\"two\"]\n"_el,
            "value = 102\n"_el,
            "[sub.sub.sub.\"three\"]\n"_el,
            "value = 103\n"_el,
            "[text.\"one\"]\n"_el,
            "value = 201\n"_el,
            "[text.\"two\"]\n"_el,
            "value = 202\n"_el,
            "[text.\"three\"]\n"_el,
            "value = 203\n"_el,
        };
        parser = std::make_shared<Parser>();
        REQUIRE_NOTHROW(doc = parser->parseOrThrow(source));
        REQUIRE_FALSE(doc->empty());
        auto expectedValueMap = ExpectedValueMap{
            {"main"_el, "SectionWithNames()"_el},
            {"main.value1"_el, "Integer(1)"_el},
            {"main.value2"_el, "Integer(2)"_el},
            {"main.value3"_el, "Integer(3)"_el},
            {"main.sub_text"_el, "SectionWithTexts()"_el},
            {"main.sub_text.\"one\""_el, "SectionWithNames()"_el},
            {"main.sub_text.\"one\".value"_el, "Integer(10)"_el},
            {"main.sub_text.\"two\""_el, "SectionWithNames()"_el},
            {"main.sub_text.\"two\".value"_el, "Integer(20)"_el},
            {"main.sub_text.\"three\""_el, "SectionWithNames()"_el},
            {"main.sub_text.\"three\".value"_el, "Integer(30)"_el},
            {"sub"_el, "IntermediateSection()"_el},
            {"sub.sub"_el, "IntermediateSection()"_el},
            {"sub.sub.sub"_el, "SectionWithTexts()"_el},
            {"sub.sub.sub.\"one\""_el, "SectionWithNames()"_el},
            {"sub.sub.sub.\"one\".value"_el, "Integer(101)"_el},
            {"sub.sub.sub.\"two\""_el, "SectionWithNames()"_el},
            {"sub.sub.sub.\"two\".value"_el, "Integer(102)"_el},
            {"sub.sub.sub.\"three\""_el, "SectionWithNames()"_el},
            {"sub.sub.sub.\"three\".value"_el, "Integer(103)"_el},
            {"text"_el, "SectionWithTexts()"_el},
            {"text.\"one\""_el, "SectionWithNames()"_el},
            {"text.\"one\".value"_el, "Integer(201)"_el},
            {"text.\"two\""_el, "SectionWithNames()"_el},
            {"text.\"two\".value"_el, "Integer(202)"_el},
            {"text.\"three\""_el, "SectionWithNames()"_el},
            {"text.\"three\".value"_el, "Integer(203)"_el},
        };
        WITH_CONTEXT(verifyValueMap(expectedValueMap));
    }
};
