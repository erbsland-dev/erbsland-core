// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../StringHelper.hpp"

#include <erbsland/re/impl/text/CharClass.hpp>
#include <erbsland/re/RegExError.hpp>
#include <erbsland/re/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::re;
using namespace el::text::literals;
using el::text::Char;
using impl::CharClass;

TESTED_TARGETS(CharClass)
TAGS(Text CharClasses)
class CharClassTest final : public el::UnitTest {
public:
    CharClass charClass;

    void testDefault() {
        charClass = {};
        charClass.prepareForUse();
        REQUIRE_FALSE(charClass.matches(Char{0U}));
        REQUIRE_FALSE(charClass.matches(Char{U'a'}));
        REQUIRE_FALSE(charClass.matches(Char{0x10000}));
        REQUIRE_FALSE(charClass.matches(Char{0xffffffff}));
        REQUIRE_EQUAL(charClass.toString(), ""_el);
    }

    void testAddSingleCharAndNormalization() {
        charClass = {};
        // add a single character
        charClass.add(Char{U'b'});
        charClass.prepareForUse();
        REQUIRE(charClass.matches(Char{U'b'}));
        REQUIRE_FALSE(charClass.matches(Char{U'a'}));
        REQUIRE_EQUAL(charClass.toString(), "b"_el);

        // adding adjacent chars merges into a single range
        charClass.add(Char{U'a'}); // now a-b
        charClass.prepareForUse();
        REQUIRE(charClass.matches(Char{U'a'}));
        REQUIRE(charClass.matches(Char{U'b'}));
        REQUIRE_EQUAL(charClass.toString(), "a-b"_el);

        // adding c merges again -> a-c
        charClass.add(Char{U'c'});
        charClass.prepareForUse();
        REQUIRE(charClass.matches(Char{U'c'}));
        REQUIRE_EQUAL(charClass.toString(), "a-c"_el);

        // adding duplicate character keeps normalization stable
        charClass.add(Char{U'c'});
        charClass.prepareForUse();
        REQUIRE_EQUAL(charClass.toString(), "a-c"_el);

        // add a non-adjacent high code point
        charClass.add(Char{0x1F600U});
        charClass.prepareForUse();
        REQUIRE(charClass.matches(Char{0x1F600U}));
        REQUIRE_FALSE(charClass.matches(Char{0x1F601U}));
        REQUIRE_EQUAL(charClass.toString(), "a-c\\u{1F600}"_el);
    }

    void testAddRangeOverlapAdjacencyAndBridging() {
        charClass = {};
        // overlapping ranges merge
        charClass.add(Char{U'a'}, Char{U'f'});
        charClass.add(Char{U'd'}, Char{U'z'});
        charClass.prepareForUse();
        REQUIRE(charClass.matches(Char{U'a'}));
        REQUIRE(charClass.matches(Char{U'z'}));
        REQUIRE_EQUAL(charClass.toString(), "a-z"_el);

        // add a disjoint range: digits
        charClass = {};
        charClass.add(Char{U'a'}, Char{U'z'});
        charClass.add(Char{U'0'}, Char{U'9'});
        charClass.prepareForUse();
        REQUIRE_EQUAL(charClass.toString(), "0-9a-z"_el);

        // bridge the gap between '9' and 'a' so everything merges
        charClass.add(Char{U':'}, Char{U'`'}); // ':' (0x3A) .. '`' (0x60)
        charClass.prepareForUse();
        REQUIRE_EQUAL(charClass.toString(), "0-z"_el);
        REQUIRE(charClass.matches(Char{U'@'}));
        REQUIRE(charClass.matches(Char{U'`'}));
        REQUIRE(charClass.matches(Char{U'a'}));
    }

