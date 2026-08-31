// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/CursorBufferTestBase.hpp"
#include "../../support/TestHelper.hpp"

#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(ParagraphOptions CursorBuffer)
class ParagraphRenderTest final : public UNITTEST_SUBCLASS(CursorBufferTestBase) {
public:
    static constexpr auto cOneLine = "one two three four five six seven eight nine ten"_el;
    static constexpr auto cTwoLines = "one two three four five six\nseven eight nine ten eleven twelve"_el;
    static constexpr auto cTextTabOneLine = "text:\tone two three four five six seven eight nine ten"_el;

    ParagraphOptions options;
    std::vector<std::string> actual;
    std::vector<std::string> expected;

    void setUp() override {
        options = {};
        actual = {};
        expected = {};
    }

    void testNoIndent() {
        options.setLineIndent(0);
        options.setFirstLineIndent(0);
        options.setWrappedLineIndent(0);

        buffer.clearScreen();
        buffer.printParagraph(cOneLine, options);
        actual = rawLinesFromBuffer();
        expected = std::vector<std::string>{
            //  01234567890123456789
            "one two three four  ",
            "five six seven eight",
            "nine ten            ",
            "                    ",
            "                    ",
        };
        REQUIRE_EQUAL_LINES(actual, expected);

        buffer.clearScreen();
        buffer.printParagraph(cTwoLines, options);
        actual = rawLinesFromBuffer();
        expected = std::vector<std::string>{
            //  01234567890123456789
            "one two three four  ",
            "five six            ",
            "seven eight nine ten",
            "eleven twelve       ",
            "                    ",
        };
        REQUIRE_EQUAL_LINES(actual, expected);
    }

    void testLineIndent() {
        options.setLineIndent(4);
        options.setFirstLineIndent(ParagraphOptions::cUseLineIndent);
        options.setWrappedLineIndent(ParagraphOptions::cUseLineIndent);

        buffer.clearScreen();
        buffer.printParagraph(cOneLine, options);
        actual = rawLinesFromBuffer();
        expected = std::vector<std::string>{
            //  01234567890123456789
            "    one two three   ",
            "    four five six   ",
            "    seven eight nine",
            "    ten             ",
            "                    ",
        };
        REQUIRE_EQUAL_LINES(actual, expected);

        buffer.clearScreen();
        buffer.printParagraph(cTwoLines, options);
        actual = rawLinesFromBuffer();
        expected = std::vector<std::string>{
            //  01234567890123456789
            "    one two three   ",
            "    four five six   ",
            "    seven eight nine",
            "    ten eleven      ",
            "    twelve          ",
        };
        REQUIRE_EQUAL_LINES(actual, expected);
    }

    void testFirstLineIndent() {
        options.setLineIndent(4);
        options.setFirstLineIndent(0);
        options.setWrappedLineIndent(ParagraphOptions::cUseLineIndent);

        buffer.clearScreen();
        buffer.printParagraph(cOneLine, options);
        actual = rawLinesFromBuffer();
        expected = std::vector<std::string>{
            //  01234567890123456789
            "one two three four  ",
            "    five six seven  ",
            "    eight nine ten  ",
            "                    ",
            "                    ",
        };
        REQUIRE_EQUAL_LINES(actual, expected);

        buffer.clearScreen();
        buffer.printParagraph(cTwoLines, options);
        actual = rawLinesFromBuffer();
        expected = std::vector<std::string>{
            //  01234567890123456789
            "one two three four  ",
            "    five six        ",
            "seven eight nine ten",
            "    eleven twelve   ",
            "                    ",
        };
        REQUIRE_EQUAL_LINES(actual, expected);

        options.setFirstLineIndent(8);
        buffer.clearScreen();
        buffer.printParagraph(cOneLine, options);
        actual = rawLinesFromBuffer();
        expected = std::vector<std::string>{
            //  01234567890123456789
            "        one two     ",
            "    three four five ",
            "    six seven eight ",
            "    nine ten        ",
            "                    ",
        };
        REQUIRE_EQUAL_LINES(actual, expected);
    }
    void testFirstLineIndentWithTab() {
        options.setLineIndent(8);
        options.setFirstLineIndent(0);
        options.setWrappedLineIndent(ParagraphOptions::cUseLineIndent);

        buffer.clearScreen();
        buffer.printParagraph(cTextTabOneLine, options);
        actual = rawLinesFromBuffer();
        expected = std::vector<std::string>{
            //  01234567890123456789
            "text:   one two     ",
            "        three four  ",
            "        five six    ",
            "        seven eight ",
            "        nine ten    ",
        };
        REQUIRE_EQUAL_LINES(actual, expected);

        options.setLineIndent(4);
        options.setFirstLineIndent(0);
        options.setWrappedLineIndent(ParagraphOptions::cUseLineIndent);

        buffer.clearScreen();
        buffer.printParagraph(cTextTabOneLine, options);
        actual = rawLinesFromBuffer();
        expected = std::vector<std::string>{
            //  01234567890123456789
            "text: one two three ",
            "    four five six   ",
            "    seven eight nine",
            "    ten             ",
            "                    ",
        };
        REQUIRE_EQUAL_LINES(actual, expected);

        options.setLineIndent(4);
        options.setFirstLineIndent(2);
        options.setWrappedLineIndent(ParagraphOptions::cUseLineIndent);

        buffer.clearScreen();
        buffer.printParagraph(cTextTabOneLine, options);
        actual = rawLinesFromBuffer();
        expected = std::vector<std::string>{
            //  01234567890123456789
            "  text: one two     ",
            "    three four five ",
            "    six seven eight ",
            "    nine ten        ",
            "                    ",
        };
        REQUIRE_EQUAL_LINES(actual, expected);
    }

    void testOversizedWordUsesRemainingSpaceBeforeWrapping() {
        options.setWordBreakMark(Block{});
        buffer.clearScreen();
        buffer.printParagraph("Path: /var/folders/example/temporary"_el, options);
        actual = rawLinesFromBuffer();
        expected = std::vector<std::string>{
            //  01234567890123456789
            "Path: /var/folders/e",
            "xample/temporary    ",
            "                    ",
            "                    ",
            "                    ",
        };
        REQUIRE_EQUAL_LINES(actual, expected);
    }

    void testOversizedWordStartsOnNextLineIfCurrentLineIsFull() {
        options.setWordBreakMark(Block{});
        buffer.clearScreen();
        buffer.printParagraph("12345678901234567890 /var/folders/example/temporary"_el, options);
        actual = rawLinesFromBuffer();
        expected = std::vector<std::string>{
            //  01234567890123456789
            "12345678901234567890",
            "/var/folders/example",
            "/temporary          ",
            "                    ",
            "                    ",
        };
        REQUIRE_EQUAL_LINES(actual, expected);
    }
};
