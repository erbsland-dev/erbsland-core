// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/text/EncodingError.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <functional>

TESTED_TARGETS(Block)
class BlockTest final : public UNITTEST_SUBCLASS(TestHelper) {
public:
    void testDisplayWidthUsesUnicodeCellWidth() {
        const auto combiningText = bytes({0x65, 0xCC, 0x81});
        const auto asciiChar = Block{"A"_el, Color{}};
        const auto wideChar = Block{U'界', Color{}};
        const auto combiningChar = Block{erbsland::text::StringEditor{combiningText}, Color{}};

        REQUIRE_EQUAL(asciiChar.displayWidth(), 1);
        REQUIRE_EQUAL(wideChar.displayWidth(), 2);
        REQUIRE_EQUAL(combiningChar.displayWidth(), 1);
    }

    void testConstructorsDecodeUtf8Utf32AndCodePoints() {
        const auto fromUtf8 = Block{erbsland::text::StringEditor{bytes({0x65, 0xCC, 0x81})}};
        const auto fromUtf32Text = Block{U"e\u0301"_el};
        constexpr auto fromCodePoint = Block{U'★'};

        REQUIRE_EQUAL(fromUtf8.characterCount(), erbsland::unit::CpLength{2});
        REQUIRE_EQUAL(fromUtf32Text.characterCount(), erbsland::unit::CpLength{2});
        REQUIRE_EQUAL(fromCodePoint.characterCount(), erbsland::unit::CpLength{1});

        REQUIRE_EQUAL(fromUtf8.characters(), (std::array<erbsland::text::Char, 3>{U'e', U'\u0301', 0}));
        REQUIRE_EQUAL(fromUtf32Text.characters(), (std::array<erbsland::text::Char, 3>{U'e', U'\u0301', 0}));
        REQUIRE_EQUAL(fromCodePoint.characters(), (std::array<erbsland::text::Char, 3>{U'★', 0, 0}));
        REQUIRE_EQUAL(fromUtf8.first(), U'e');
        REQUIRE_EQUAL(fromCodePoint.first(), U'★');
    }

    void testBraceInitializedDefaultStyleIsUnambiguous() {
        static_assert(requires { Block{U'X', {}}; });
        static_assert(requires { Block{"X"_el, {}}; });
        static_assert(requires { Block{U"X"_el, {}}; });

        const auto fromCodePoint = Block{U'X', {}};
        const auto fromUtf8 = Block{"X"_el, {}};
        const auto fromUtf32 = Block{U"X"_el, {}};

        REQUIRE_EQUAL(fromCodePoint.style(), BlockStyle{});
        REQUIRE_EQUAL(fromUtf8.style(), BlockStyle{});
        REQUIRE_EQUAL(fromUtf32.style(), BlockStyle{});
    }

    void testToStringAndByteCountEncodeUtf8() {
        const auto character = Block{U"e\u0301"_el, fg::BrightWhite, bg::Blue};
        auto buffer = std::string{"prefix:"};

        buffer += blockToStdString(character);

        REQUIRE_EQUAL(character.byteCount(), erbsland::unit::ByteLength{3});
        REQUIRE_EQUAL(character.toString(), erbsland::text::StringEditor{bytes({0x65, 0xCC, 0x81})});
        REQUIRE_EQUAL(buffer, std::string{"prefix:"} + bytes({0x65, 0xCC, 0x81}));
    }

    void testWithCombiningAppendsZeroWidthCodePoints() {
        const auto combined = Block{U'e'}.withCombining(U'\u0301');

        REQUIRE_EQUAL(combined.characterCount(), erbsland::unit::CpLength{2});
        REQUIRE_EQUAL(combined.characters(), (std::array<erbsland::text::Char, 3>{U'e', U'\u0301', 0}));
        REQUIRE_EQUAL(combined.toString(), erbsland::text::StringEditor{bytes({0x65, 0xCC, 0x81})});
    }

    void testWithCombiningThrowRejectsInvalidCodePoints() {
        REQUIRE_THROWS_AS(
            erbsland::text::EncodingError, Block{}.withCombining(U'\u0301', erbsland::text::EncodingErrorMode::Throw));
        REQUIRE_THROWS_AS(
            erbsland::text::EncodingError, Block{U'e'}.withCombining(U'x', erbsland::text::EncodingErrorMode::Throw));
        REQUIRE_THROWS_AS(
            erbsland::text::EncodingError, Block{U'e'}.withCombining(U'\n', erbsland::text::EncodingErrorMode::Throw));
        REQUIRE_THROWS_AS(
            erbsland::text::EncodingError,
            Block{U"e\u0301\u0302"_el}.withCombining(U'\u0303', erbsland::text::EncodingErrorMode::Throw));
    }

