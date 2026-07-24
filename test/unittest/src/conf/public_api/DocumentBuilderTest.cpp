// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/DocumentBuilder.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::conf;
using namespace el::text::literals;

TESTED_TARGETS(DocumentBuilder)
class DocumentBuilderTest final : public el::UnitTest {
public:
    using ExpectedValueMap = std::map<el::text::String, el::text::String>;

    DocumentBuilder builder;
    DocumentPtr doc;

    const Location location{
        {}, el::unit::CodeLocation{el::unit::LineIndex{0U}, el::unit::ColumnIndex{0U}}}; // a placeholder location.

    // NOTE:
    // As the public interface `DocumentBuilder` is just a wrapper around `impl::DocumentBuilder`, this unit test
    // is deliberately limited to this wrapping functionality.
    // The tests check if the calls are correctly passed to the implementation and exceptions are passed back to
    // the user code. See `DocumentBuilderImplTest` and `DocumentBuilderStorageTest` for extensive tests
    // of the builder logic.

    auto additionalErrorMessages() -> std::string override {
        try {
            std::string result;
            if (doc == nullptr) {
                doc = builder.getDocumentAndReset();
            }
            if (doc != nullptr) {
                auto flatMap = doc->toFlatValueMap();
                result += "State of the last document 'doc':\n";
                for (const auto &[namePath, value] : flatMap) {
                    result += el::text::StringConverter{namePath.toText()}.toStdString() + ": " +
                        el::text::StringConverter{value->toTestText()}.toStdString() + "\n";
                }
            }
            return result;
        } catch (...) {
            return "Exception while creating additional error messages.";
        }
    }

    void setUp() override {
        builder.reset();
        doc = nullptr;
    }

