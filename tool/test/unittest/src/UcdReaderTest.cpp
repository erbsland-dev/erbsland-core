// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ParseNumberError.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <tool/unicode/UcdReader.hpp>

#include <utility>

using namespace el::text::literals;

TESTED_TARGETS(UcdReader)
class UcdReaderTest final : public el::UnitTest {
public:
    el::List<tool::unicode::UcdRecord> _records;

    void requireParseSuccess(el::String text, const el::ItemCount fieldCount, bool exactFields = false) {
        _records = tool::unicode::UcdReader{std::move(text), "unittest.txt"_el}.parse(fieldCount, exactFields);
    }

    void testRangeAndEmptyFields() {
        requireParseSuccess("# comment\n0041..0043 ; Letter ; # trailing\n"_el, el::ItemCount{3}, true);
        REQUIRE_EQUAL(_records.count().toSizeT(), 1U);
        REQUIRE_EQUAL(_records.first().first, U'A');
        REQUIRE_EQUAL(_records.first().last, U'C');
        REQUIRE(_records.first().fields.last().isEmpty());
    }

    void testSingleCodePointsAndWhitespace() {
        requireParseSuccess(
            " \r\n  # heading\r\n\t0041 ; Letter ; ; value  # comment\r\n10FFFF;Limit;;end"_el, el::ItemCount{4}, true);
        REQUIRE_EQUAL(_records.count(), el::ItemCount{2});
        REQUIRE_EQUAL(_records.first().first, U'A');
        REQUIRE_EQUAL(_records.first().last, U'A');
        REQUIRE_EQUAL(_records.first().fields, (el::StringList{"0041"_el, "Letter"_el, ""_el, "value"_el}));
        REQUIRE_EQUAL(_records.last().first, char32_t{0x10FFFF});
    }

    void testRangeBoundariesAndRepeatedParsing() {
        auto reader = tool::unicode::UcdReader{
            "0000..0000; Zero\nD800..DFFF;Surrogates\n10FFFE..10FFFF;Limit;\n"_el, "ranges.txt"_el};
        _records = reader.parse(el::ItemCount{2});
        REQUIRE_EQUAL(_records.count(), el::ItemCount{3});
        REQUIRE_EQUAL(_records.first().first, char32_t{0});
        REQUIRE_EQUAL(_records.first().last, char32_t{0});
        REQUIRE_EQUAL(_records.get(el::ItemIndex{1}).first, char32_t{0xD800});
        REQUIRE_EQUAL(_records.get(el::ItemIndex{1}).last, char32_t{0xDFFF});
        REQUIRE_EQUAL(_records.last().last, char32_t{0x10FFFF});
        REQUIRE(_records.last().fields.last().isEmpty());
        const auto repeated = reader.parse(el::ItemCount{2});
        REQUIRE_EQUAL(repeated.count(), _records.count());
        REQUIRE_EQUAL(repeated.last().fields, _records.last().fields);
    }

    void testCommentsAndFieldCounts() {
        requireParseSuccess("\n # comment\n\t\r\n"_el, el::ItemCount{2U}, true);
        REQUIRE(_records.isEmpty());
        requireParseSuccess("0041;Letter;extra;\n"_el, el::ItemCount{2});
        REQUIRE_EQUAL(_records.first().fields.count(), el::ItemCount{4});
        REQUIRE_THROWS((tool::unicode::UcdReader{"0041;Letter;extra"_el, "test.txt"_el}.parse(el::ItemCount{2}, true)));
        requireParseSuccess("0041"_el, el::ItemCount{1U}, true);
        REQUIRE_EQUAL(_records.first().fields.count(), el::ItemCount{1});
    }

    void testCodePointSyntax() {
        const auto reader = tool::unicode::UcdReader{""_el, "code.txt"_el};
        REQUIRE_EQUAL(reader.codePoint("a"_el), char32_t{0xA});
        REQUIRE_EQUAL(reader.codePoint("0B01"_el), char32_t{0xB01});
        REQUIRE_EQUAL(reader.codePoint("0b01"_el), char32_t{0xB01});
        REQUIRE_EQUAL(reader.codePoint("00aBcD"_el), char32_t{0xABCD});
        REQUIRE_EQUAL(reader.codePoint("10FFFF"_el), char32_t{0x10FFFF});
        for (
            const auto &text :
            el::StringList{""_el, "0x41"_el, "-41"_el, "+41"_el, "0000041"_el, "GG"_el, "41Z"_el, " 41"_el, "41 "_el}) {
            REQUIRE_THROWS_AS(el::ApplicationError, reader.codePoint(text));
        }
        REQUIRE_THROWS_AS(el::ApplicationError, reader.codePoint("110000"_el));
    }

    void testErrorContextAndCause() {
        try {
            [[maybe_unused]] const auto records =
                tool::unicode::UcdReader{"# header\n\n0041..ZZ;Letter"_el, "source.txt"_el}.parse(el::ItemCount{2});
            REQUIRE(false);
        } catch (const el::ApplicationError &error) {
            REQUIRE_EQUAL(error.sourcePath(), "source.txt"_el);
            REQUIRE_EQUAL(error.codeLocation().line(), el::LineIndex{2});
            REQUIRE_EQUAL(error.description(), "Failed to parse a UCD code point."_el);
            REQUIRE(error.cause());
            REQUIRE_THROWS_AS(el::ParseNumberError, std::rethrow_exception(error.cause()));
        }
        try {
            [[maybe_unused]] const auto records =
                tool::unicode::UcdReader{"# header\n0041;Letter;extra"_el, "fields.txt"_el}.parse(
                    el::ItemCount{2}, true);
            REQUIRE(false);
        } catch (const el::ApplicationError &error) {
            REQUIRE_EQUAL(error.codeLocation().line(), el::LineIndex{1});
            REQUIRE(error.description().contains("expected 2 fields, got 3"_el));
            REQUIRE_FALSE(error.cause());
        }
    }

    void requireParseFail(const el::String &text, const el::ItemCount fieldCount) {
        REQUIRE_THROWS(tool::unicode::UcdReader{text, "unittest.txt"_el}.parse(fieldCount));
    }

    void testMalformedRanges() {
        WITH_CONTEXT(requireParseFail("0043..0041;Letter"_el, el::ItemCount{2}));
        WITH_CONTEXT(requireParseFail("110000;Letter"_el, el::ItemCount{2}));
        WITH_CONTEXT(requireParseFail("0041...0042;Letter"_el, el::ItemCount{2}));
        WITH_CONTEXT(requireParseFail("0041;"_el, el::ItemCount{3}));
        WITH_CONTEXT(requireParseFail(";Letter"_el, el::ItemCount{2}));
        WITH_CONTEXT(requireParseFail("..0042;Letter"_el, el::ItemCount{2}));
        WITH_CONTEXT(requireParseFail("0041..;Letter"_el, el::ItemCount{2}));
        WITH_CONTEXT(requireParseFail("0041..0042..0043;Letter"_el, el::ItemCount{2}));
        WITH_CONTEXT(requireParseFail("0041..110000;Letter"_el, el::ItemCount{2}));
    }
};
