// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(BlockText)
class BlockTextTest final : public el::UnitTest {
public:
    void testDefaultConstructionUsesEmptyTextAndDefaultOptions() {
        const auto text = BlockText{};
        const auto expectedRect = bgeo::BlockRectangle{0, 0, 0, 0};

        REQUIRE(text.blockString().isEmpty());
        REQUIRE_EQUAL(text.rectangle(), expectedRect);
        REQUIRE_EQUAL(text.color(), Color{});
        REQUIRE_EQUAL(text.font(), nullptr);
        REQUIRE_EQUAL(text.animation(), BlockTextAnimation::None);
        REQUIRE_EQUAL(text.alignment(), bgeo::Alignment::TopLeft);
        REQUIRE_EQUAL(text.paragraphSpacing(), ParagraphSpacing::SingleLine);
    }

    void testTextUsesSingleParagraphSpacingByDefault() {
        const auto text = BlockText{};

        REQUIRE_EQUAL(text.paragraphSpacing(), ParagraphSpacing::SingleLine);
    }

    void testConstructorStoresTextRectangleAndAlignment() {
        const auto text =
            BlockText{BlockStringEditor{"Hello"_el}, bgeo::BlockRectangle{1, 2, 7, 3}, bgeo::Alignment::CenterRight};

        requireStringEqual(text.blockString(), U"Hello"_el);
        REQUIRE_EQUAL(text.rectangle(), (bgeo::BlockRectangle{1, 2, 7, 3}));
        REQUIRE_EQUAL(text.alignment(), bgeo::Alignment::CenterRight);
    }

    void testTextAndTextOptionSettersReplaceStoredConfiguration() {
        auto text = BlockText{};
        auto options = BlockTextOptions{bgeo::Alignment::BottomLeft};
        const auto font = std::make_shared<Font>(2);

        options.setColor(Color{fg::Yellow, bg::Blue});
        options.setFont(font);
        options.setAnimation(BlockTextAnimation::ColorDiagonal);
        options.setLineIndent(3);
        options.setWrappedLineIndent(5);
        options.setMargins(bgeo::BlockMargins{2, 1});
        options.setParagraphSpacing(ParagraphSpacing::DoubleLine);
        options.setTabStops({6});

        text.setBlockString(BlockStringEditor{"ABC"_el});
        text.setRectangle(bgeo::BlockRectangle{4, 5, 6, 7});
        text.setBlockTextOptions(options);

        requireStringEqual(text.blockString(), U"ABC"_el);
        REQUIRE_EQUAL(text.rectangle(), (bgeo::BlockRectangle{4, 5, 6, 7}));
        REQUIRE_EQUAL(text.color(), Color(fg::Yellow, bg::Blue));
        REQUIRE_EQUAL(text.font(), font);
        REQUIRE_EQUAL(text.animation(), BlockTextAnimation::ColorDiagonal);
        REQUIRE_EQUAL(text.alignment(), bgeo::Alignment::BottomLeft);
        REQUIRE_EQUAL(text.lineIndent(), 3);
        REQUIRE_EQUAL(text.wrappedLineIndent(), 5);
        REQUIRE_EQUAL(text.margins(), bgeo::BlockMargins(2, 1));
        REQUIRE_EQUAL(text.paragraphSpacing(), ParagraphSpacing::DoubleLine);
        REQUIRE_EQUAL(text.tabStops().size(), std::size_t{1});
        REQUIRE_EQUAL(text.tabStops()[0], 6);
    }

    void testWrapperSettersUpdateTheEmbeddedTextOptions() {
        auto text = BlockText{};

        text.setColorSequence(ColorSequence{Color{fg::Red, bg::Black}, Color{fg::Green, bg::Black}});
        text.setColor(Color{fg::Magenta, bg::Cyan});
        text.setAnimation(BlockTextAnimation::ColorDiagonal);
        text.setAlignment(bgeo::Alignment::BottomCenter);
        text.setMargins(bgeo::BlockMargins{1});
        text.setWordSeparators(U",;"_el);
        text.setWordBreakMark(Block{U'='});
        text.setMaximumLineWraps(-1);
        text.setOnError(ParagraphOnError::Empty);

        REQUIRE_EQUAL(text.colorSequence().sequenceLength(), std::size_t{1});
        REQUIRE_EQUAL(text.color(), Color(fg::Magenta, bg::Cyan));
        REQUIRE_EQUAL(text.animation(), BlockTextAnimation::ColorDiagonal);
        REQUIRE_EQUAL(text.alignment(), bgeo::Alignment::BottomCenter);
        REQUIRE_EQUAL(text.margins(), bgeo::BlockMargins(1));
        REQUIRE_EQUAL(text.wordSeparators(), U",;"_el);
        REQUIRE_EQUAL(text.wordBreakMark(), U'=');
        REQUIRE_EQUAL(text.maximumLineWraps(), 0);
        REQUIRE_EQUAL(text.onError(), ParagraphOnError::Empty);
    }

    void testSetColorCreatesSingleEntryColorSequence() {
        auto text = BlockText{BlockStringEditor{"A"_el}, bgeo::BlockRectangle{0, 0, 1, 1}, bgeo::Alignment::TopLeft};

        REQUIRE_EQUAL(text.color(), Color{});

        text.setColor(Color{fg::Cyan, bg::Black});

        REQUIRE_EQUAL(text.color(), Color(fg::Cyan, bg::Black));
        REQUIRE_EQUAL(text.colorSequence().sequenceLength(), std::size_t{1});
    }

private:
    void requireStringEqual(const BlockString &actual, const erbsland::text::U32String &expected) {
        REQUIRE_EQUAL(actual.length().toSizeT(), expected.length().toSizeT());
        for (std::size_t i = 0; i < expected.length().toSizeT(); ++i) {
            REQUIRE_EQUAL(actual[BlockIndex::fromSizeT(i)], expected[erbsland::unit::CpIndex::fromSizeT(i)]);
        }
    }
};
