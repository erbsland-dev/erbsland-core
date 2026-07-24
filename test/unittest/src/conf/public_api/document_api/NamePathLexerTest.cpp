// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/conf/NamePath.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/text/StringList.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::conf;
using namespace el::text::literals;

TESTED_TARGETS(NamePath)
class NamePathLexerTest final : public el::UnitTest {
public:
    NamePath namePath;

    auto additionalErrorMessages() -> std::string override {
        try {
            return std::format(
                "namePath:\n{}", el::text::StringConverter{internalView(namePath)->toString(2)}.toStdString());
        } catch (...) {
            return "Unexpected exception thrown";
        }
    }

    void testLexingText() {
        struct TestData {
            el::text::String text;
            NamePath expected;
        };
        const auto testData = std::vector<TestData>{
            {""_el, {}},
            {"one.two.three"_el, // basic path with no normalization required.
                NamePath{
                    {Name::createRegular("one"_el), Name::createRegular("two"_el), Name::createRegular("three"_el)}}},
            {"   Name1 . Name2 . Name 3  "_el, // basic path with spacing and required normalization.
                NamePath{
                    {Name::createRegular("name1"_el),
                        Name::createRegular("name2"_el),
                        Name::createRegular("name_3"_el)}}},
            {"server[12].info.\" This is a text\""_el, // Mixed elements.
                NamePath{
                    {Name::createRegular("server"_el),
                        Name::createIndex(12),
                        Name::createRegular("info"_el),
                        Name::createText(" This is a text"_el)}}},
            {"[12][34]"_el,             // Nested lists
                NamePath{{Name::createIndex(12), Name::createIndex(34)}}},
            {"server.value[12][34]"_el, // Nested lists
                NamePath{
                    {Name::createRegular("server"_el),
                        Name::createRegular("value"_el),
                        Name::createIndex(12),
                        Name::createIndex(34)}}},
            {"server.text.\"\"[1234].filter"_el, // Text index.
                NamePath{
                    {Name::createRegular("server"_el),
                        Name::createRegular("text"_el),
                        Name::createTextIndex(1234),
                        Name::createRegular("filter"_el)}}},
            {"\"a text\".value"_el, NamePath{{Name::createText("a text"_el), Name::createRegular("value"_el)}}},
            {"@version"_el, // basic path with no normalization required.
                NamePath{{Name::createRegular("@version"_el)}}},

        };
        for (auto const &data : testData) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    namePath = NamePath::fromText(data.text);
                    REQUIRE_EQUAL(namePath, data.expected);
                },
                [&]() -> std::string {
                    return std::format("Failed for text: \"{}\"", el::text::StringConverter{data.text}.toStdString());
                });
        }
    }

    void testLexingInvalidText() {
        const auto testData = el::text::StringList{
            "."_el,
            "name\n.name"_el,
            "name\r.name"_el,
            "name..name"_el,
            "name."_el,
            "name.value."_el,
            " name   .. name"_el,
            " name  .  "_el,
            "  name  .   value   .  "_el,
            "9name.value"_el,
            "name.9value"_el,
            "name[x]"_el,
            "name.[10]"_el,
            "name.value[x]"_el,
            "name[0]name"_el,
            "main.Name  Name"_el,
            "main.\" text \"name"_el,
            "main._value"_el,
            "main.value_"_el,
            "main.value__value"_el,
            "main.value _value"_el,
            "main.value_ value"_el,
            el::text::StringEditor::fromCharacter(el::text::Char{U'a'}, el::unit::CpLength{5000U}),
            el::text::StringEditor{"main.name"_el}
                .append(el::text::Char{U'a'}, el::unit::CpLength{100U})
                .append(".value"_el),
            "main.\"\""_el,
            "main.\"\".value"_el,
            "main.\"\"\"\n"_el,
        };
        for (const auto &text : testData) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() { REQUIRE_THROWS(NamePath::fromText(text)); },
                [&]() -> std::string {
                    return std::format("Failed for text: \"{}\"", el::text::StringConverter{text}.toStdString());
                });
        }
    }

    void testPathToText() {
        struct TestData {
            NamePath path;
            el::text::String expected;
        };
        const auto testData = std::vector<TestData>{
            {NamePath{}, ""_el},
            {NamePath{{Name::createRegular("one"_el), Name::createRegular("two"_el), Name::createRegular("three"_el)}},
                "one.two.three"_el},
            {NamePath{{Name::createTextIndex(1234), Name::createRegular("value"_el)}}, "\"\"[1234].value"_el},
            {NamePath{{Name::createRegular("value"_el), Name::createTextIndex(1234), Name::createTextIndex(0)}},
                "value.\"\"[1234].\"\"[0]"_el},
        };
        for (auto const &data : testData) {
            namePath = data.path;
            REQUIRE_EQUAL(namePath.toText(), data.expected);
        }
    }
};
