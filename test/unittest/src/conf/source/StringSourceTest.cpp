// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/impl/constants/Limits.hpp>
#include <erbsland/conf/impl/source/StringSource.hpp>
#include <erbsland/conf/Source.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/text/String.hpp>

#include <string>

using namespace el::conf;
using namespace el::text::literals;

TESTED_TARGETS(Source)
class StringSourceTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    SourcePtr source;

    void testConstructionAndState() {
        source = Source::fromString(el::text::String{"test"});
        REQUIRE(source != nullptr);
        REQUIRE(source->name() == "text"_el);
        REQUIRE(source->path().isEmpty());
        REQUIRE_FALSE(source->isOpen());
        REQUIRE_FALSE(source->atEnd());

        REQUIRE_NOTHROW(source->open());
        REQUIRE(source->isOpen());
        source->close();
        REQUIRE_FALSE(source->isOpen());
        REQUIRE_FALSE(source->atEnd());
    }

    void testLineEndingsAndFinalLine() {
        source = Source::fromString(el::text::String{"one\ntwo\r\n\nfour"});
        REQUIRE_NOTHROW(source->open());
        REQUIRE_EQUAL(source->readLine(), el::text::String{"one\n"});
        REQUIRE_EQUAL(source->readLine(), el::text::String{"two\r\n"});
        REQUIRE_EQUAL(source->readLine(), el::text::String{"\n"});
        REQUIRE_EQUAL(source->readLine(), el::text::String{"four"});
        REQUIRE(source->atEnd());
        REQUIRE_FALSE(source->isOpen());
        REQUIRE(source->readLine().isEmpty());
    }

    void testEmptyInput() {
        source = Source::fromString(el::text::String{});
        REQUIRE_NOTHROW(source->open());
        REQUIRE(source->readLine().isEmpty());
        REQUIRE(source->atEnd());
        REQUIRE_FALSE(source->isOpen());
    }

    void testClosedSourceErrors() {
        source = Source::fromString(el::text::String{"line\nnext\n"});
        REQUIRE_THROWS_AS(ConfError, source->readLine());
        REQUIRE_NOTHROW(source->open());
        REQUIRE_EQUAL(source->readLine(), el::text::String{"line\n"});
        source->close();
        REQUIRE_THROWS_AS(ConfError, source->readLine());
    }

    void testByteLengthBoundaries() {
        auto valid = std::string(limits::maxLineLength, 'a');
        source = Source::fromString(el::text::String{valid});
        REQUIRE_NOTHROW(source->open());
        REQUIRE_EQUAL(source->readLine().length().toSizeT(), limits::maxLineLength);

        auto validWithEnding = std::string(limits::maxLineLength - 1, 'b');
        validWithEnding.push_back('\n');
        source = Source::fromString(el::text::String{validWithEnding});
        REQUIRE_NOTHROW(source->open());
        REQUIRE_EQUAL(source->readLine().length().toSizeT(), limits::maxLineLength);

        auto invalid = std::string(limits::maxLineLength, 'c');
        invalid.push_back('\n');
        source = Source::fromString(el::text::String{invalid});
        REQUIRE_NOTHROW(source->open());
        REQUIRE_THROWS_AS(ConfError, source->readLine());
        REQUIRE_FALSE(source->isOpen());
    }

    void testMultibyteByteLengthBoundaries() {
        auto valid = std::u8string{};
        for (std::size_t i = 0; i < 1333; ++i) {
            valid.append(u8"€");
        }
        valid.push_back(u8'\n');
        REQUIRE_EQUAL(valid.size(), limits::maxLineLength);
        source = Source::fromString(el::text::String{std::u8string_view{valid}});
        REQUIRE_NOTHROW(source->open());
        REQUIRE_EQUAL(source->readLine().length().toSizeT(), limits::maxLineLength);

        auto invalid = valid;
        invalid.insert(invalid.end() - 1, u8'!');
        source = Source::fromString(el::text::String{std::u8string_view{invalid}});
        REQUIRE_NOTHROW(source->open());
        REQUIRE_THROWS_AS(ConfError, source->readLine());
    }

    void testMalformedUtf8IsPreserved() {
        auto malformed = std::string{"a"};
        malformed.push_back(static_cast<char>(0x80));
        malformed.push_back('\n');
        source = Source::fromString(el::text::String{malformed});
        REQUIRE_NOTHROW(source->open());
        const auto line = source->readLine();
        REQUIRE_EQUAL(line.length().toSizeT(), malformed.size());
        REQUIRE_FALSE(line.isValidUtf8());
    }

    void testReturnedLinesKeepTheirStorageAlive() {
        source = Source::fromString(el::text::String{"first\nsecond\n"});
        REQUIRE_NOTHROW(source->open());
        const auto first = source->readLine();
        const auto second = source->readLine();
        source.reset();
        REQUIRE_EQUAL(first, el::text::String{"first\n"});
        REQUIRE_EQUAL(second, el::text::String{"second\n"});
    }

    void testCodeSnippetAtDocumentBoundariesAndWithCrlf() {
        source = Source::fromString(el::text::String{"zero\r\none\ntwo\r\nthree\nfour"});

        const auto first = source->codeSnippet(el::unit::CodeLocation{el::unit::LineIndex::zero()});
        REQUIRE(first.has_value());
        REQUIRE_EQUAL(first->startLine, el::unit::LineIndex::zero());
        REQUIRE_EQUAL(first->language, "elcl"_el);
        REQUIRE_EQUAL(first->lines.count(), el::unit::ElementCount{3U});
        REQUIRE_EQUAL(first->lines.get(el::unit::ElementIndex::zero()), "zero"_el);
        REQUIRE_EQUAL(first->lines.get(el::unit::ElementIndex{2U}), "two"_el);

        const auto last = source->codeSnippet(el::unit::CodeLocation{el::unit::LineIndex{4U}});
        REQUIRE(last.has_value());
        REQUIRE_EQUAL(last->startLine, el::unit::LineIndex{2U});
        REQUIRE_EQUAL(last->lines.count(), el::unit::ElementCount{3U});
        REQUIRE_EQUAL(last->lines.get(el::unit::ElementIndex{2U}), "four"_el);
    }

    void testCodeSnippetMiddleAndInvalidLocations() {
        source = Source::fromString(el::text::String{"zero\none\ntwo\nthree\nfour\nfive"});
        const auto middle = source->codeSnippet(el::unit::CodeLocation{el::unit::LineIndex{3U}});
        REQUIRE(middle.has_value());
        REQUIRE_EQUAL(middle->startLine, el::unit::LineIndex{1U});
        REQUIRE_EQUAL(middle->lines.count(), el::unit::ElementCount{5U});
        REQUIRE_EQUAL(middle->lines.get(el::unit::ElementIndex::zero()), "one"_el);
        REQUIRE_EQUAL(middle->lines.get(el::unit::ElementIndex{4U}), "five"_el);

        REQUIRE_FALSE(source->codeSnippet(el::unit::CodeLocation{}).has_value());
        REQUIRE_FALSE(source->codeSnippet(el::unit::CodeLocation{el::unit::LineIndex{99U}}).has_value());
        source = Source::fromString(el::text::String{});
        REQUIRE_FALSE(source->codeSnippet(el::unit::CodeLocation{el::unit::LineIndex::zero()}).has_value());
    }
};
