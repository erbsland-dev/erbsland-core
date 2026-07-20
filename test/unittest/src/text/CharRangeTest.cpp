// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/CharRange.hpp>
#include <erbsland/text/StdFormatForText.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <set>

TESTED_TARGETS(CharRange)
class CharRangeTest final : public el::UnitTest {
public:
    void testConstruction() {
        using Char = el::text::Char;
        using CharRange = el::text::CharRange;

        constexpr auto empty = CharRange{};
        constexpr auto single = CharRange{Char{U'A'}};
        constexpr auto ordered = CharRange{Char{U'A'}, Char{U'Z'}};
        constexpr auto reversed = CharRange{Char{U'Z'}, Char{U'A'}};
        constexpr auto invalidSingle = CharRange{Char{0x110000U}};
        constexpr auto invalidRange = CharRange{Char{U'A'}, Char{0xD800U}};

        static_assert(empty.isEmpty());
        static_assert(single.isSingleChar());
        static_assert(ordered.from() == Char{U'A'});
        static_assert(ordered.to() == Char{U'Z'});
        static_assert(reversed.from() == Char{U'A'});
        static_assert(reversed.to() == Char{U'Z'});
        static_assert(invalidSingle.isEmpty());
        static_assert(invalidRange.isEmpty());

        REQUIRE(empty.isEmpty());
        REQUIRE(single.isSingleChar());
        REQUIRE_EQUAL(reversed, ordered);
        REQUIRE(invalidSingle.isEmpty());
        REQUIRE(invalidRange.isEmpty());
    }

    void testContainsAndUnicodeValidity() {
        using Char = el::text::Char;
        using CharRange = el::text::CharRange;

        constexpr auto range = CharRange{Char{U'A'}, Char{U'C'}};
        constexpr auto acrossSurrogateGap = CharRange{Char{0xD7FFU}, Char{0xE000U}};
        constexpr auto all = CharRange::all();

        static_assert(range.contains(Char{U'B'}));
        static_assert(!range.contains(Char{U'D'}));
        static_assert(!range.contains(Char{0xD800U}));
        static_assert(acrossSurrogateGap.contains(Char{0xD7FFU}));
        static_assert(!acrossSurrogateGap.contains(Char{0xD800U}));
        static_assert(acrossSurrogateGap.contains(Char{0xE000U}));
        static_assert(all.contains(Char{0U}));
        static_assert(all.contains(Char{0x10FFFFU}));
        static_assert(!all.contains(Char{0x110000U}));

        REQUIRE(range.contains(Char{U'B'}));
        REQUIRE_FALSE(range.contains(Char{U'D'}));
        REQUIRE_FALSE(range.contains(Char{0xD800U}));
        REQUIRE(acrossSurrogateGap.contains(Char{0xD7FFU}));
        REQUIRE_FALSE(acrossSurrogateGap.contains(Char{0xD800U}));
        REQUIRE(acrossSurrogateGap.contains(Char{0xE000U}));
    }

    void testOrderingAndMerging() {
        using Char = el::text::Char;
        using CharRange = el::text::CharRange;

        constexpr auto ac = CharRange{Char{U'A'}, Char{U'C'}};
        constexpr auto df = CharRange{Char{U'D'}, Char{U'F'}};
        constexpr auto eg = CharRange{Char{U'E'}, Char{U'G'}};
        constexpr auto xz = CharRange{Char{U'X'}, Char{U'Z'}};
        constexpr auto beforeSurrogates = CharRange{Char{0xD7FFU}};
        constexpr auto afterSurrogates = CharRange{Char{0xE000U}};

        static_assert(ac < xz);
        static_assert(ac.isAdjacentTo(df));
        static_assert(ac.canMergeWith(df));
        static_assert(df.overlaps(eg));
        static_assert(beforeSurrogates.isAdjacentTo(afterSurrogates));
        static_assert(!ac.canMergeWith(xz));
        static_assert(ac.mergedWith(df) == CharRange{Char{U'A'}, Char{U'F'}});
        static_assert(df.mergedWith(eg) == CharRange{Char{U'D'}, Char{U'G'}});
        static_assert(ac.mergedWith(xz).isEmpty());

        auto ranges = std::set<CharRange>{xz, ac, df};
        REQUIRE_EQUAL(ranges.begin()->from(), Char{U'A'});
        REQUIRE(beforeSurrogates.canMergeWith(afterSurrogates));
    }

    void testUnicodeMappingPredicates() {
        using Char = el::text::Char;
        using CharRange = el::text::CharRange;

        const auto empty = CharRange{};
        const auto digits = CharRange{Char{U'0'}, Char{U'9'}};
        const auto uppercaseAscii = CharRange{Char{U'A'}, Char{U'Z'}};
        const auto lowercaseAscii = CharRange{Char{U'a'}, Char{U'z'}};
        const auto latinCapitalADiaeresis = CharRange{Char{0x00C4U}};
        const auto latinSmallADiaeresis = CharRange{Char{0x00E4U}};
        const auto kelvinSign = CharRange{Char{0x212AU}};

        REQUIRE_FALSE(empty.containsCaseFoldableCharacters());
        REQUIRE_FALSE(digits.containsCaseFoldableCharacters());
        REQUIRE_FALSE(digits.containsLowercaseMappableCharacters());
        REQUIRE_FALSE(digits.containsUppercaseMappableCharacters());

        REQUIRE(uppercaseAscii.containsCaseFoldableCharacters());
        REQUIRE(uppercaseAscii.containsLowercaseMappableCharacters());
        REQUIRE_FALSE(uppercaseAscii.containsUppercaseMappableCharacters());

        REQUIRE_FALSE(lowercaseAscii.containsCaseFoldableCharacters());
        REQUIRE_FALSE(lowercaseAscii.containsLowercaseMappableCharacters());
        REQUIRE(lowercaseAscii.containsUppercaseMappableCharacters());

        REQUIRE(latinCapitalADiaeresis.containsCaseFoldableCharacters());
        REQUIRE(latinCapitalADiaeresis.containsLowercaseMappableCharacters());
        REQUIRE(latinSmallADiaeresis.containsUppercaseMappableCharacters());
        REQUIRE(kelvinSign.containsCaseFoldableCharacters());
        REQUIRE(kelvinSign.containsLowercaseMappableCharacters());
    }
};
