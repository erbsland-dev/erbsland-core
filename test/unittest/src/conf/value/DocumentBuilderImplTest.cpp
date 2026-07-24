// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/impl/utilities/InternalError.hpp>
#include <erbsland/conf/impl/value/DocumentBuilder.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::conf;
using namespace el::text::literals;
using impl::DocumentBuilder;

TESTED_TARGETS(DocumentBuilder)
class DocumentBuilderImplTest final : public el::UnitTest {
public:
    using ExpectedValueMap = std::map<el::text::String, el::text::String>;

    DocumentBuilder builder;
    DocumentPtr doc;

    const Location location{
        {}, el::unit::CodeLocation{el::unit::LineIndex{0U}, el::unit::ColumnIndex{0U}}}; // a placeholder location.

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
        // As we will use a member variable for all following tests, this test constructs and destructs
        // an instance to verify the memory handling.
        std::weak_ptr<impl::Value> weakValue;
        {
            impl::DocumentBuilder builder;
            builder.addSectionMap(NamePath::fromText("main"_el), location);
            auto value = impl::Value::createInteger(1);
            weakValue = value;
            builder.addValue(NamePath::fromText("main.value_1"_el), value, location);
            auto doc = builder.getDocumentAndReset();
            REQUIRE(doc != nullptr);
            REQUIRE(doc->value(NamePath::fromText("main.value_1"_el))->type() == ValueType::Integer);
            // destruct...
        }
        REQUIRE(weakValue.expired()); // no leaks.
    }

    void testBasics() {
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main"_el), location));
        REQUIRE_NOTHROW(
            builder.addValue(NamePath::fromText("main.value_1"_el), impl::Value::createInteger(1), location));
        REQUIRE_NOTHROW(builder.addValue(NamePath::fromText("value_2"_el), impl::Value::createInteger(2), location));
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
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main"_el), location));
        REQUIRE_NOTHROW(
            builder.addValue(NamePath::fromText("main.value_1"_el), impl::Value::createInteger(12345), location));
        REQUIRE_NOTHROW(
            builder.addValue(NamePath::fromText("main.value_2"_el), impl::Value::createBoolean(true), location));
        REQUIRE_NOTHROW(
            builder.addValue(NamePath::fromText("main.value_3"_el), impl::Value::createFloat(123.456), location));
        REQUIRE_NOTHROW(
            builder.addValue(NamePath::fromText("main.value_4"_el), impl::Value::createText("😆"_el), location));
        REQUIRE_NOTHROW(builder.addValue(
            NamePath::fromText("main.value_5"_el), impl::Value::createDate(makeDate(2025, 12, 26)), location));
        REQUIRE_NOTHROW(builder.addValue(
            NamePath::fromText("main.value_6"_el),
            impl::Value::createTimeWithZone(makeTimeWithZone(22, 11, 33, 123456000)),
            location));
        REQUIRE_NOTHROW(builder.addValue(
            NamePath::fromText("main.value_7"_el),
            impl::Value::createDateTime(
                el::time::DateTime{makeDate(2025, 12, 26), makeTimeWithZone(22, 11, 33, 123456000)}),
            location));
        REQUIRE_NOTHROW(builder.addValue(
            NamePath::fromText("main.value_8"_el), impl::Value::createBytes(bytesFromHex("0102aabbcc"_el)), location));
        REQUIRE_NOTHROW(builder.addValue(
            NamePath::fromText("main.value_9"_el),
            impl::Value::createCalendarDelta(el::time::CalendarDelta{el::time::Hours{5}}),
            location));
        REQUIRE_NOTHROW(builder.addValue(
            NamePath::fromText("main.value_10"_el),
            impl::Value::createRegEx(el::re::RegEx::compile("abc"_el)),
            location));
        std::vector<impl::ValuePtr> valueList;
        valueList.emplace_back(impl::Value::createInteger(1));
        valueList.emplace_back(impl::Value::createInteger(2));
        valueList.emplace_back(impl::Value::createInteger(3));
        REQUIRE_NOTHROW(builder.addValue(
            NamePath::fromText("main.value_11"_el), impl::Value::createValueList(std::move(valueList)), location));
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
            {"main.value_11"_el, "ValueList()"_el},
            {"main.value_11[0]"_el, "Integer(1)"_el},
            {"main.value_11[1]"_el, "Integer(2)"_el},
            {"main.value_11[2]"_el, "Integer(3)"_el},
        };
        WITH_CONTEXT(verifyValueMap(expectedValueMap));
    }

    void testNestedSections() {
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main"_el), location));
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main.server"_el), location));
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main.server.filter"_el), location));
        REQUIRE_NOTHROW(builder.addValue(NamePath::fromText("value_1"_el), impl::Value::createInteger(1), location));
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main.client"_el), location));
        REQUIRE_NOTHROW(builder.addValue(NamePath::fromText("value_2"_el), impl::Value::createInteger(2), location));
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main.server.handler"_el), location));
        REQUIRE_NOTHROW(builder.addValue(NamePath::fromText("value_3"_el), impl::Value::createInteger(3), location));
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("web"_el), location));
        REQUIRE_NOTHROW(builder.addValue(NamePath::fromText("value_4"_el), impl::Value::createInteger(4), location));
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("web.pages"_el), location));
        REQUIRE_NOTHROW(builder.addValue(NamePath::fromText("value_5"_el), impl::Value::createInteger(5), location));
        REQUIRE_NOTHROW(
            builder.addValue(NamePath::fromText("main.server.value_6"_el), impl::Value::createInteger(6), location));
        REQUIRE_NOTHROW(
            builder.addValue(NamePath::fromText("main.client.value_7"_el), impl::Value::createInteger(7), location));
        REQUIRE_NOTHROW(builder.addValue(
            NamePath::fromText("main.server.handler.value_8"_el), impl::Value::createInteger(8), location));
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
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main"_el), location));
        REQUIRE_NOTHROW(builder.addSectionList(NamePath::fromText("main.server"_el), location));
        REQUIRE_NOTHROW(builder.addValue(NamePath::fromText("value_1"_el), impl::Value::createInteger(1), location));
        REQUIRE_NOTHROW(builder.addSectionList(NamePath::fromText("main.server"_el), location));
        REQUIRE_NOTHROW(builder.addValue(NamePath::fromText("value_2"_el), impl::Value::createInteger(2), location));
        REQUIRE_NOTHROW(builder.addSectionList(NamePath::fromText("main.server"_el), location));
        REQUIRE_NOTHROW(builder.addValue(NamePath::fromText("value_3"_el), impl::Value::createInteger(3), location));
        REQUIRE_NOTHROW(
            builder.addValue(NamePath::fromText("main.server.value_4"_el), impl::Value::createInteger(4), location));
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main.server.details"_el), location));
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
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("one.two.three.four"_el), location));
        // one, two and three are intermediate sections at this point
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("one"_el), location));
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("one.two.three"_el), location));
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
        REQUIRE_THROWS_AS(
            ConfError, builder.addValue(NamePath::fromText("main"_el), impl::Value::createInteger(1), location));
        REQUIRE_THROWS_AS(
            ConfError, builder.addValue(NamePath::fromText("main.server"_el), impl::Value::createInteger(1), location));
        // Invalid name paths.
        REQUIRE_THROWS_AS(ConfError, builder.addSectionMap(NamePath{}, location));
        REQUIRE_THROWS_AS(ConfError, builder.addSectionMap(NamePath::fromText("main[5]"_el), location));
        REQUIRE_THROWS_AS(ConfError, builder.addSectionMap(NamePath::fromText("main.\"\"[5]"_el), location));
        REQUIRE_THROWS_AS(ConfError, builder.addSectionList(NamePath{}, location));
        REQUIRE_THROWS_AS(ConfError, builder.addSectionList(NamePath::fromText("main[5]"_el), location));
        REQUIRE_THROWS_AS(ConfError, builder.addSectionList(NamePath::fromText("main.\"\"[5]"_el), location));
        REQUIRE_THROWS_AS(ConfError, builder.addSectionList(NamePath::fromText("main.\"text\""_el), location));
        REQUIRE_THROWS_AS(ConfError, builder.addValue(NamePath{}, impl::Value::createInteger(1), location));
        REQUIRE_THROWS_AS(
            ConfError, builder.addValue(NamePath::fromText("main[1]"_el), impl::Value::createInteger(1), location));
        REQUIRE_THROWS_AS(
            ConfError,
            builder.addValue(NamePath::fromText("main.\"\"[2]"_el), impl::Value::createInteger(1), location));
        // Adding a value to a non-existing section.
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main.server"_el), location));
        REQUIRE_THROWS_AS(
            ConfError,
            builder.addValue(NamePath::fromText("main.one.two.three"_el), impl::Value::createInteger(1), location));
        // after all these errors, no additional elements should be created.
        auto expectedValueMap = ExpectedValueMap{
            {"main"_el, "IntermediateSection()"_el},
            {"main.server"_el, "SectionWithNames()"_el},
        };
        verifyValueMap(expectedValueMap);
    }

    void testNameConflicts() {
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main.server"_el), location));
        REQUIRE_THROWS_AS(ConfError, builder.addSectionMap(NamePath::fromText("main.server"_el), location));
        REQUIRE_THROWS_AS(ConfError, builder.addSectionList(NamePath::fromText("main"_el), location));
        REQUIRE_THROWS_AS(ConfError, builder.addSectionList(NamePath::fromText("main.server"_el), location));
        REQUIRE_THROWS_AS(
            ConfError, builder.addValue(NamePath::fromText("main.server"_el), impl::Value::createInteger(1), location));
        // after all errors, only the initial two elements should exist.
        auto expectedValueMap = ExpectedValueMap{
            {"main"_el, "IntermediateSection()"_el},
            {"main.server"_el, "SectionWithNames()"_el},
        };
        verifyValueMap(expectedValueMap);
    }

    void testNameConflicts2() {
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main"_el), location));
        REQUIRE_NOTHROW(
            builder.addValue(NamePath::fromText("main.server"_el), impl::Value::createInteger(1), location));
        REQUIRE_THROWS_AS(ConfError, builder.addSectionMap(NamePath::fromText("main.server"_el), location));
        REQUIRE_THROWS_AS(ConfError, builder.addSectionMap(NamePath::fromText("main.server.section"_el), location));
        REQUIRE_THROWS_AS(ConfError, builder.addSectionList(NamePath::fromText("main.server"_el), location));
        REQUIRE_THROWS_AS(ConfError, builder.addSectionList(NamePath::fromText("main.server.section"_el), location));
        REQUIRE_THROWS_AS(
            ConfError,
            builder.addValue(NamePath::fromText("main.server.value"_el), impl::Value::createInteger(1), location));
        // after all errors, only the initial two elements should exist.
        auto expectedValueMap = ExpectedValueMap{
            {"main"_el, "SectionWithNames()"_el},
            {"main.server"_el, "Integer(1)"_el},
        };
        verifyValueMap(expectedValueMap);
    }

    void testTextNames() {
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main.text"_el), location));
        REQUIRE_NOTHROW(
            builder.addValue(NamePath::fromText("main.text.\"Value 1\""_el), impl::Value::createInteger(1), location));
        REQUIRE_NOTHROW(
            builder.addValue(NamePath::fromText("main.text.\"Value 2\""_el), impl::Value::createInteger(2), location));
        REQUIRE_NOTHROW(
            builder.addValue(NamePath::fromText("main.text.\"Value 3\""_el), impl::Value::createInteger(3), location));
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main.sub.\"Section 1\""_el), location));
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main.sub.\"Section 2\""_el), location));
        REQUIRE_NOTHROW(builder.addValue(NamePath::fromText("value_4"_el), impl::Value::createInteger(4), location));
        auto expectedValueMap = ExpectedValueMap{
            {"main"_el, "IntermediateSection()"_el},
            {"main.text"_el, "SectionWithTexts()"_el},
            {"main.text.\"Value 1\""_el, "Integer(1)"_el},
            {"main.text.\"Value 2\""_el, "Integer(2)"_el},
            {"main.text.\"Value 3\""_el, "Integer(3)"_el},
            {"main.sub"_el, "SectionWithTexts()"_el},
            {"main.sub.\"Section 1\""_el, "SectionWithNames()"_el},
            {"main.sub.\"Section 2\""_el, "SectionWithNames()"_el},
            {"main.sub.\"Section 2\".value_4"_el, "Integer(4)"_el},
        };
        verifyValueMap(expectedValueMap);
    }

    void testTextNameSectionErrors() {
        // Text sections must not be added to the document itself.
        REQUIRE_THROWS_AS(ConfError, builder.addSectionMap(NamePath::fromText("\"Text\""_el), location));
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main.text"_el), location));
        // Must not mix text names with regular names.
        REQUIRE_THROWS_AS(ConfError, builder.addSectionMap(NamePath::fromText("main.\"Text\""_el), location));
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main.text.\"Text\""_el), location));
        // Must not mix text names with regular names.
        REQUIRE_THROWS_AS(ConfError, builder.addSectionMap(NamePath::fromText("main.text.regular"_el), location));
        // Subsections aren't allowed for text sections.
        REQUIRE_THROWS_AS(
            ConfError, builder.addSectionMap(NamePath::fromText("main.text.\"Text\".regular"_el), location));
        // Section list must not have text names.
        REQUIRE_THROWS_AS(ConfError, builder.addSectionList(NamePath::fromText("main.text.\"Text2\""_el), location));
        // make sure only valid elements got added.
        auto expectedValueMap = ExpectedValueMap{
            {"main"_el, "IntermediateSection()"_el},
            {"main.text"_el, "SectionWithTexts()"_el},
            {"main.text.\"Text\""_el, "SectionWithNames()"_el},
        };
        verifyValueMap(expectedValueMap);
    }

    void testTextNameValueErrors() {
        // Regular values must not be added to the root, this is also true for text names.
        REQUIRE_THROWS_AS(
            ConfError, builder.addValue(NamePath::fromText("\"Text\""_el), impl::Value::createInteger(1), location));
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main"_el), location));
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main.text"_el), location));
        // Mixing regular with text names is not allowed.
        REQUIRE_THROWS_AS(
            ConfError,
            builder.addValue(NamePath::fromText("main.\"Text\""_el), impl::Value::createInteger(1), location));
        REQUIRE_NOTHROW(
            builder.addValue(NamePath::fromText("main.text.\"Value 1\""_el), impl::Value::createInteger(1), location));
        // Mixing regular with text names is not allowed.
        REQUIRE_THROWS_AS(
            ConfError,
            builder.addValue(NamePath::fromText("main.text.value_2"_el), impl::Value::createInteger(1), location));
        // make sure only valid elements got added.
        auto expectedValueMap = ExpectedValueMap{
            {"main"_el, "SectionWithNames()"_el},
            {"main.text"_el, "SectionWithTexts()"_el},
            {"main.text.\"Value 1\""_el, "Integer(1)"_el},
        };
        verifyValueMap(expectedValueMap);
    }

    void testAddingInvalidValueTypes() {
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main"_el), location));
        // 'addValue' must only accept values.
        REQUIRE_THROWS(
            builder.addValue(NamePath::fromText("main.section"_el), impl::Value::createSectionWithNames(), location));
        REQUIRE_THROWS(builder.addValue(
            NamePath::fromText("main.section"_el), impl::Value::createIntermediateSection(), location));
        REQUIRE_THROWS(
            builder.addValue(NamePath::fromText("main.section"_el), impl::Value::createSectionList(), location));
        REQUIRE_THROWS(
            builder.addValue(NamePath::fromText("main.section"_el), impl::Value::createSectionWithTexts(), location));
        REQUIRE_THROWS(builder.addValue(NamePath::fromText("main.section"_el), nullptr, location));

        struct UndefinedValue : impl::Value {
            UndefinedValue() = default;
            [[nodiscard]] auto type() const noexcept -> ValueType override { return ValueType::Undefined; }
            [[nodiscard]] auto deepCopy() const -> impl::ValuePtr override {
                impl::throwInternalError("not implemented"_el);
            }
        };
        REQUIRE_THROWS(
            builder.addValue(NamePath::fromText("main.section"_el), std::make_shared<UndefinedValue>(), location));
    }

    void testAddingIntUsingTemplates() {
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main"_el), location));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_1"_el), static_cast<int8_t>(1)));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_2"_el), static_cast<uint8_t>(2)));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_3"_el), static_cast<int16_t>(3)));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_4"_el), static_cast<uint16_t>(4)));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_5"_el), static_cast<int32_t>(5)));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_6"_el), static_cast<uint32_t>(6)));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_7"_el), static_cast<int64_t>(7)));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_8"_el), static_cast<uint64_t>(8)));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_9"_el), static_cast<int>(9)));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_10"_el), static_cast<unsigned int>(10)));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_11"_el), static_cast<long>(11)));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_12"_el), static_cast<unsigned long>(12)));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_13"_el), static_cast<short>(13)));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_14"_el), static_cast<unsigned short>(14)));

        auto expectedValueMap = ExpectedValueMap{
            {"main"_el, "SectionWithNames()"_el},
            {"main.value_1"_el, "Integer(1)"_el},
            {"main.value_2"_el, "Integer(2)"_el},
            {"main.value_3"_el, "Integer(3)"_el},
            {"main.value_4"_el, "Integer(4)"_el},
            {"main.value_5"_el, "Integer(5)"_el},
            {"main.value_6"_el, "Integer(6)"_el},
            {"main.value_7"_el, "Integer(7)"_el},
            {"main.value_8"_el, "Integer(8)"_el},
            {"main.value_9"_el, "Integer(9)"_el},
            {"main.value_10"_el, "Integer(10)"_el},
            {"main.value_11"_el, "Integer(11)"_el},
            {"main.value_12"_el, "Integer(12)"_el},
            {"main.value_13"_el, "Integer(13)"_el},
            {"main.value_14"_el, "Integer(14)"_el},
        };
        verifyValueMap(expectedValueMap);
    }

    void testAddingFloatsUsingTemplates() {
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main"_el), location));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_1"_el), static_cast<float>(1.1)));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_2"_el), static_cast<double>(2.2)));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_3"_el), static_cast<long double>(3.3)));

        auto expectedValueMap = ExpectedValueMap{
            {"main"_el, "SectionWithNames()"_el},
            {"main.value_1"_el, "Float(1.1)"_el},
            {"main.value_2"_el, "Float(2.2)"_el},
            {"main.value_3"_el, "Float(3.3)"_el},
        };
        verifyValueMap(expectedValueMap);
    }

    void testAddingRegEx() {
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main"_el), location));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_1"_el), el::re::RegEx::compile("abc"_el)));

        auto expectedValueMap = ExpectedValueMap{
            {"main"_el, "SectionWithNames()"_el},
            {"main.value_1"_el, "RegEx(\"abc\")"_el},
        };
        verifyValueMap(expectedValueMap);
    }

    void testAddingValuesUsingTemplates() {
        REQUIRE_NOTHROW(builder.addSectionMap(NamePath::fromText("main"_el), location));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_1"_el), 12345));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_2"_el), true));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_3"_el), 123.456));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_4"_el), el::text::String{"😆"_el}));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_5"_el), makeDate(2025, 12, 26)));
        REQUIRE_NOTHROW(
            builder.addValueT(NamePath::fromText("main.value_6"_el), makeTimeWithZone(22, 11, 33, 123456000)));
        REQUIRE_NOTHROW(builder.addValueT(
            NamePath::fromText("main.value_7"_el),
            el::time::DateTime{makeDate(2025, 12, 26), makeTimeWithZone(22, 11, 33, 123456000)}));
        REQUIRE_NOTHROW(builder.addValueT(NamePath::fromText("main.value_8"_el), bytesFromHex("0102aabbcc"_el)));
        REQUIRE_NOTHROW(
            builder.addValueT(NamePath::fromText("main.value_9"_el), el::time::CalendarDelta{el::time::Hours{5}}));
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
};