    void testConstructorNormalizationFromVector() {
        // unsorted, overlapping and adjacent input must normalize and sort
        std::vector<el::re::impl::CharRange> input{
            {Char{U'd'}, Char{U'f'}},
            {Char{U'a'}, Char{U'c'}},
            {Char{U'b'}, Char{U'e'}},
            {Char{U'h'}, Char{U'h'}},
            {Char{U'g'}, Char{U'g'}},
        };
        CharClass fromVec{std::move(input)};
        fromVec.prepareForUse();
        REQUIRE_EQUAL(fromVec.toString(), "a-h"_el); // g and h are adjacent to f after merge

        // Build the same via add in different order and ensure equality
        CharClass built;
        built.add(Char{U'a'}, Char{U'c'});
        built.add(Char{U'd'}, Char{U'f'});
        built.add(Char{U'h'}, Char{U'h'});
        built.add(Char{U'g'}, Char{U'g'});
        built.prepareForUse();
        REQUIRE_EQUAL(fromVec, built);
        REQUIRE_EQUAL(fromVec, built);
    }

    void testSortTieOnFirstUsesLast() {
        // Both ranges start at the same character, but with different ends.
        // After normalization they must merge to the longer one.
        std::vector<el::re::impl::CharRange> input{
            {Char{U'a'}, Char{U'b'}}, {Char{U'a'}, Char{U'd'}}, {Char{U'f'}, Char{U'f'}}};
        CharClass ranges{std::move(input)};
        ranges.prepareForUse();
        REQUIRE_EQUAL(ranges.toString(), "a-df"_el);
        REQUIRE(ranges.matches(Char{U'c'}));
        REQUIRE_FALSE(ranges.matches(Char{U'e'}));
    }

    void testEquality() {
        CharClass x;
        x.add(Char{U'a'}, Char{U'c'});
        x.add(Char{U'f'}, Char{U'h'});

        CharClass y;
        y.add(Char{U'f'}, Char{U'h'});
        y.add(Char{U'c'}, Char{U'a'}); // reversed order on purpose

        x.prepareForUse();
        y.prepareForUse();
        REQUIRE_EQUAL(x, y);
        REQUIRE_EQUAL(x, y);
    }

    void testMatchesBoundariesAndGaps() {
        charClass = {};
        charClass.add(Char{U'a'}, Char{U'c'});
        charClass.add(Char{U'f'}, Char{U'h'});
        charClass.add(Char{0x1F600U}, Char{0x1F601U}); // 😀-😁
        charClass.prepareForUse();

        // boundaries within ASCII ranges
        REQUIRE(charClass.matches(Char{U'a'}));
        REQUIRE(charClass.matches(Char{U'c'}));
        REQUIRE_FALSE(charClass.matches(Char{U'A'})); // below first range
        REQUIRE_FALSE(charClass.matches(Char{U'd'}));
        REQUIRE_FALSE(charClass.matches(Char{U'e'}));
        REQUIRE(charClass.matches(Char{U'f'}));
        REQUIRE(charClass.matches(Char{U'h'}));
        REQUIRE_FALSE(charClass.matches(Char{U'i'}));

        // boundaries within high Unicode range
        REQUIRE(charClass.matches(Char{0x1F600U}));
        REQUIRE(charClass.matches(Char{0x1F601U}));
        REQUIRE_FALSE(charClass.matches(Char{0x1F602U}));
    }

    void testCopyAndMoveSemantics() {
        CharClass original;
        original.add(Char{U'a'}, Char{U'c'});
        original.add(Char{0x1F600U});
        original.prepareForUse();
        REQUIRE_EQUAL(original.toString(), "a-c\\u{1F600}"_el);

        // copy ctor
        CharClass copy{original};
        REQUIRE_EQUAL(copy, original);

        // move ctor
        CharClass moveCtor{std::move(copy)};
        REQUIRE_EQUAL(moveCtor, original);

        // copy assignment
        CharClass assigned;
        assigned = original;
        REQUIRE_EQUAL(assigned, original);

        // move assignment (content should move; we check the target has expected state)
        CharClass movedTo;
        movedTo = std::move(original);
        REQUIRE_EQUAL(movedTo.toString(), "a-c\\u{1F600}"_el);
        REQUIRE(movedTo.matches(Char{U'b'}));
        REQUIRE(movedTo.matches(Char{0x1F600U}));
    }

