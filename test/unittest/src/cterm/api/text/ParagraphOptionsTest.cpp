// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

#include <stdexcept>

TESTED_TARGETS(ParagraphOptions)
class ParagraphOptionsTest final : public el::UnitTest {
public:
    void testDefaultWordSeparatorsUseTheSharedSpaceAndTabSet() {
        const auto options = ParagraphOptions{};
        const auto expectedWordSeparators = erbsland::text::CharSet{"\t "_el};

        REQUIRE_EQUAL(options.wordSeparatorSet(), expectedWordSeparators);
        REQUIRE_EQUAL(options.wordSeparators(), U"\t "_el);
    }

    void testLineBreakMarksAcceptValidShortStrings() {
        auto options = ParagraphOptions{};

        options.setLineBreakEndMark(BlockStringEditor{U"->"_el});
        options.setLineBreakStartMark(BlockStringEditor{U"<-"_el});

        REQUIRE_EQUAL(options.lineBreakEndMark().length(), BlockCount{2U});
        REQUIRE_EQUAL(options.lineBreakEndMark()[BlockIndex{0U}], U'-');
        REQUIRE_EQUAL(options.lineBreakEndMark()[BlockIndex{1U}], U'>');
        REQUIRE_EQUAL(options.lineBreakStartMark().length(), BlockCount{2U});
        REQUIRE_EQUAL(options.lineBreakStartMark()[BlockIndex{0U}], U'<');
        REQUIRE_EQUAL(options.lineBreakStartMark()[BlockIndex{1U}], U'-');
    }

    void testLineBreakEndMarkRejectsTooLongOrControlText() {
        auto options = ParagraphOptions{};

        REQUIRE_THROWS_AS(erbsland::err::ParameterError, options.setLineBreakEndMark(BlockStringEditor{U"abc"_el}));
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, options.setLineBreakEndMark(BlockStringEditor{U"\n"_el}));
    }

    void testLineBreakStartMarkAllowsLongTextButRejectsControlText() {
        auto options = ParagraphOptions{};

        options.setLineBreakStartMark(BlockStringEditor{U"│ │        "_el});
        REQUIRE_EQUAL(options.lineBreakStartMark().length(), BlockCount{11U});
        REQUIRE_THROWS_AS(erbsland::err::ParameterError, options.setLineBreakStartMark(BlockStringEditor{U"\t"_el}));
    }

    void testIndentsAndMarginsCanBeConfiguredAsOneValueObject() {
        auto options = ParagraphOptions{};
        auto indents = ParagraphIndents{2, 4, 6, bgeo::BlockMargins{1, 3}};

        options.setIndents(indents);

        REQUIRE_EQUAL(options.indents(), indents);
        REQUIRE_EQUAL(options.lineIndent(), 2);
        REQUIRE_EQUAL(options.firstLineIndent(), 4);
        REQUIRE_EQUAL(options.wrappedLineIndent(), 6);
        REQUIRE_EQUAL(options.margins(), bgeo::BlockMargins(1, 3));
    }

    void testWordSeparatorsAreCanonicalizedAndReuseSharedDefaults() {
        auto options = ParagraphOptions{};
        const auto defaultWordSeparators = erbsland::text::CharSet{"\t "_el};

        options.setWordSeparators(U" \t\t "_el);

        REQUIRE_EQUAL(options.wordSeparatorSet(), defaultWordSeparators);
        REQUIRE_EQUAL(options.wordSeparators(), U"\t "_el);

        options.setWordSeparators(U".,.,"_el);

        REQUIRE_NOT_EQUAL(options.wordSeparatorSet(), defaultWordSeparators);
        REQUIRE_EQUAL(options.wordSeparators(), U",."_el);
        REQUIRE(options.wordSeparatorSet().contains(U','));
        REQUIRE(options.wordSeparatorSet().contains(U'.'));
        REQUIRE(!options.wordSeparatorSet().contains(U';'));
    }
};
