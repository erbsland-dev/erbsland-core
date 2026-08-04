// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValueTestHelper.hpp"

#include <erbsland/conf/StdFormat.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Document Value)
class ValueToTextTest final : public UNITTEST_SUBCLASS(ValueTestHelper) {
public:
    TESTED_TARGETS(toTextRepresentation)
    void testToTextRepresentation() {
        struct TestData {
            std::string valueText;
            el::text::String expectedText;
        };
        const auto testData = std::vector<TestData>{
            {"0xff", "255"_el},     // integer
            {"enabled", "true"_el}, // boolean
            {"off", "false"_el},    // boolean
            // float requires separate test, as the output format isn't stable.
            {"\"\\u{41}BC\"", "ABC"_el},
            {"2025-02-22", "2025-02-22"_el},                           // date
            {"14:02:01.100Z", "14:02:01.1Z"_el},                       // time
            {"2025-02-22 14:02:01.100Z", "2025-02-22 14:02:01.1Z"_el}, // date-time
            {"<  01 0203 >", "010203"_el},                             // bytes
            {"20 seconds", "20s"_el},                                  // time delta
            {"/regex/", "regex"_el},                                   // regular expression
        };
        for (const auto &data : testData) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    setupTemplate2(data.valueText);
                    const auto actualText = value->toTextRepresentation();
                    REQUIRE_EQUAL(actualText, data.expectedText);
                },
                [&]() -> std::string {
                    return std::format(
                        "Failed for value: \"{}\" expected: \"{}\"",
                        data.valueText,
                        el::text::StringConverter{data.expectedText}.toStdString());
                });
        }
        setupTemplate2("123.456");
        const auto text = value->toTextRepresentation();
        const auto doubleValue = std::stod(el::text::StringConverter{text}.toStdString());
        REQUIRE_LESS(std::abs(doubleValue - 123.456), std::numeric_limits<double>::epsilon());
    }

    void testToTestText() {
        struct TestData {
            std::string valueText;
            el::text::String expectedText;
        };
        const auto testData = std::vector<TestData>{
            {"0xff", "Integer(255)"_el},     // integer
            {"enabled", "Boolean(true)"_el}, // boolean
            {"off", "Boolean(false)"_el},    // boolean
            // float requires separate test, as the output format isn't stable.
            {"\"\\u{41}BC\"", "Text(\"ABC\")"_el},
            {R"("\t\n\r\\\". :=\u{e9}")", R"(Text("\u{9}\u{a}\u{d}\u{5c}\u{22}\u{2e} \u{3a}\u{3d}\u{e9}"))"_el},
            {"2025-02-22", "Date(2025-02-22)"_el},                               // date
            {"14:02:01.100Z", "Time(14:02:01.1z)"_el},                           // time
            {"2025-02-22 14:02:01.100Z", "DateTime(2025-02-22 14:02:01.1z)"_el}, // date-time
            {"<  01 0203 >", "Bytes(010203)"_el},                                // bytes
            {"20 seconds", "TimeDelta(20s)"_el},                                 // time delta
            {"/regex/", "RegEx(\"regex\")"_el},                                  // regular expression
        };
        for (const auto &data : testData) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    setupTemplate2(data.valueText);
                    const auto actualText = value->toTestText();
                    REQUIRE_EQUAL(actualText, data.expectedText);
                },
                [&]() -> std::string {
                    return std::format(
                        "Failed for value: \"{}\" expected: \"{}\"",
                        data.valueText,
                        el::text::StringConverter{data.expectedText}.toStdString());
                });
        }
        setupTemplate2("123.456");
        const auto text = value->toTestText();
        REQUIRE(text.startsWith("Float("_el));
        REQUIRE(text.endsWith(")"_el));
        const auto stdText = el::text::StringConverter{text}.toStdString();
        const auto doubleValue = std::stod(stdText.substr(6, stdText.size() - 7));
        REQUIRE_LESS(std::abs(doubleValue - 123.456), std::numeric_limits<double>::epsilon());
    }
};