    void testNormalizationIdempotenceOnIncrementalAdds() {
        charClass = {};
        charClass.add(Char{U'a'}, Char{U'c'});
        charClass.prepareForUse();
        REQUIRE_EQUAL(charClass.toString(), "a-c"_el);
        charClass.add(Char{U'c'}, Char{U'e'}); // adjacency/overlap
        charClass.prepareForUse();
        REQUIRE_EQUAL(charClass.toString(), "a-e"_el);
        charClass.add(Char{U'b'}, Char{U'd'}); // contained
        charClass.prepareForUse();
        REQUIRE_EQUAL(charClass.toString(), "a-e"_el);
        charClass.add(Char{U'g'}); // disjoint
        charClass.prepareForUse();
        REQUIRE_EQUAL(charClass.toString(), "a-eg"_el);
        charClass.add(Char{U'f'}); // bridge e..g to merge into a-g
        charClass.prepareForUse();
        REQUIRE_EQUAL(charClass.toString(), "a-g"_el);
        // ensure membership across the merged span
        REQUIRE(charClass.matches(Char{U'a'}));
        REQUIRE(charClass.matches(Char{U'g'}));
        REQUIRE_FALSE(charClass.matches(Char{U'h'}));
    }

    void testToStringCaretPrefixAndJoin() {
        charClass = {};
        charClass.add(Char{U'a'}, Char{U'c'});
        charClass.add(Char{0x1F600U});
        charClass.prepareForUse();
        REQUIRE_EQUAL(charClass.toString(), "a-c\\u{1F600}"_el);
    }

    void testConditionalDetachOnSharedData() {
        // Build and prepare a base instance
        CharClass base;
        base.add(Char{U'a'}, Char{U'c'});
        base.prepareForUse();
        REQUIRE(base.matches(Char{U'b'}));

        // Create a shared copy (both share the same internal data object)
        CharClass shared = base;

        // Trigger conditionalDetach() via add() on the copy while data is shared
        shared.add(Char{U'x'});
        // Prepare both for use again (also triggers conditionalDetach() path internally if needed)
        shared.prepareForUse();
        base.prepareForUse();

        // Both instances must remain valid and usable
        REQUIRE(shared.matches(Char{U'x'}));
        REQUIRE(shared.matches(Char{U'a'}));
        REQUIRE(base.matches(Char{U'a'}));

        // We do not assert copy-on-write semantics here; the goal is to exercise
        // CharClass::conditionalDetach() code path safely.
    }

    void testOperatorEqualsReturnsFalseOnDifferentHashes() {
        // Two different prepared ranges must not be equal; this exercises
        // the fast-hash inequality path and the return at line 65 in CharClass.cpp
        CharClass letters;
        letters.add(Char{U'a'}, Char{U'z'});
        letters.prepareForUse();

        CharClass digits;
        digits.add(Char{U'0'}, Char{U'9'});
        digits.prepareForUse();

        REQUIRE_NOT_EQUAL(letters, digits);
        REQUIRE_NOT_EQUAL(letters, digits);
    }

    void testNullCharacterAndInvalidCharacterBoundary() {
        charClass = {};
        charClass.add(Char::null());
        charClass.prepareForUse();
        REQUIRE(charClass.matches(Char::null()));
        REQUIRE_EQUAL(charClass.toString(), "\\u0000"_el);

        REQUIRE_THROWS_AS(RegExError, charClass.add(Char::endOfData()));
        REQUIRE_THROWS_AS(RegExError, charClass.add(Char{0xD800U}, Char{0xE000U}));

        CharClass fromInvalidRange{std::vector<el::re::impl::CharRange>{{Char::endOfData(), Char::endOfData()}}};
        REQUIRE_THROWS_AS(RegExError, fromInvalidRange.prepareForUse());
    }
};
