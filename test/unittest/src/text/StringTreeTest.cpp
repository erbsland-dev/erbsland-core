// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/IntegerFormatFlag.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringTree.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <vector>

using namespace el::text::literals;

using el::text::IntegerFormat;
using el::text::IntegerFormatFlag;
using el::text::StringConverter;
using el::text::StringTree;
using el::unit::CpLength;

TESTED_TARGETS(StringTree)
class StringTreeTest final : public el::UnitTest {
public:
    void testBasicRendering() {

        auto tree = StringTree{"Root"_el};
        tree.append("plain line"_el);
        tree.append("text"_el, "value"_el);
        tree.append("flag"_el, true);
        tree.append("count"_el, 42);

        REQUIRE_EQUAL(
            StringConverter{tree.toString()}.toStdString(),
            std::string{"Root:\n"
                        "    plain line\n"
                        "    text: value\n"
                        "    flag: true\n"
                        "    count: 42"});
    }

    void testSubtreesAndEmptySubtree() {

        auto child = StringTree{"Child"_el};
        child.append("answer"_el, 42);

        auto tree = StringTree{"Root"_el};
        tree.append("child"_el, child);
        tree.append("empty"_el, StringTree{});

        REQUIRE_EQUAL(
            StringConverter{tree.toString(CpLength{2U})}.toStdString(),
            std::string{"Root:\n"
                        "  child:\n"
                        "    Child:\n"
                        "      answer: 42\n"
                        "  empty: (empty)"});
    }

    void testIntegerFormattingAndLists() {

        auto format = IntegerFormat::hexadecimal();
        format.setFlags(IntegerFormatFlag::BasePrefix);
        const auto values = std::vector<int>{10, 11};

        auto tree = StringTree{"Root"_el};
        tree.append("hex"_el, 255, format);
        tree.appendList("values"_el, values, [](const int value) {
            auto result = StringTree{"value"_el};
            result.append("number"_el, value);
            return result;
        });

        REQUIRE_EQUAL(
            StringConverter{tree.toString()}.toStdString(),
            std::string{"Root:\n"
                        "    hex: 0xff\n"
                        "    values:\n"
                        "        [0]:\n"
                        "            value:\n"
                        "                number: 10\n"
                        "        [1]:\n"
                        "            value:\n"
                        "                number: 11"});
    }
};
