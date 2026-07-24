// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../StringHelper.hpp"
#include "../TestHelper.hpp"

#include <erbsland/re/impl/text/CharRange.hpp>
#include <erbsland/re/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::re;
using namespace el::text::literals;
using el::text::Char;
using impl::CharRange;

TESTED_TARGETS(CharRange)
TAGS(Text)
class ReCharRangeTest final : public UNITTEST_SUBCLASS(re_test::TestHelper) {
public:
    CharRange charRange;

    void testDefault() {
        charRange = {};
        REQUIRE_EQUAL(charRange.first(), Char{0U});
        REQUIRE_EQUAL(charRange.last(), Char{0U});
        REQUIRE(charRange.matches(Char{0U}));
        REQUIRE_FALSE(charRange.matches(Char{1U}));
        REQUIRE_EQUAL(charRange.toString(), "\\u0000"_el);
    }

    void testConstructorOrderingAndAccessors() {
        // ordered
        charRange = CharRange{U'a', U'z'};
        REQUIRE_EQUAL(charRange.first(), Char{U'a'});
        REQUIRE_EQUAL(charRange.last(), Char{U'z'});
        // reversed gets reordered automatically
        charRange = CharRange{U'z', U'a'};
        REQUIRE_EQUAL(charRange.first(), Char{U'a'});
        REQUIRE_EQUAL(charRange.last(), Char{U'z'});
    }

    void testCopyAndAssignment() {
        CharRange a{U'a', U'z'};
        CharRange b{a}; // copy ctor
        REQUIRE(a == b);
        CharRange c;    // assignment
        c = a;
        REQUIRE(a == c);
    }

    void testOperators() {
        const auto a1 = CharRange{U'a'};
        const auto b1 = CharRange{U'a'};
        const auto a2 = CharRange{U'a', U'z'};
        const auto b2 = CharRange{U'a', U'z'};
        const auto a3 = CharRange{U'x', U'z'};
        const auto b3 = CharRange{U'x', U'z'};
        WITH_CONTEXT(requireAllOperators(a1, a2, a3, b1, b2, b3))
        constexpr auto orderedSequence = std::array{
            CharRange{Char{0x0000U}, Char{0x0000U}},
            CharRange{Char{U'a'}, Char{U'g'}},
            CharRange{Char{U'c'}, Char{U'g'}},
            CharRange{Char{U'c'}, Char{U'z'}},
            CharRange{Char{U'←'}, Char{U'→'}},
            CharRange{Char{U'😀'}, Char{U'😄'}},
        };
        WITH_CONTEXT(requireStrictOrder(orderedSequence));
    }

    void testMatchesBoundariesAndOutside() {
        charRange = CharRange{Char{U'a'}, Char{U'z'}};
        REQUIRE_FALSE(charRange.matches(Char{U'A'})); // below
        REQUIRE(charRange.matches(Char{U'a'}));       // first
        REQUIRE(charRange.matches(Char{U'm'}));       // inside
        REQUIRE(charRange.matches(Char{U'z'}));       // last
        REQUIRE_FALSE(charRange.matches(Char{U'{'})); // above ('z'+1)

        // reversed construction still matches correctly
        charRange = CharRange{Char{U'z'}, Char{U'a'}};
        REQUIRE(charRange.matches(Char{U'a'}));
        REQUIRE(charRange.matches(Char{U'z'}));
        REQUIRE_FALSE(charRange.matches(Char{U'{'}));
    }

    void testToStringSingleCharactersAndEscapes() {
        // safe ASCII single character
        charRange = CharRange{Char{U'a'}, Char{U'a'}};
        REQUIRE_EQUAL(charRange.toString(), "a"_el);

        // space must be escaped as \u0020
        charRange = CharRange{Char{U' '}, Char{U' '}};
        REQUIRE_EQUAL(charRange.toString(), "\\u0020"_el);

        // backslash escaping is verified below as range endpoint (same code path)

        // hyphen and caret must be escaped as hex
        charRange = CharRange{Char{U'-'}, Char{U'-'}};
        REQUIRE_EQUAL(charRange.toString(), "\\u002D"_el);
        charRange = CharRange{Char{U'^'}, Char{U'^'}};
        REQUIRE_EQUAL(charRange.toString(), "\\u005E"_el);

        // DEL and non-ASCII must be escaped
        charRange = CharRange{Char{0x7FU}, Char{0x7FU}};
        REQUIRE_EQUAL(charRange.toString(), "\\u007F"_el);
        charRange = CharRange{Char{0x00E4U}, Char{0x00E4U}}; // ä
        REQUIRE_EQUAL(charRange.toString(), "\\u00E4"_el);

        // non-BMP must be escaped with \U and 8 hex digits
        charRange = CharRange{Char{0x1F600U}, Char{0x1F600U}}; // 😀
        REQUIRE_EQUAL(charRange.toString(), "\\u{1F600}"_el);
    }

    void testScalarSafeAdjacency() {
        REQUIRE(CharRange::canMerge(Char{0xD7FFU}, Char{0xE000U}));
        REQUIRE_FALSE(CharRange::canMerge(Char{0xD7FFU}, Char{0xE001U}));
        REQUIRE_FALSE(CharRange::canMerge(Char{0x10FFFFU}, Char{0x10FFFFU + 1U}));
        REQUIRE_FALSE(CharRange::canMerge(Char::endOfData(), Char::endOfData()));
    }

    void testToStringRangesFormatting() {
        // simple ASCII range
        charRange = CharRange{Char{U'a'}, Char{U'z'}};
        REQUIRE_EQUAL(charRange.toString(), "a-z"_el);

        // endpoints that need escaping keep hyphen separator
        charRange = CharRange{Char{U'\t'}, Char{U'\n'}}; // 0x09 - 0x0A
        REQUIRE_EQUAL(charRange.toString(), "\\u0009-\\u000A"_el);

        // mix of escaped and unescaped endpoints
        charRange = CharRange{Char{U'^'}, Char{U'a'}}; // reversed; '^' gets escaped
        REQUIRE_EQUAL(charRange.toString(), "\\u005E-a"_el);

        // include backslash as an endpoint
        charRange = CharRange{Char{U'0'}, Char{0x5CU}};
        REQUIRE_EQUAL(charRange.toString(), "0-\\\\"_el);

        // high Unicode range
        charRange = CharRange{Char{0x1F600U}, Char{0x1F601U}}; // 😀-😁
        REQUIRE_EQUAL(charRange.toString(), "\\u{1F600}-\\u{1F601}"_el);
    }
};
