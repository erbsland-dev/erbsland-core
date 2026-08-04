// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/conf/ConfErrorCategory.hpp>
#include <erbsland/conf/impl/constants/Limits.hpp>
#include <erbsland/conf/impl/lexer/NameLexer.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>

using namespace erbsland::conf;
using namespace erbsland::text::literals;
using impl::NameLexer;

TESTED_TARGETS(NameLexer)
class NameLexerTest final : public el::UnitTest {
public:
    Name name;

    struct TestData {
        el::text::String text;           ///< The name path to test.
        std::vector<Name> expectedNames; ///< The list of expected names.
    };
    using TestDataList = std::vector<TestData>;

    struct ErrorData {
        el::text::String text;                                               ///< A text with an error in it.
        ConfErrorCategory expectedErrorCategory = ConfErrorCategory::Syntax; ///< The expected error class.
    };
    using ErrorDataList = std::vector<ErrorData>;

    void verifyTestData(const TestDataList &testDataList) {
        for (const auto &testData : testDataList) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    NameLexer lexer{testData.text};
                    REQUIRE_NOTHROW(lexer.initialize());
                    for (auto i = 0; i < testData.expectedNames.size(); ++i) {
                        runWithContext(
                            SOURCE_LOCATION(),
                            [&]() {
                                const auto expectedName = testData.expectedNames[i];
                                REQUIRE(lexer.hasNext());
                                REQUIRE_NOTHROW(name = lexer.next());
                                REQUIRE_EQUAL(name, expectedName);
                            },
                            [&]() -> std::string { return std::format("Failed at index: {}", i); });
                    }
                    REQUIRE_FALSE(lexer.hasNext());
                },
                [&]() -> std::string {
                    return std::format("Failed for text: `{}`", el::text::StringConverter{testData.text}.toStdString());
                });
        }
    }

    void verifyErrorData(const ErrorDataList &errorDataList) {
        for (const auto &errorData : errorDataList) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    NameLexer lexer{errorData.text};
                    REQUIRE_NOTHROW(lexer.initialize());
                    try {
                        NameList names;
                        while (lexer.hasNext()) {
                            name = lexer.next();
                            names.push_back(name); // do something useful looking.
                        }
                        REQUIRE(false);            // expected error, but none was thrown.
                    } catch (ConfError const &error) {
                        REQUIRE_EQUAL(error.category(), errorData.expectedErrorCategory);
                    }
                },
                [&]() -> std::string {
                    return std::format(
                        "Failed for text: `{}`", el::text::StringConverter{errorData.text}.toStdString());
                });
        }
    }

    void testEmpty() {
        NameLexer lexer{""_el};
        REQUIRE_NOTHROW(lexer.initialize());
        REQUIRE_FALSE(lexer.hasNext());
        REQUIRE_NOTHROW(name = lexer.next());
    }

    void testTooLong() {
        NameLexer lexer{el::text::StringEditor::fromCharacter(
            el::text::Char{U'a'}, el::unit::CpLength{el::conf::impl::limits::maxLineLength + 100})};
        REQUIRE_THROWS_AS(ConfError, lexer.initialize());
    }

    void testRegularNames() {
        const auto testData = TestDataList{
            {"a"_el, {Name::createRegular("a"_el)}},
            {"A_longer32_regular09_NAME"_el, {Name::createRegular("a_longer32_regular09_name"_el)}},
            {"     name"_el, {Name::createRegular("name"_el)}},
            {"name   \t     "_el, {Name::createRegular("name"_el)}},
            {"   \t      name   \t     "_el, {Name::createRegular("name"_el)}},
            {"A regular Name 345 with Spaces"_el, {Name::createRegular("a_regular_name_345_with_spaces"_el)}},
            {"      Name with Spaces        "_el, {Name::createRegular("name_with_spaces"_el)}},
        };
        WITH_CONTEXT(verifyTestData(testData));
    }

    void testIndexes() {
        const auto testData = TestDataList{
            {"[0]"_el, {Name::createIndex(0)}},
            {"[1]"_el, {Name::createIndex(1)}},
            {"[27302]"_el, {Name::createIndex(27302)}},
            {"[27'302]"_el, {Name::createIndex(27302)}},
            {"[   129]"_el, {Name::createIndex(129)}},
            {"[762   ]"_el, {Name::createIndex(762)}},
            {"    [1]"_el, {Name::createIndex(1)}},
            {"[1]    "_el, {Name::createIndex(1)}},
        };
        WITH_CONTEXT(verifyTestData(testData));
    }

    void testTextNames() {
        const auto testData = TestDataList{
            {R"("a")"_el, {Name::createText("a"_el)}},
            {R"(    "a")"_el, {Name::createText("a"_el)}},
            {R"("a"    )"_el, {Name::createText("a"_el)}},
            {R"("   abc   ")"_el, {Name::createText("   abc   "_el)}},
            {R"("\r\t\n\u1234\u{1f20}")"_el, {Name::createText("\r\t\n\u1234\u1f20"_el)}},
        };
        WITH_CONTEXT(verifyTestData(testData));
    }

    void testTextIndexes() {
        const auto testData = TestDataList{
            {R"(""[0])"_el, {Name::createTextIndex(0)}},
            {R"(""[1])"_el, {Name::createTextIndex(1)}},
            {R"(""[93821])"_el, {Name::createTextIndex(93821)}},
            {R"(""[  93'821  ])"_el, {Name::createTextIndex(93821)}},
        };
        WITH_CONTEXT(verifyTestData(testData));
    }

    void testSingleNameErrors() {
        const auto testData = ErrorDataList{
            {"?"_el, ConfErrorCategory::Character},
            {"_name"_el, ConfErrorCategory::Syntax},
            {"name_"_el, ConfErrorCategory::Syntax},
            {"name__name"_el, ConfErrorCategory::Syntax},
            {"name  name"_el, ConfErrorCategory::Syntax},
            {".."_el, ConfErrorCategory::Syntax},
            {"[]"_el, ConfErrorCategory::Syntax},
            {"[-1]"_el, ConfErrorCategory::Syntax},
            {"[a]"_el, ConfErrorCategory::Syntax},
            {"\"\""_el, ConfErrorCategory::Syntax},
            {"\"\\u{0}\""_el, ConfErrorCategory::Syntax},
            {"\"\"[]"_el, ConfErrorCategory::Syntax},
            {"\"\"[-1]"_el, ConfErrorCategory::Syntax},
        };
    }

    void testPaths() {
        const auto testData = TestDataList{
            {"a.b.c.d"_el,
                {
                    Name::createRegular("a"_el),
                    Name::createRegular("b"_el),
                    Name::createRegular("c"_el),
                    Name::createRegular("d"_el),
                }},
            {"  a regular name  . Second One. Another  .Last Name  "_el,
                {
                    Name::createRegular("a_regular_name"_el),
                    Name::createRegular("second_one"_el),
                    Name::createRegular("another"_el),
                    Name::createRegular("last_name"_el),
                }},
            {"\"text\".name[123].\"text\""_el,
                {
                    Name::createText("text"_el),
                    Name::createRegular("name"_el),
                    Name::createIndex(123),
                    Name::createText("text"_el),
                }},
            {"[789].\"\"[123].name"_el,
                {Name::createIndex(789), Name::createTextIndex(123), Name::createRegular("name"_el)}},
        };
        WITH_CONTEXT(verifyTestData(testData));
    }

    void testPathErrors() {
        const auto testData = ErrorDataList{
            // Paths must not end with a separator.
            {"a.b.c."_el, ConfErrorCategory::UnexpectedEnd},
            {"a.b.c.  "_el, ConfErrorCategory::UnexpectedEnd},
            {"a.b.c   ."_el, ConfErrorCategory::UnexpectedEnd},
            // Paths must not start with a separator.
            {".a.b.c"_el, ConfErrorCategory::Syntax},
            {"    .a.b.c"_el, ConfErrorCategory::Syntax},
            {".   a.b.c"_el, ConfErrorCategory::Syntax},
            // Subsequent separators aren't allowed
            {"a.b..c.d"_el, ConfErrorCategory::Syntax},
            {"a.b.    .c.d"_el, ConfErrorCategory::Syntax},
            // Names must not start with underlines.
            {"a._b.c.d"_el, ConfErrorCategory::Syntax},
            {"a.   _b.c.d"_el, ConfErrorCategory::Syntax},
            // Indexes must not immediately follow a seperator.
            {"a.[1].c.d"_el, ConfErrorCategory::Syntax},
            {"a.   [1].c.d"_el, ConfErrorCategory::Syntax},
            // Empty index is not allowed.
            {"a.[].c.d"_el, ConfErrorCategory::Syntax},
            // Names must be separated properly.
            {"a.\"text\"\"text\".c.d"_el, ConfErrorCategory::Syntax},
            {"a.\"text\"name.c.d"_el, ConfErrorCategory::Syntax},
            {"a.[1]name.c.d"_el, ConfErrorCategory::Syntax},
            {"a.[1][2].c.d"_el, ConfErrorCategory::Syntax},
            {"a.[1] [2].c.d"_el, ConfErrorCategory::Syntax},
            {"a.\"\".c.d"_el, ConfErrorCategory::Syntax},
            {"a.\"\"[1][2].c.d"_el, ConfErrorCategory::Syntax},
            {"a.\"\"[1]\"\"[2].c.d"_el, ConfErrorCategory::Syntax},
            {"a.\"\"[1]name.c.d"_el, ConfErrorCategory::Syntax},
            // Only tab and space is considered as spacing.
            {"a.b\n.c.d"_el, ConfErrorCategory::Syntax},
            {"a.b\r.c.d"_el, ConfErrorCategory::Syntax},
        };
        WITH_CONTEXT(verifyErrorData(testData));
    }
};
