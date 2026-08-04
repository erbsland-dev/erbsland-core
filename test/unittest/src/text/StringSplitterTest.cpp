// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringSplitter.hpp>
#include <erbsland/text/u16/U16StringSplitter.hpp>
#include <erbsland/text/u32/U32StringSplitter.hpp>
#include <erbsland/text/u8/U8StringSplitter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <type_traits>

using namespace el::text;
using namespace el::text::literals;

static_assert(std::is_same_v<StringSplitter, U8StringSplitter>);

TESTED_TARGETS(StringSplitMode StringSplitter U8StringSplitter U16StringSplitter U32StringSplitter)
class StringSplitterTest final : public el::UnitTest {
public:
    void testDiscardSeparator() {
        auto splitter = StringSplitter{",alpha,,beta,"_el, Char{U','}};

        REQUIRE_FALSE(splitter.isAtEnd());
        REQUIRE_EQUAL(splitter.remaining(), ",alpha,,beta,"_el);
        REQUIRE_EQUAL(splitter.next(), ""_el);
        REQUIRE_EQUAL(splitter.next(), "alpha"_el);
        REQUIRE_EQUAL(splitter.next(), ""_el);
        REQUIRE_EQUAL(splitter.next(), "beta"_el);
        REQUIRE_FALSE(splitter.isAtEnd());
        REQUIRE_EQUAL(splitter.next(), ""_el);
        REQUIRE(splitter.isAtEnd());
        REQUIRE(splitter.next().isEmpty());
        REQUIRE(splitter.remaining().isEmpty());
    }

    void testKeepSeparator() {
        auto splitter = StringSplitter{"one\ntwo\n\n"_el, Char{U'\n'}, StringSplitMode::KeepSeparator};

        REQUIRE_EQUAL(splitter.next(), "one\n"_el);
        REQUIRE_EQUAL(splitter.next(), "two\n"_el);
        REQUIRE_EQUAL(splitter.next(), "\n"_el);
        REQUIRE(splitter.isAtEnd());
        REQUIRE(splitter.next().isEmpty());
    }

    void testCharacterSetAndUnicode() {
        auto splitter = StringSplitter{"one,two;three☃four"_el, CharSet{",;☃"_el}};

        REQUIRE_EQUAL(splitter.next(), "one"_el);
        REQUIRE_EQUAL(splitter.next(), "two"_el);
        REQUIRE_EQUAL(splitter.next(), "three"_el);
        REQUIRE_EQUAL(splitter.next(), "four"_el);
        REQUIRE(splitter.isAtEnd());
    }

    void testEmptyAndNoSeparator() {
        auto empty = StringSplitter{""_el, Char{U','}};
        REQUIRE_FALSE(empty.isAtEnd());
        REQUIRE(empty.next().isEmpty());
        REQUIRE(empty.isAtEnd());

        auto noSeparator = StringSplitter{"complete"_el, CharSet{}};
        REQUIRE_EQUAL(noSeparator.next(), "complete"_el);
        REQUIRE(noSeparator.isAtEnd());
    }

    void testResetAndSourceAccess() {
        auto splitter = StringSplitter{"a,b"_el, Char{U','}};
        REQUIRE_EQUAL(splitter.text(), "a,b"_el);
        REQUIRE_EQUAL(splitter.next(), "a"_el);
        REQUIRE_EQUAL(splitter.remaining(), "b"_el);

        splitter.reset();
        REQUIRE_FALSE(splitter.isAtEnd());
        REQUIRE_EQUAL(splitter.remaining(), "a,b"_el);
        REQUIRE_EQUAL(splitter.next(), "a"_el);
    }

    void testSkipPartsWithoutCreatingSlices() {
        auto discard = StringSplitter{"first,,third"_el, Char{U','}};
        discard.skip();
        REQUIRE_EQUAL(discard.remaining(), ",third"_el);
        discard.skip();
        REQUIRE_EQUAL(discard.next(), "third"_el);
        REQUIRE(discard.isAtEnd());
        discard.skip();
        REQUIRE(discard.isAtEnd());

        auto keep = StringSplitter{"one\ntwo\n"_el, Char{U'\n'}, StringSplitMode::KeepSeparator};
        keep.skip();
        REQUIRE_EQUAL(keep.remaining(), "two\n"_el);
        keep.skip();
        REQUIRE(keep.isAtEnd());
    }

    void testMalformedUtf8IsPreserved() {
        auto malformed = std::string{"left"};
        malformed.push_back(static_cast<char>(0x80U));
        malformed.append(",right");
        auto splitter = StringSplitter{String{malformed}, Char{U','}};

        const auto first = splitter.next();
        REQUIRE_FALSE(first.isValidUtf8());
        REQUIRE_EQUAL(first.length().toSizeT(), std::size_t{5U});
        REQUIRE_EQUAL(splitter.next(), "right"_el);
        REQUIRE(splitter.isAtEnd());
    }

    void testWidthVariants() {
        auto splitter16 = U16StringSplitter{u"a☃b☃"_el, Char{U'☃'}};
        REQUIRE_EQUAL(splitter16.next(), u"a"_el);
        REQUIRE_EQUAL(splitter16.next(), u"b"_el);
        REQUIRE_EQUAL(splitter16.next(), u""_el);
        REQUIRE(splitter16.isAtEnd());

        auto splitter32 = U32StringSplitter{U"a☃b☃"_el, Char{U'☃'}, StringSplitMode::KeepSeparator};
        REQUIRE_EQUAL(splitter32.next(), U"a☃"_el);
        REQUIRE_EQUAL(splitter32.next(), U"b☃"_el);
        REQUIRE(splitter32.isAtEnd());
    }
};
