// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(BlockTextOptions)
class BlockTextOptionsTest final : public el::UnitTest {
public:
    void testDefaultOptionsExposeTheExpectedDefaults() {
        const auto options = BlockTextOptions{};

        REQUIRE(options.colorSequence().empty());
        REQUIRE_EQUAL(options.color(), Color{});
        REQUIRE_EQUAL(options.font(), nullptr);
        REQUIRE_EQUAL(options.animation(), BlockTextAnimation::None);
        REQUIRE_EQUAL(options.alignment(), bgeo::Alignment::TopLeft);
        REQUIRE_EQUAL(options.lineIndent(), 0);
        REQUIRE_EQUAL(options.firstLineIndent(), 0);
        REQUIRE_EQUAL(options.wrappedLineIndent(), 0);
        REQUIRE_EQUAL(options.margins(), bgeo::BlockMargins{0});
        REQUIRE_EQUAL(options.backgroundMode(), ParagraphBackgroundMode::Default);
        REQUIRE(options.lineBreakEndMark().isEmpty());
        REQUIRE(options.lineBreakStartMark().isEmpty());
        REQUIRE_EQUAL(options.paragraphSpacing(), ParagraphSpacing::SingleLine);
        REQUIRE_EQUAL(options.wordSeparators(), U"\t "_el);
        REQUIRE_EQUAL(options.wordBreakMark(), U'-');
        REQUIRE_EQUAL(options.maximumLineWraps(), 0);
        requireStringEqual(options.paragraphEllipsisMark(), U"…"_el);
        const auto tabStopCount = options.tabStops().size();
        REQUIRE_EQUAL(tabStopCount, std::size_t{1});
        REQUIRE_EQUAL(options.tabStops()[0], ParagraphOptions::cTabWrappedLineIndent);
        REQUIRE_EQUAL(options.tabOverflowBehavior(), TabOverflowBehavior::AddSpace);
        REQUIRE_EQUAL(options.onError(), ParagraphOnError::PlainOutput);
    }

    void testColorFontAnimationAndParagraphOptionsCanBeConfigured() {
        auto options = BlockTextOptions{bgeo::Alignment::Center};
        auto paragraphOptions = ParagraphOptions{bgeo::Alignment::BottomRight};
        const auto font = std::make_shared<Font>(3);

        paragraphOptions.setLineIndent(2);
        paragraphOptions.setFirstLineIndent(4);
        paragraphOptions.setWrappedLineIndent(6);
        paragraphOptions.setMargins(bgeo::BlockMargins{1, 2});
        paragraphOptions.setBackgroundMode(ParagraphBackgroundMode::FullBoth);
        paragraphOptions.setLineBreakEndMark(BlockStringEditor{"<"_el});
        paragraphOptions.setLineBreakStartMark(BlockStringEditor{">"_el});
        paragraphOptions.setParagraphSpacing(ParagraphSpacing::DoubleLine);
        paragraphOptions.setWordSeparators(U".,"_el);
        paragraphOptions.setWordBreakMark(Block{U'~'});
        paragraphOptions.setMaximumLineWraps(5);
        paragraphOptions.setParagraphEllipsisMark(BlockStringEditor{"..."_el});
        paragraphOptions.setTabStops({2, 8});
        paragraphOptions.setTabOverflowBehavior(TabOverflowBehavior::LineBreak);
        paragraphOptions.setOnError(ParagraphOnError::Empty);

        options.setColorSequence(ColorSequence{Color{fg::Red, bg::Black}, Color{fg::Blue, bg::Black}});
        options.setFont(font);
        options.setAnimation(BlockTextAnimation::ColorDiagonal);
        options.setParagraphOptions(paragraphOptions);

        const auto colorSequenceLength = options.colorSequence().sequenceLength();
        REQUIRE_EQUAL(colorSequenceLength, std::size_t{2});
        REQUIRE_EQUAL(options.color(), Color(fg::Red, bg::Black));
        REQUIRE_EQUAL(options.font(), font);
        REQUIRE_EQUAL(options.animation(), BlockTextAnimation::ColorDiagonal);
        const auto alignment = options.paragraphOptions().alignment();
        REQUIRE_EQUAL(alignment, bgeo::Alignment::BottomRight);
        REQUIRE_EQUAL(options.lineIndent(), 2);
        REQUIRE_EQUAL(options.firstLineIndent(), 4);
        REQUIRE_EQUAL(options.wrappedLineIndent(), 6);
        REQUIRE_EQUAL(options.margins(), bgeo::BlockMargins(1, 2));
        REQUIRE_EQUAL(options.backgroundMode(), ParagraphBackgroundMode::FullBoth);
        requireStringEqual(options.lineBreakEndMark(), U"<"_el);
        requireStringEqual(options.lineBreakStartMark(), U">"_el);
        REQUIRE_EQUAL(options.paragraphSpacing(), ParagraphSpacing::DoubleLine);
        REQUIRE_EQUAL(options.wordSeparators(), U",."_el);
        REQUIRE_EQUAL(options.wordBreakMark(), U'~');
        REQUIRE_EQUAL(options.maximumLineWraps(), 5);
        requireStringEqual(options.paragraphEllipsisMark(), U"..."_el);
        const auto tabStopCount = options.tabStops().size();
        REQUIRE_EQUAL(tabStopCount, std::size_t{2});
        REQUIRE_EQUAL(options.tabStops()[0], 2);
        REQUIRE_EQUAL(options.tabStops()[1], 8);
        REQUIRE_EQUAL(options.tabOverflowBehavior(), TabOverflowBehavior::LineBreak);
        REQUIRE_EQUAL(options.onError(), ParagraphOnError::Empty);
    }

