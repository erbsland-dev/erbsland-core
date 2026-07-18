// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserBase.hpp"

#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u32/U32String.hpp>

TESTED_TARGETS(Parser)
TAGS(Parsing)
class ParserPatternLengthTest final : public UNITTEST_SUBCLASS(ParserBase) {
public:
    void testWithinBuiltInLimit() {
        // Default settings with a very small pattern — must succeed.
        parser = Parser{"abc"_el};
        REQUIRE_NOTHROW(node = parser.parse());
    }

    void testCustomLimitExactBoundary() {
        // At exactly the custom limit the parser rejects (position >= limit triggers error).
        Settings settings;
        settings.setMaximumPatternLength(el::unit::CpLength{3U});

        parser = Parser{"abc"_el, {}, settings}; // size == limit
        REQUIRE_THROWS(node = parser.parse());
    }

    void testExceedWithAsciiCharacters() {
        Settings settings;
        settings.setMaximumPatternLength(el::unit::CpLength{2U});

        parser = Parser{"aaaaaaaaaaaa"_el, {}, settings};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testExceedByCharacterCountOnly() {
        Settings settings;
        settings.setMaximumPatternLength(el::unit::CpLength{5U});

        parser = Parser{"aaaaaa"_el, {}, settings};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testExceedByCharacterCountOnlyWithMultibyte() {
        Settings settings;
        settings.setMaximumPatternLength(el::unit::CpLength{2U});

        parser = Parser{"ééééé"_el, {}, settings};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testUtf16AndUtf32LimitsCountCodePoints() {
        Settings settings;
        settings.setMaximumPatternLength(el::unit::CpLength{3U});

        parser = Parser{el::text::U16String{u"😀😀😀"_el}, {}, settings};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{el::text::U32String{U"😀😀😀"_el}, {}, settings};
        REQUIRE_THROWS(node = parser.parse());

        settings.setMaximumPatternLength(el::unit::CpLength{4U});
        parser = Parser{el::text::U16String{u"😀😀😀"_el}, {}, settings};
        REQUIRE_NOTHROW(node = parser.parse());
        parser = Parser{el::text::U32String{U"😀😀😀"_el}, {}, settings};
        REQUIRE_NOTHROW(node = parser.parse());
    }
};