    void testWithCombiningReplaceLeavesInvalidAdditionsUnchanged() {
        const auto base = Block{U"e\u0301\u0302"_el};

        REQUIRE_EQUAL(Block{}.withCombining(U'\u0301'), Block{});
        REQUIRE_EQUAL(Block{U'e'}.withCombining(U'x'), Block{U'e'});
        REQUIRE_EQUAL(Block{U'e'}.withCombining(U'\n'), Block{U'e'});
        REQUIRE_EQUAL(base.withCombining(U'\u0303'), base);
        REQUIRE_EQUAL(base.withCombining(U'\u0303', erbsland::text::EncodingErrorMode::Ignore), base);
    }

    void testEqualityComparesCodePointsAndColors() {
        auto attributes = BlockAttributes{};
        attributes.setBold(true);
        const auto left = Block{U'★', Color{fg::Yellow, bg::Blue}, attributes};
        const auto equal = Block{U'★', Color{fg::Yellow, bg::Blue}, attributes};
        const auto differentColor = Block{U'★', fg::Yellow, bg::Black};
        auto differentAttributes = attributes;
        differentAttributes.setUnderline(true);
        const auto differentAttributeChar = Block{U'★', Color{fg::Yellow, bg::Blue}, differentAttributes};
        const auto differentCodePoint = Block{U'☆', fg::Yellow, bg::Blue};

        REQUIRE(left == equal);
        REQUIRE_FALSE(left != equal);
        REQUIRE_FALSE(left == differentColor);
        REQUIRE_FALSE(left == differentAttributeChar);
        REQUIRE_FALSE(left == differentCodePoint);
        REQUIRE(differentColor != left);
        REQUIRE(differentCodePoint != left);
    }

    void testEqualityIgnoresTheDisplayWidthCache() {
        auto attributes = BlockAttributes{};
        attributes.setItalic(true);
        auto left = Block{U'界', Color{fg::Yellow, bg::Blue}, attributes};
        auto right = Block{U'界', Color{fg::Yellow, bg::Blue}, attributes};

        REQUIRE_EQUAL(left.displayWidth(), 2);
        REQUIRE(left == right);

        REQUIRE_EQUAL(right.displayWidth(), 2);
        REQUIRE(left == right);
    }

    void testSingleCodePointComparisonsIgnoreColorButRejectMultiCodePointCharacters() {
        const auto colored = Block{U'★', fg::Yellow, bg::Blue};
        const auto combined = Block{U"e\u0301"_el};

        REQUIRE(colored == U'★');
        REQUIRE_FALSE(colored != U'★');
        REQUIRE(colored != U'☆');
        REQUIRE_FALSE(combined == U'e');
        REQUIRE(combined != U'e');
    }

    void testWithOverlayPreservesInheritedComponents() {
        const auto base = Block{U'X', fg::Green, bg::Blue};

        const auto changedForeground = base.withOverlay(BlockStyle{Color{fg::BrightWhite, bg::Inherited}});
        REQUIRE_EQUAL(changedForeground.color(), Color(fg::BrightWhite, bg::Blue));

        const auto changedBackground = base.withOverlay(BlockStyle{Color{fg::Inherited, bg::Black}});
        REQUIRE_EQUAL(changedBackground.color(), Color(fg::Green, bg::Black));
    }

    void testWithOverlayAppliesDefaultAsExplicitReset() {
        const auto base = Block{U'X', fg::Green, bg::Blue};

        const auto resetForeground = base.withOverlay(BlockStyle{Color{fg::Default, bg::Inherited}});
        REQUIRE_EQUAL(resetForeground.color(), Color(fg::Default, bg::Blue));

        const auto resetBackground = base.withOverlay(BlockStyle{Color{fg::Inherited, bg::Default}});
        REQUIRE_EQUAL(resetBackground.color(), Color(fg::Green, bg::Default));
    }

    void testWithColorReplacedAndWithBaseUseExpectedRules() {
        const auto base = Block{U'X', fg::Green, bg::Blue};
        const auto overlaid =
            Block{U'X', fg::Inherited, bg::Yellow}.withBase(BlockStyle{Color{fg::BrightWhite, bg::Blue}});

        REQUIRE_EQUAL(
            base.withColorReplaced(Color{fg::BrightWhite, bg::Black}).color(), Color(fg::BrightWhite, bg::Black));
        REQUIRE_EQUAL(
            base.withBase(BlockStyle{Color{fg::Inherited, bg::BrightBlack}}).color(), Color(fg::Green, bg::Blue));
        REQUIRE_EQUAL(overlaid.color(), Color(fg::BrightWhite, bg::Yellow));
    }