    void testWrapperSettersUpdateAndClampParagraphState() {
        auto options = BlockTextOptions{};

        options.setColor(Color{fg::Cyan, bg::Blue});
        options.setAlignment(bgeo::Alignment::BottomCenter);
        options.setLineIndent(-4);
        options.setFirstLineIndent(ParagraphOptions::cUseLineIndent);
        options.setWrappedLineIndent(-7);
        options.setMargins(bgeo::BlockMargins{2});
        options.setMaximumLineWraps(-2);
        options.setLineBreakEndMark(BlockStringEditor{"!"_el});
        options.setLineBreakStartMark(BlockStringEditor{"?"_el});
        options.setParagraphEllipsisMark(BlockStringEditor{"(more)"_el});
        options.setTabStops({4});

        const auto colorSequenceLength = options.colorSequence().sequenceLength();
        REQUIRE_EQUAL(colorSequenceLength, std::size_t{1});
        REQUIRE_EQUAL(options.color(), Color(fg::Cyan, bg::Blue));
        REQUIRE_EQUAL(options.alignment(), bgeo::Alignment::BottomCenter);
        REQUIRE_EQUAL(options.lineIndent(), 0);
        REQUIRE_EQUAL(options.firstLineIndent(), 0);
        REQUIRE_EQUAL(options.wrappedLineIndent(), 0);
        REQUIRE_EQUAL(options.margins(), bgeo::BlockMargins(2));
        REQUIRE_EQUAL(options.maximumLineWraps(), 0);
        requireStringEqual(options.lineBreakEndMark(), U"!"_el);
        requireStringEqual(options.lineBreakStartMark(), U"?"_el);
        requireStringEqual(options.paragraphEllipsisMark(), U"(more)"_el);
        const auto tabStopCount = options.tabStops().size();
        REQUIRE_EQUAL(tabStopCount, std::size_t{1});
        REQUIRE_EQUAL(options.tabStops()[0], 4);
    }

private:
    void requireStringEqual(const BlockString &actual, const erbsland::text::U32String &expected) {
        const auto actualLength = actual.length().toSizeT();
        const auto expectedLength = expected.length().toSizeT();
        REQUIRE_EQUAL(actualLength, expectedLength);
        for (std::size_t i = 0; i < expected.length().toSizeT(); ++i) {
            REQUIRE_EQUAL(actual[BlockIndex::fromSizeT(i)], expected[erbsland::unit::CpIndex::fromSizeT(i)]);
        }
    }
};
