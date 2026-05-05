// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/cterm/ParagraphIndents.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(ParagraphIndents)
class ParagraphIndentsTest final : public el::UnitTest {
public:
    void testDefaultConstructionUsesZeroIndentsAndZeroMargins() {
        const auto indents = ParagraphIndents{};

        REQUIRE_EQUAL(indents.lineIndent(), 0);
        REQUIRE_EQUAL(indents.firstLineIndent(), 0);
        REQUIRE_EQUAL(indents.wrappedLineIndent(), 0);
        REQUIRE_EQUAL(indents.margins(), bgeo::BlockMargins{0});
    }

    void testSettersClampValuesAndResolveUseLineIndent() {
        auto indents = ParagraphIndents{};

        indents.setLineIndent(-4);
        indents.setFirstLineIndent(ParagraphIndents::cUseLineIndent);
        indents.setWrappedLineIndent(-3);
        indents.setMargins(bgeo::BlockMargins{1, 2, 3, 4});

        REQUIRE_EQUAL(indents.lineIndent(), 0);
        REQUIRE_EQUAL(indents.firstLineIndent(), 0);
        REQUIRE_EQUAL(indents.wrappedLineIndent(), 0);
        REQUIRE_EQUAL(indents.margins(), bgeo::BlockMargins(1, 2, 3, 4));
    }

    void testExplicitValuesAndEqualityArePreserved() {
        auto indents = ParagraphIndents{2, 4, 6, bgeo::BlockMargins{3, 1}};
        const auto same = ParagraphIndents{2, 4, 6, bgeo::BlockMargins{3, 1}};
        const auto different = ParagraphIndents{2, 4, 5, bgeo::BlockMargins{3, 1}};

        REQUIRE_EQUAL(indents.lineIndent(), 2);
        REQUIRE_EQUAL(indents.firstLineIndent(), 4);
        REQUIRE_EQUAL(indents.wrappedLineIndent(), 6);
        REQUIRE_EQUAL(indents.margins(), bgeo::BlockMargins(3, 1));
        REQUIRE_EQUAL(indents, same);
        REQUIRE_NOT_EQUAL(indents, different);
    }
};
