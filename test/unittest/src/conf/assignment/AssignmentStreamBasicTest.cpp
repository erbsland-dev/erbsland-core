// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AssignmentStreamHelper.hpp"

using namespace el::text::literals;

TESTED_TARGETS(AssignmentStream)
class AssignmentStreamBasicTest final : public UNITTEST_SUBCLASS(AssignmentStreamHelper) {
public:
    // Basic test to make sure the unittest works as expected.
    void testBasicFunctionality() {
        WITH_CONTEXT(setupAssignmentStream("basic.elcl"));
        WITH_CONTEXT(requireSectionMap("main"_el));
        WITH_CONTEXT(requireValue("main.server"_el, ValueType::Text));
        WITH_CONTEXT(requireValue("main.port"_el, ValueType::Integer));
        WITH_CONTEXT(requireEnd());
    }

    void testSingleValues() {
        WITH_CONTEXT(setupAssignmentStream("single_values.elcl"));
        WITH_CONTEXT(requireSectionMap("main"_el));
        WITH_CONTEXT(requireValue("main.value_1"_el, ValueType::Integer, Integer{12345}));
        WITH_CONTEXT(requireValue("main.value_2"_el, ValueType::Boolean, true));
        WITH_CONTEXT(requireValue("main.value_3"_el, ValueType::Float, 12.345));
        WITH_CONTEXT(requireValue("main.value_4"_el, ValueType::Text, el::text::String{"This is Text"_el}));
        WITH_CONTEXT(requireValue("main.value_5"_el, ValueType::Text, el::text::String{"This is Code"_el}));
        WITH_CONTEXT(requireValue("main.value_6"_el, ValueType::Date, makeDate(2026, 8, 10)));
        WITH_CONTEXT(requireValue("main.value_7"_el, ValueType::Time, makeTime(17, 54, 12, 0)));
        WITH_CONTEXT(requireValue(
            "main.value_8"_el,
            ValueType::DateTime,
            el::time::DateTime{makeDate(2026, 8, 10), makeTimeWithZone(17, 54, 12, 0)}));
        WITH_CONTEXT(requireValue("main.value_9"_el, ValueType::Bytes, bytesFromHex("010203fdfeff"_el)));
        WITH_CONTEXT(
            requireValue("main.value_10"_el, ValueType::TimeDelta, el::time::CalendarDelta{el::time::Weeks{10}}));
        WITH_CONTEXT(requireValue("main.value_11"_el, ValueType::RegEx, el::re::RegEx::compile("regex"_el)));
        REQUIRE_FALSE(assignment.value()->asRegEx()->isCompiled());
        const auto firstMatch = assignment.value()->asRegEx()->fullMatch("regex"_el);
        REQUIRE_NOT_EQUAL(firstMatch, nullptr);

        WITH_CONTEXT(requireValue("main.value_12"_el, ValueType::Integer, Integer{12345}));
        WITH_CONTEXT(requireValue("main.value_13"_el, ValueType::Text, el::text::String{"This is Text"_el}));
        WITH_CONTEXT(requireValue("main.value_14"_el, ValueType::Date, makeDate(2026, 8, 10)));
    }