    void verifyValueMap(const ExpectedValueMap &expectedValueMap) {
        doc = builder.getDocumentAndReset();
        REQUIRE(doc != nullptr);
        auto flatMap = doc->toFlatValueMap();
        // First, convert and verify all name paths.
        auto actualValues = std::map<el::text::String, el::text::String>{};
        for (auto it = flatMap.begin(); it != flatMap.end(); ++it) {
            auto namePathText = it->first.toText();
            runWithContext(
                SOURCE_LOCATION(),
                [&]() { REQUIRE(expectedValueMap.contains(namePathText)); },
                [&]() -> std::string {
                    return std::format(
                        "Unexpected additional value: {} = {}",
                        el::text::StringConverter{namePathText}.toStdString(),
                        el::text::StringConverter{it->second->toTestText()}.toStdString());
                });
            actualValues[namePathText] = it->second->toTestText();
        }
        // Now test if all expected values are part of the document.
        for (const auto &[expectedNamePath, expectedValueText] : expectedValueMap) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() { REQUIRE(actualValues.contains(expectedNamePath)); },
                [&]() -> std::string {
                    return std::format(
                        "Missing value: {} = {}",
                        el::text::StringConverter{expectedNamePath}.toStdString(),
                        el::text::StringConverter{expectedValueText}.toStdString());
                });
            const auto actualValueText = actualValues[expectedNamePath];
            const auto expectedValueStdText = el::text::StringConverter{expectedValueText}.toStdString();
            if (expectedValueText.startsWith("Float("_el)) {
                // special handling for floating point values.
                const auto expectedFloat = std::stof(expectedValueStdText.substr(6, expectedValueStdText.size() - 7));
                REQUIRE(actualValueText.startsWith("Float("_el));
                const auto actualValueStdText = el::text::StringConverter{actualValueText}.toStdString();
                const auto actualFloat = std::stof(actualValueStdText.substr(6, actualValueStdText.size() - 7));
                REQUIRE_LESS(std::abs(actualFloat - expectedFloat), std::numeric_limits<double>::epsilon());
            } else {
                REQUIRE_EQUAL(actualValueText, expectedValueText);
            }
        }
    }

    void testConstruction() {
        // create and destroy a builder and document locally.
        DocumentBuilder builder;
        builder.addSectionMap("main"_el);
        builder.addValue("main.value_1"_el, 1);
        auto doc = builder.getDocumentAndReset();
        REQUIRE(doc != nullptr);
        REQUIRE(doc->value(NamePath::fromText("main.value_1"_el))->type() == ValueType::Integer);
    }

    void testBasics() {
        REQUIRE_NOTHROW(builder.addSectionMap("main"_el));
        REQUIRE_NOTHROW(builder.addValue("main.value_1"_el, 1));
        REQUIRE_NOTHROW(builder.addValue("value_2"_el, 2));
        auto expectedValueMap = ExpectedValueMap{
            {"main"_el, "SectionWithNames()"_el},
            {"main.value_1"_el, "Integer(1)"_el},
            {"main.value_2"_el, "Integer(2)"_el},
        };
        WITH_CONTEXT(verifyValueMap(expectedValueMap));
    }

    void testEmptyDocument() {
        auto expectedValueMap = ExpectedValueMap{
            // empty map
        };
        WITH_CONTEXT(verifyValueMap(expectedValueMap));
    }

    void testAllTypes() {
        REQUIRE_NOTHROW(builder.addSectionMap("main"_el));
        REQUIRE_NOTHROW(builder.addValue("main.value_1"_el, 12345));
        REQUIRE_NOTHROW(builder.addValue("main.value_2"_el, true));
        REQUIRE_NOTHROW(builder.addValue("main.value_3"_el, 123.456));
        REQUIRE_NOTHROW(builder.addValue("main.value_4"_el, el::text::String{"😆"_el}));
        REQUIRE_NOTHROW(builder.addValue("main.value_5"_el, makeDate(2025, 12, 26)));
        REQUIRE_NOTHROW(builder.addValue("main.value_6"_el, makeTimeWithZone(22, 11, 33, 123456000)));
        REQUIRE_NOTHROW(builder.addValue(
            "main.value_7"_el, el::time::DateTime{makeDate(2025, 12, 26), makeTimeWithZone(22, 11, 33, 123456000)}));
        REQUIRE_NOTHROW(builder.addValue("main.value_8"_el, bytesFromHex("0102aabbcc"_el)));
        REQUIRE_NOTHROW(builder.addValue("main.value_9"_el, el::time::CalendarDelta{el::time::Hours{5}}));
        auto expectedValueMap = ExpectedValueMap{
            {"main"_el, "SectionWithNames()"_el},
            {"main.value_1"_el, "Integer(12345)"_el},
            {"main.value_2"_el, "Boolean(true)"_el},
            {"main.value_3"_el, "Float(123.456)"_el},
            {"main.value_4"_el, "Text(\"\\u{1f606}\")"_el},
            {"main.value_5"_el, "Date(2025-12-26)"_el},
            {"main.value_6"_el, "Time(22:11:33.123456z)"_el},
            {"main.value_7"_el, "DateTime(2025-12-26 22:11:33.123456z)"_el},
            {"main.value_8"_el, "Bytes(0102aabbcc)"_el},
            {"main.value_9"_el, "TimeDelta(5h)"_el},
        };
        WITH_CONTEXT(verifyValueMap(expectedValueMap));
    }

    void testAllTypes2() {
        REQUIRE_NOTHROW(builder.addSectionMap("main"_el));
        REQUIRE_NOTHROW(builder.addInteger("main.value_1"_el, 12345));
        REQUIRE_NOTHROW(builder.addBoolean("main.value_2"_el, true));
        REQUIRE_NOTHROW(builder.addFloat("main.value_3"_el, 123.456));
        REQUIRE_NOTHROW(builder.addText("main.value_4"_el, el::text::String{"😆"_el}));
        REQUIRE_NOTHROW(builder.addDate("main.value_5"_el, makeDate(2025, 12, 26)));
        REQUIRE_NOTHROW(builder.addTimeWithZone("main.value_6"_el, makeTimeWithZone(22, 11, 33, 123456000)));
        REQUIRE_NOTHROW(builder.addDateTime(
            "main.value_7"_el, el::time::DateTime{makeDate(2025, 12, 26), makeTimeWithZone(22, 11, 33, 123456000)}));
        REQUIRE_NOTHROW(builder.addBytes("main.value_8"_el, bytesFromHex("0102aabbcc"_el)));
        REQUIRE_NOTHROW(builder.addCalendarDelta("main.value_9"_el, el::time::CalendarDelta{el::time::Hours{5}}));
        REQUIRE_NOTHROW(builder.addRegEx("main.value_10"_el, el::re::RegEx::compile("abc"_el)));
        REQUIRE_NOTHROW(builder.addTime("main.value_11"_el, makeTime(22, 11, 33, 123456000)));
        auto expectedValueMap = ExpectedValueMap{
            {"main"_el, "SectionWithNames()"_el},
            {"main.value_1"_el, "Integer(12345)"_el},
            {"main.value_2"_el, "Boolean(true)"_el},
            {"main.value_3"_el, "Float(123.456)"_el},
            {"main.value_4"_el, "Text(\"\\u{1f606}\")"_el},
            {"main.value_5"_el, "Date(2025-12-26)"_el},
            {"main.value_6"_el, "Time(22:11:33.123456z)"_el},
            {"main.value_7"_el, "DateTime(2025-12-26 22:11:33.123456z)"_el},
            {"main.value_8"_el, "Bytes(0102aabbcc)"_el},
            {"main.value_9"_el, "TimeDelta(5h)"_el},
            {"main.value_10"_el, "RegEx(\"abc\")"_el},
            {"main.value_11"_el, "Time(22:11:33.123456)"_el},
        };
        WITH_CONTEXT(verifyValueMap(expectedValueMap));
    }

    void testNullRegExIsRejected() {
        REQUIRE_NOTHROW(builder.addSectionMap("main"_el));
        REQUIRE_THROWS(builder.addRegEx("main.value"_el, {}));
    }

    void testAllTypes3() {
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main"_el)));
        REQUIRE_NOTHROW(builder.addInteger(NamePath::fromText("main.value_1"_el), 12345));
        REQUIRE_NOTHROW(builder.addBoolean(NamePath::fromText("main.value_2"_el), true));
        REQUIRE_NOTHROW(builder.addFloat(NamePath::fromText("main.value_3"_el), 123.456));
        REQUIRE_NOTHROW(builder.addText(NamePath::fromText("main.value_4"_el), el::text::String{"😆"_el}));
        REQUIRE_NOTHROW(builder.addDate(NamePath::fromText("main.value_5"_el), makeDate(2025, 12, 26)));
        REQUIRE_NOTHROW(
            builder.addTimeWithZone(NamePath::fromText("main.value_6"_el), makeTimeWithZone(22, 11, 33, 123456000)));
        REQUIRE_NOTHROW(builder.addDateTime(
            NamePath::fromText("main.value_7"_el),
            el::time::DateTime{makeDate(2025, 12, 26), makeTimeWithZone(22, 11, 33, 123456000)}));
        REQUIRE_NOTHROW(builder.addBytes(NamePath::fromText("main.value_8"_el), bytesFromHex("0102aabbcc"_el)));
        REQUIRE_NOTHROW(builder.addCalendarDelta(
            NamePath::fromText("main.value_9"_el), el::time::CalendarDelta{el::time::Hours{5}}));
        REQUIRE_NOTHROW(builder.addRegEx(NamePath::fromText("main.value_10"_el), el::re::RegEx::compile("abc"_el)));
        auto expectedValueMap = ExpectedValueMap{
            {"main"_el, "SectionWithNames()"_el},
            {"main.value_1"_el, "Integer(12345)"_el},
            {"main.value_2"_el, "Boolean(true)"_el},
            {"main.value_3"_el, "Float(123.456)"_el},
            {"main.value_4"_el, "Text(\"\\u{1f606}\")"_el},
            {"main.value_5"_el, "Date(2025-12-26)"_el},
            {"main.value_6"_el, "Time(22:11:33.123456z)"_el},
            {"main.value_7"_el, "DateTime(2025-12-26 22:11:33.123456z)"_el},
            {"main.value_8"_el, "Bytes(0102aabbcc)"_el},
            {"main.value_9"_el, "TimeDelta(5h)"_el},
            {"main.value_10"_el, "RegEx(\"abc\")"_el},
        };
        WITH_CONTEXT(verifyValueMap(expectedValueMap));
    }

    void testNestedSections() {
        REQUIRE_NOTHROW(builder.addSectionMap("main"_el));
        REQUIRE_NOTHROW(builder.addSectionMap("main.server"_el));
        REQUIRE_NOTHROW(builder.addSectionMap("main.server.filter"_el));
        REQUIRE_NOTHROW(builder.addValue("value_1"_el, 1));
        REQUIRE_NOTHROW(builder.addSectionMap("main.client"_el));
        REQUIRE_NOTHROW(builder.addValue("value_2"_el, 2));
        REQUIRE_NOTHROW(builder.addSectionMap("main.server.handler"_el));
        REQUIRE_NOTHROW(builder.addValue("value_3"_el, 3));
        REQUIRE_NOTHROW(builder.addSectionMap("web"_el));
        REQUIRE_NOTHROW(builder.addValue("value_4"_el, 4));
        REQUIRE_NOTHROW(builder.addSectionMap("web.pages"_el));
        REQUIRE_NOTHROW(builder.addValue("value_5"_el, 5));
        REQUIRE_NOTHROW(builder.addValue("main.server.value_6"_el, 6));
        REQUIRE_NOTHROW(builder.addValue("main.client.value_7"_el, 7));
        REQUIRE_NOTHROW(builder.addValue("main.server.handler.value_8"_el, 8));
        auto expectedValueMap = ExpectedValueMap{
            {"main"_el, "SectionWithNames()"_el},
            {"main.server"_el, "SectionWithNames()"_el},
            {"main.server.value_6"_el, "Integer(6)"_el},
            {"main.server.filter"_el, "SectionWithNames()"_el},
            {"main.server.filter.value_1"_el, "Integer(1)"_el},
            {"main.client"_el, "SectionWithNames()"_el},
            {"main.client.value_2"_el, "Integer(2)"_el},
            {"main.client.value_7"_el, "Integer(7)"_el},
            {"main.server.handler"_el, "SectionWithNames()"_el},
            {"main.server.handler.value_3"_el, "Integer(3)"_el},
            {"main.server.handler.value_8"_el, "Integer(8)"_el},
            {"web"_el, "SectionWithNames()"_el},
            {"web.value_4"_el, "Integer(4)"_el},
            {"web.pages"_el, "SectionWithNames()"_el},
            {"web.pages.value_5"_el, "Integer(5)"_el},
        };
        WITH_CONTEXT(verifyValueMap(expectedValueMap));
    }

    void testSectionList() {
        REQUIRE_NOTHROW(builder.addSectionMap("main"_el));
        REQUIRE_NOTHROW(builder.addSectionList("main.server"_el));
        REQUIRE_NOTHROW(builder.addValue("value_1"_el, 1));
        REQUIRE_NOTHROW(builder.addSectionList(NamePath::fromText("main.server"_el)));
        REQUIRE_NOTHROW(builder.addValue("value_2"_el, 2));
        REQUIRE_NOTHROW(builder.addSectionList("main.server"_el));
        REQUIRE_NOTHROW(builder.addValue("value_3"_el, 3));
        REQUIRE_NOTHROW(builder.addValue("main.server.value_4"_el, 4));
        REQUIRE_NOTHROW(builder.addSectionMap("main.server.details"_el));
        auto expectedValueMap = ExpectedValueMap{
            {"main"_el, "SectionWithNames()"_el},
            {"main.server"_el, "SectionList()"_el},
            {"main.server[0]"_el, "SectionWithNames()"_el},
            {"main.server[0].value_1"_el, "Integer(1)"_el},
            {"main.server[1]"_el, "SectionWithNames()"_el},
            {"main.server[1].value_2"_el, "Integer(2)"_el},
            {"main.server[2]"_el, "SectionWithNames()"_el},
            {"main.server[2].value_3"_el, "Integer(3)"_el},
            {"main.server[2].value_4"_el, "Integer(4)"_el},
            {"main.server[2].details"_el, "SectionWithNames()"_el},
        };
        WITH_CONTEXT(verifyValueMap(expectedValueMap));
    }

    void testIntermediateConversion() {
        REQUIRE_NOTHROW(builder.addSectionMap("one.two.three.four"_el));
        // one, two and three are intermediate sections at this point
        REQUIRE_NOTHROW(builder.addSectionMap("one"_el));
        REQUIRE_NOTHROW(builder.addSectionMap("one.two.three"_el));
        auto expectedValueMap = ExpectedValueMap{
            {"one"_el, "SectionWithNames()"_el},
            {"one.two"_el, "IntermediateSection()"_el},
            {"one.two.three"_el, "SectionWithNames()"_el},
            {"one.two.three.four"_el, "SectionWithNames()"_el},
        };
        WITH_CONTEXT(verifyValueMap(expectedValueMap));
    }

    void testCommonErrors() {
        // Adding values before any section is created.
        REQUIRE_THROWS_AS(ConfError, builder.addValue("main"_el, 1));
        REQUIRE_THROWS_AS(ConfError, builder.addValue("main.server"_el, 1));
        // Invalid name paths.
        REQUIRE_THROWS_AS(ConfError, builder.addSectionMap(NamePath{}));
        REQUIRE_THROWS_AS(ConfError, builder.addSectionMap("main[5]"_el));
        REQUIRE_THROWS_AS(ConfError, builder.addSectionMap("main.\"\"[5]"_el));
        REQUIRE_THROWS_AS(ConfError, builder.addSectionList(NamePath{}));
        REQUIRE_THROWS_AS(ConfError, builder.addSectionList("main[5]"_el));
        REQUIRE_THROWS_AS(ConfError, builder.addSectionList("main.\"\"[5]"_el));
        REQUIRE_THROWS_AS(ConfError, builder.addSectionList("main.\"text\""_el));
        REQUIRE_THROWS_AS(ConfError, builder.addValue(NamePath{}, 1));
        REQUIRE_THROWS_AS(ConfError, builder.addValue("main[1]"_el, 1));
        REQUIRE_THROWS_AS(ConfError, builder.addValue("main.\"\"[2]"_el, 1));
        // Adding a value to a non-existing section.
        REQUIRE_NOTHROW(builder.addSectionMap("main.server"_el));
        REQUIRE_THROWS_AS(ConfError, builder.addValue("main.one.two.three"_el, 1));
        // after all these errors, no additional elements should be created.
        auto expectedValueMap = ExpectedValueMap{
            {"main"_el, "IntermediateSection()"_el},
            {"main.server"_el, "SectionWithNames()"_el},
        };
        verifyValueMap(expectedValueMap);
    }
};