    void testConstructorsAndWithAttributesPreserveCharacterAttributes() {
        auto attributes = BlockAttributes{};
        attributes.setBold(true);
        attributes.setUnderline(true);

        const auto coloredCharacter = Block{U'X', Color{fg::Green, bg::Blue}, attributes};
        REQUIRE(coloredCharacter.attributes().isBold());
        REQUIRE(coloredCharacter.attributes().isUnderline());

        auto replacedAttributes = BlockAttributes::reset();
        replacedAttributes.setItalic(true);
        const auto updatedCharacter = coloredCharacter.withAttributes(replacedAttributes);
        REQUIRE_FALSE(updatedCharacter.attributes().isBold());
        REQUIRE(updatedCharacter.attributes().isItalic());
    }

    void testStyleConstructorAccessorAndSetterExposeTheCombinedStyle() {
        auto attributes = BlockAttributes{};
        attributes.setItalic(true);
        auto style = BlockStyle{Color{fg::Green, bg::Blue}, attributes};

        auto character = Block{U'X', style};

        REQUIRE_EQUAL(character.style(), style);
        REQUIRE_EQUAL(character.color(), Color(fg::Green, bg::Blue));
        REQUIRE(character.attributes().isItalic());

        auto replacementAttributes = BlockAttributes{};
        replacementAttributes.setBold(true);
        style = BlockStyle{Color{fg::BrightWhite, bg::Black}, replacementAttributes};
        character.setStyle(style);

        REQUIRE_EQUAL(character.style(), style);
        REQUIRE(character.attributes().isBold());
        REQUIRE_FALSE(character.attributes().isItalic());
    }

    void testWithOverlayAndWithBaseResolveColorAndAttributesTogether() {
        auto currentAttributes = BlockAttributes{};
        currentAttributes.setBold(true);
        auto overlayAttributes = BlockAttributes{};
        overlayAttributes.setBold(false);
        overlayAttributes.setUnderline(true);
        auto baseAttributes = BlockAttributes{};
        baseAttributes.setItalic(true);
        const auto character = Block{U'X', Color{fg::Green, bg::Blue}, currentAttributes};

        const auto overlaid = character.withOverlay(BlockStyle{Color{fg::Inherited, bg::Black}, overlayAttributes});
        REQUIRE_EQUAL(overlaid.color(), Color(fg::Green, bg::Black));
        REQUIRE_FALSE(overlaid.attributes().isBold());
        REQUIRE(overlaid.attributes().isUnderline());

        const auto based = character.withBase(BlockStyle{Color{fg::BrightWhite, bg::Default}, baseAttributes});
        REQUIRE_EQUAL(based.color(), Color(fg::Green, bg::Blue));
        REQUIRE(based.attributes().isBold());
        REQUIRE(based.attributes().isItalic());

        const auto fromBaseCharacter = Block{U'Y', Color{fg::BrightWhite, bg::Default}, baseAttributes};
        REQUIRE_EQUAL(character.withBase(fromBaseCharacter), based);
    }

    void testSpacingRecognizesSingleWhitespaceCodePoints() {
        REQUIRE(Block{U' '}.isSpacing());
        REQUIRE(Block{U'\t'}.isSpacing());
        REQUIRE_FALSE(Block{U"e\u0301"_el}.isSpacing());
        REQUIRE_FALSE(Block{}.isSpacing());
    }

    void testIsEmptySpaceAndSingleCodePointComparisons() {
        const auto empty = Block{};
        const auto space = Block::space();
        const auto symbol = Block{U'X', fg::Red, bg::Black};
        const auto combined = Block{U"e\u0301"_el};

        REQUIRE(empty.isEmpty());
        REQUIRE_FALSE(space.isEmpty());
        REQUIRE(space == U' ');
        REQUIRE_EQUAL(space.color(), Color{});
        REQUIRE(symbol == U'X');
        REQUIRE_FALSE(symbol == U'A');
        REQUIRE_FALSE(combined == U'e');
    }

    void testEmptyBlockCreatesAnEmptyCharacterWithTheRequestedStyle() {
        auto attributes = BlockAttributes{};
        attributes.setUnderline(true);
        const auto style = BlockStyle{Color{fg::BrightWhite, bg::Blue}, attributes};

        const auto emptyBlock = Block::emptyBlock(style);

        REQUIRE(emptyBlock.isEmpty());
        REQUIRE_EQUAL(emptyBlock.style(), style);
        REQUIRE_FALSE(emptyBlock.isSpacing());
    }

    void testRenderedEqualsTreatsInheritedColorsLikeDefaults() {
        const auto inherited = Block{U'X', fg::Inherited, bg::Inherited};
        const auto defaults = Block{U'X', fg::Default, bg::Default};

        REQUIRE(inherited.renderedEquals(defaults));
        REQUIRE(defaults.renderedEquals(inherited));
    }