    void testMultiLineValues() {
        WITH_CONTEXT(setupAssignmentStream("multiline_values.elcl"));
        WITH_CONTEXT(requireSectionMap("text"_el));
        WITH_CONTEXT(requireValue("text.value_1"_el, ValueType::Text, el::text::String{"Hello World!"_el}));
        WITH_CONTEXT(requireValue("text.value_2"_el, ValueType::Text, el::text::String{"\nHello World!\n"_el}));
        WITH_CONTEXT(requireValue("text.value_3"_el, ValueType::Text, el::text::String{"Hello World!"_el}));
        WITH_CONTEXT(requireValue("text.value_4"_el, ValueType::Text, el::text::String{"    Hello World!"_el}));
        WITH_CONTEXT(requireValue(
            "text.value_5"_el,
            ValueType::Text,
            el::text::String{"The first line.\nA second line.\nThird line of text."_el}));
        WITH_CONTEXT(requireSectionMap("code"_el));
        WITH_CONTEXT(requireValue("code.value_1"_el, ValueType::Text, el::text::String{"Code\\n"_el}));
        WITH_CONTEXT(requireValue("code.value_2"_el, ValueType::Text, el::text::String{"\nCode\\n\n"_el}));
        WITH_CONTEXT(requireValue("code.value_3"_el, ValueType::Text, el::text::String{"Code\\n"_el}));
        WITH_CONTEXT(requireValue("code.value_4"_el, ValueType::Text, el::text::String{"    Code\\n"_el}));
        WITH_CONTEXT(requireValue(
            "code.value_5"_el,
            ValueType::Text,
            el::text::String{"if len(lines) == 3:\n    print(f\"{lines}\\n\")\nexit(0)"_el}));
        WITH_CONTEXT(requireSectionMap("regex"_el));
        WITH_CONTEXT(requireValue(
            "regex.value_1"_el,
            ValueType::RegEx,
            el::re::RegEx::compile("^\\w+\\.[Ee][Ll][Cc][Ll]$"_el, el::re::Flags{el::re::Flag::Verbose})));
        WITH_CONTEXT(requireValue(
            "regex.value_2"_el,
            ValueType::RegEx,
            el::re::RegEx::compile("\n^\\w+\\.[Ee][Ll][Cc][Ll]$\n"_el, el::re::Flags{el::re::Flag::Verbose})));
        REQUIRE_FALSE(assignment.value()->asRegEx()->isCompiled());
        const auto secondMatch = assignment.value()->asRegEx()->fullMatch("config.ELCL"_el);
        REQUIRE_NOT_EQUAL(secondMatch, nullptr);
        WITH_CONTEXT(requireValue(
            "regex.value_3"_el,
            ValueType::RegEx,
            el::re::RegEx::compile("^\\w+\\.[Ee][Ll][Cc][Ll]$"_el, el::re::Flags{el::re::Flag::Verbose})));
        WITH_CONTEXT(requireValue(
            "regex.value_4"_el,
            ValueType::RegEx,
            el::re::RegEx::compile("    ^\\w+\\.[Ee][Ll][Cc][Ll]$"_el, el::re::Flags{el::re::Flag::Verbose})));
        WITH_CONTEXT(requireValue(
            "regex.value_5"_el,
            ValueType::RegEx,
            el::re::RegEx::compile("^\\w+\n    \\.[Ee][Ll][Cc][Ll]\n$"_el, el::re::Flags{el::re::Flag::Verbose})));
        WITH_CONTEXT(requireSectionMap("bytes"_el));
        WITH_CONTEXT(requireValue("bytes.value_1"_el, ValueType::Bytes, bytesFromHex("01020304e1e2e3e4"_el)));
        WITH_CONTEXT(requireValue("bytes.value_2"_el, ValueType::Bytes, bytesFromHex("01020304e1e2e3e4"_el)));
        WITH_CONTEXT(requireValue("bytes.value_3"_el, ValueType::Bytes, bytesFromHex("01020304e1e2e3e4"_el)));
        WITH_CONTEXT(requireValue("bytes.value_4"_el, ValueType::Bytes, bytesFromHex("01020304e1e2e3e4"_el)));
        WITH_CONTEXT(requireEnd());
    }

    void testSections() {
        WITH_CONTEXT(setupAssignmentStream("sections.elcl"));
        WITH_CONTEXT(requireSectionMap("main"_el));
        WITH_CONTEXT(requireSectionMap("main.server.filter"_el));
        WITH_CONTEXT(requireValue("main.server.filter.value"_el, ValueType::Text, el::text::String{"text"_el}));
        WITH_CONTEXT(requireSectionMap("main.client.filter"_el));
        WITH_CONTEXT(requireValue("main.client.filter.value"_el, ValueType::Text, el::text::String{"text"_el}));
        WITH_CONTEXT(requireSectionMap("text.\"First Text\""_el));
        WITH_CONTEXT(requireValue("text.\"First Text\".value"_el, ValueType::Integer, 1));
        WITH_CONTEXT(requireSectionMap("text.\"Second Text\""_el));
        WITH_CONTEXT(requireValue("text.\"Second Text\".value"_el, ValueType::Integer, 2));
        WITH_CONTEXT(requireEnd());
    }

    void testMeta() {
        WITH_CONTEXT(setupAssignmentStream("meta.elcl"));
        WITH_CONTEXT(requireMetaValue("@signature"_el, ValueType::Text, el::text::String{"data"_el}));
        WITH_CONTEXT(requireMetaValue("@version"_el, ValueType::Text, el::text::String{"1.0"_el}));
        WITH_CONTEXT(
            requireMetaValue("@features"_el, ValueType::Text, el::text::String{"core multi-line time-delta"_el}));
        WITH_CONTEXT(requireSectionMap("main"_el));
        WITH_CONTEXT(requireMetaValue("@include"_el, ValueType::Text, el::text::String{"path1"_el}));
        WITH_CONTEXT(requireMetaValue("@include"_el, ValueType::Text, el::text::String{"path2"_el}));
        WITH_CONTEXT(requireSectionMap("second"_el));
        WITH_CONTEXT(requireEnd());
    }
};