    void testRenderedEqualsCanIgnoreColorDifferences() {
        const auto left = Block{U'X', fg::Red, bg::Black};
        const auto right = Block{U'X', fg::Blue, bg::Yellow};

        REQUIRE_FALSE(left.renderedEquals(right));
        REQUIRE(left.renderedEquals(right, false));
    }

    void testRenderedEqualsTreatsInheritedAttributesLikeDisabled() {
        auto disabledAttributes = BlockAttributes::reset();
        disabledAttributes.setBold(false);
        disabledAttributes.setUnderline(false);
        const auto inherited = Block{U'X', Color{}, BlockAttributes{}};
        const auto disabled = Block{U'X', Color{}, disabledAttributes};

        REQUIRE(inherited.renderedEquals(disabled));
        REQUIRE(disabled.renderedEquals(inherited));

        auto boldAttributes = BlockAttributes{};
        boldAttributes.setBold(true);
        const auto bold = Block{U'X', Color{}, boldAttributes};
        REQUIRE_FALSE(inherited.renderedEquals(bold));
        REQUIRE(inherited.renderedEquals(bold, true, false));
    }

    void testTextConstructorsNormalizeUnsupportedTextDeterministically() {
        const auto emptyUtf8 = Block{""_el};
        const auto emptyUtf32 = Block{U""_el};
        const auto leadingCombining = Block{U"\u0301"_el};
        const auto laterVisibleCharacter = Block{U"ab\u0301\u0302"_el};
        const auto thirdCombiningMark =
            Block{erbsland::text::StringEditor{bytes({0x61, 0xCC, 0x81, 0xCC, 0x82, 0xCC, 0x83})}};
        const auto controlCode = Block{"\n"_el};

        REQUIRE_EQUAL(emptyUtf8.first(), U'\uFFFD');
        REQUIRE_EQUAL(emptyUtf32.first(), U'\uFFFD');
        REQUIRE_EQUAL(leadingCombining.first(), U'\uFFFD');
        REQUIRE_EQUAL(laterVisibleCharacter.first(), U'\uFFFD');
        REQUIRE_EQUAL(controlCode.first(), U'\uFFFD');
        REQUIRE_EQUAL(
            thirdCombiningMark.characters(), (std::array<erbsland::text::Char, 3>{U'a', U'\u0301', U'\u0302'}));
    }

    void testUtf8ConstructorsReplaceEncodingErrorMode() {
        const auto character = Block{erbsland::text::StringEditor{bytes({0xC3})}};

        REQUIRE_EQUAL(character.characterCount(), erbsland::unit::CpLength{1});
        REQUIRE_EQUAL(character.first(), U'\uFFFD');
        REQUIRE_EQUAL(character.toString(), erbsland::text::StringEditor{bytes({0xEF, 0xBF, 0xBD})});
    }

    void testTextConstructorsPreserveStyleAfterNormalization() {
        constexpr auto invalidUtf32 = std::array<char32_t, 1>{0xD800U};

        const auto style = BlockStyle{Color{fg::Yellow, bg::Blue}, BlockAttributes::reset()};
        const auto invalidUtf8 = bytes({0xC3, 0x42});
        const auto character = Block{erbsland::text::StringEditor{std::string_view{invalidUtf8}}, style};
        const auto utf32Character = Block{
            erbsland::text::U32StringEditor{std::u32string_view{invalidUtf32.data(), invalidUtf32.size()}}, style};

        REQUIRE_EQUAL(character.first(), U'\uFFFD');
        REQUIRE_EQUAL(character.style(), style);
        REQUIRE_EQUAL(utf32Character.first(), U'\uFFFD');
        REQUIRE_EQUAL(utf32Character.style(), style);
    }

    void testHashMatchesStdHashAndReflectsTextAndColor() {
        auto attributes = BlockAttributes{};
        attributes.setStrikethrough(true);
        const auto base = Block{U'★', Color{fg::Yellow, bg::Blue}, attributes};
        const auto differentText = Block{U'☆', fg::Yellow, bg::Blue};
        const auto differentColor = Block{U'★', fg::Yellow, bg::Black};
        auto differentAttributes = attributes;
        differentAttributes.setStrikethrough(false);
        const auto differentAttributeChar = Block{U'★', Color{fg::Yellow, bg::Blue}, differentAttributes};

        REQUIRE_EQUAL(base.hash(), std::hash<Block>{}(base));
        REQUIRE_NOT_EQUAL(base.hash(), differentText.hash());
        REQUIRE_NOT_EQUAL(base.hash(), differentColor.hash());
        REQUIRE_NOT_EQUAL(base.hash(), differentAttributeChar.hash());
    }
};
