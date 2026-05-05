// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/bgeo/all.hpp>
#include <erbsland/cterm/Buffer.hpp>
#include <erbsland/cterm/theme/ThemeAccessor.hpp>
#include <erbsland/cterm/theme/ThemeBuilder.hpp>
#include <erbsland/cterm/theme/ThemePainter.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <stdexcept>
#include <type_traits>

using namespace erbsland::cterm;
using namespace erbsland::text::literals;
namespace bgeo = erbsland::bgeo;

TESTED_TARGETS(Theme ThemeAccessor ThemeBuilder ThemePainter Identifier Tags)
class ThemeTest final : public el::UnitTest {
public:
    void testIdentifiersAreStrongTypedAndHierarchical() {
        static_assert(!std::is_same_v<theme::Element::Id, theme::Part::Id>);
        static_assert(!std::is_constructible_v<theme::Element, theme::Part::Id>);
        static_assert(!std::is_constructible_v<theme::Part, theme::Element::Id>);

        REQUIRE_FALSE(theme::Element{}.isValid());
        REQUIRE_FALSE(theme::Part{}.isValid());
        REQUIRE_FALSE(theme::Tag{}.isValid());
        REQUIRE_EQUAL(theme::Element::Base.value(), 1);
        REQUIRE_EQUAL(theme::Part::Background.value(), 1);
        REQUIRE_EQUAL(theme::Element::custom(1).value(), 1001);
        REQUIRE_EQUAL(theme::Part::custom(25).value(), 1025);
        REQUIRE_THROWS_AS(std::invalid_argument, theme::Element::custom(0));
        REQUIRE_THROWS_AS(std::invalid_argument, theme::Part::custom(-1));
    }

    void testCustomElementsCanRegisterBaseElement() {
        const auto customPanel = theme::Element::custom(91);
        theme::Theme::registerElement(customPanel, theme::Element::Panel);

        auto builder = theme::ThemeBuilder{};
        builder.edit(theme::Selector{theme::Element::Panel, theme::Part::Border})
            .setColor(Color{fg::Green})
            .setBlocks(U'P');
        const auto activeTheme = builder.build();

        const auto accessor = theme::ThemeAccessor{activeTheme, theme::Selector{customPanel, theme::Part::Border}};

        REQUIRE_EQUAL(accessor.color().fg(), fg::Green);
        REQUIRE_EQUAL(accessor.block().singleCodePoint(), U'P');
        REQUIRE_THROWS_AS(std::logic_error, theme::Theme::registerElement(customPanel, theme::Element::Layout));
        REQUIRE_THROWS_AS(std::logic_error, theme::Theme::registerElement(theme::Element::Panel, theme::Element::Base));
        REQUIRE_THROWS_AS(std::invalid_argument, theme::Theme::registerElement(theme::Element::custom(92), {}));
    }

    void testTagsUseBuilderAssignedBits() {
        auto builder = theme::ThemeBuilder{};
        const auto warning = builder.registerTag();
        const auto error = builder.registerTag();
        auto tags = theme::Tags{warning};

        tags.add(error);

        REQUIRE(warning.isValid());
        REQUIRE(error.isValid());
        REQUIRE_NOT_EQUAL(warning, error);
        REQUIRE(tags.contains(warning));
        REQUIRE(tags.contains(error));
        REQUIRE(tags.containsAll(theme::Tags{warning, error}));
        tags.remove(warning);
        REQUIRE_FALSE(tags.contains(warning));
    }

    void testRegisterTagThrowsAfterSixtyFourTags() {
        auto builder = theme::ThemeBuilder{};

        for (auto index = 0; index < 64; ++index) {
            REQUIRE(builder.registerTag().isValid());
        }

        REQUIRE_THROWS_AS(std::logic_error, builder.registerTag());
    }

    void testElementAndPartFallbackResolveEffectiveValues() {
        auto builder = theme::ThemeBuilder{};
        builder.edit(theme::Selector{theme::Element::Base}).setStyle(BlockStyle::reset());
        builder.edit(theme::Selector{theme::Element::Surface, theme::Part::Border})
            .setColor(Color{fg::Green, bg::Blue})
            .setBlocks(U'#');
        const auto activeTheme = builder.build();

        REQUIRE(activeTheme != nullptr);
        const auto border =
            theme::ThemeAccessor{activeTheme, theme::Selector{theme::Element::Button, theme::Part::Border}};

        REQUIRE_EQUAL(border.color(), Color(fg::Green, bg::Blue));
        REQUIRE_EQUAL(border.block().singleCodePoint(), U'#');
    }

    void testMissingThemePropertiesStayTransparent() {
        const auto activeTheme = theme::ThemeBuilder{}.build();
        const auto accessor =
            theme::ThemeAccessor{activeTheme, theme::Selector{theme::Element::Button, theme::Part::Text}};

        REQUIRE_EQUAL(accessor.color(), Color{});
        REQUIRE_EQUAL(accessor.attributes(), BlockAttributes{});
        REQUIRE_EQUAL(accessor.block().color(), Color{});
        REQUIRE_EQUAL(accessor.block().attributes(), BlockAttributes{});
    }

    void testStateSpecificityUsesBestPartialStateBase() {
        auto builder = theme::ThemeBuilder{};
        builder.edit(theme::Selector{theme::Element::Surface, theme::Part::Text}).setColor(Color{fg::Green});
        builder.edit(theme::Selector{theme::Element::Surface, theme::Part::Text}.requireState(theme::State::Disabled))
            .setColor(Color{fg::Yellow});
        const auto activeTheme = builder.build();

        const auto selector = theme::Selector{theme::Element::Surface, theme::Part::Text}.withStates(
            theme::States{theme::State::Focused} | theme::States{theme::State::Disabled});
        const auto accessor = theme::ThemeAccessor{activeTheme, selector};

        REQUIRE_EQUAL(accessor.color().fg(), fg::Yellow);
    }

    void testTagSpecificityUsesBestPartialTagBase() {
        auto builder = theme::ThemeBuilder{};
        const auto warning = builder.registerTag();
        const auto important = builder.registerTag();
        builder.edit(theme::Selector{theme::Element::Surface, theme::Part::Text}).setColor(Color{fg::Green});
        builder.edit(theme::Selector{theme::Element::Surface, theme::Part::Text}.requireTag(warning))
            .setColor(Color{fg::Yellow});
        const auto activeTheme = builder.build();

        const auto selector =
            theme::Selector{theme::Element::Surface, theme::Part::Text}.withTags(theme::Tags{warning, important});
        const auto accessor = theme::ThemeAccessor{activeTheme, selector};

        REQUIRE_EQUAL(accessor.color().fg(), fg::Yellow);
    }

    void testPropertiesResolveBlocksMarginsPaddingAndAttributes() {
        auto builder = theme::ThemeBuilder{};
        builder.edit(theme::Selector{theme::Element::Surface, theme::Part::Background})
            .setColor(Color{fg::Red, bg::Blue})
            .setColorSequence(ColorSequence{Color{fg::Yellow, bg::Blue}, Color{fg::Blue, bg::Blue}})
            .setAttributes(BlockAttributes::Bold)
            .setBlocks(U'X')
            .setMargins(bgeo::BlockMargins{1})
            .setPadding(bgeo::BlockMargins{2});
        builder.edit(theme::Selector{theme::Element::Panel, theme::Part::Background})
            .setColor(Color{fg::Inherited, bg::Green})
            .setAttributes(BlockAttributes{});
        const auto activeTheme = builder.build();

        const auto accessor =
            theme::ThemeAccessor{activeTheme, theme::Selector{theme::Element::Panel, theme::Part::Background}};

        REQUIRE_EQUAL(accessor.color(), Color(fg::Inherited, bg::Green));
        REQUIRE(accessor.colorSequence().empty());
        REQUIRE_FALSE(accessor.attributes().isEnabled(BlockAttributes::Bold));
        REQUIRE_FALSE(accessor.attributes().isSpecified(BlockAttributes::Bold));
        REQUIRE_EQUAL(accessor.block().singleCodePoint(), U'X');
        REQUIRE_EQUAL(accessor.margins(), bgeo::BlockMargins{1});
        REQUIRE_EQUAL(accessor.padding(), bgeo::BlockMargins{2});
    }

    void testPartialBlockDefinitionsKeepInheritedRoles() {
        auto builder = theme::ThemeBuilder{};
        builder.edit(theme::Selector{theme::Element::Surface, theme::Part::Border}).setBlocks(U'.');
        builder.edit(theme::Selector{theme::Element::Panel, theme::Part::Border})
            .setBlock(theme::BlockRole::LeftBracket, U'[');
        const auto activeTheme = builder.build();

        const auto accessor =
            theme::ThemeAccessor{activeTheme, theme::Selector{theme::Element::Panel, theme::Part::Border}};

        REQUIRE_EQUAL(accessor.block(theme::BlockRole::LeftBracket).singleCodePoint(), U'[');
        REQUIRE_EQUAL(accessor.block(theme::BlockRole::RightBracket).singleCodePoint(), U'.');
        REQUIRE_EQUAL(accessor.block(theme::BlockRole::LeftOuterPadding).singleCodePoint(), U'.');
    }

    void testThemeAccessorReturnsHorizontalSpacing() {
        auto builder = theme::ThemeBuilder{};
        builder.edit(theme::Selector{theme::Element::Surface, theme::Part::Text})
            .setMargins(bgeo::BlockMargins{1, 2, 3, 4})
            .setPadding(bgeo::BlockMargins{5, 6, 7, 8});
        const auto activeTheme = builder.build();
        const auto accessor =
            theme::ThemeAccessor{activeTheme, theme::Selector{theme::Element::Surface, theme::Part::Text}};

        REQUIRE_EQUAL(accessor.horizontalMargins(), (bgeo::BlockMargins{0, 2, 0, 4}));
        REQUIRE_EQUAL(accessor.horizontalPadding(), (bgeo::BlockMargins{0, 6, 0, 8}));
        REQUIRE_EQUAL(accessor.extent(), (bgeo::BlockSize{20, 16}));
        REQUIRE_EQUAL(accessor.horizontalExtent(), (bgeo::BlockSize{20, 0}));
        const auto layout = accessor.layout(bgeo::BlockRectangle{0, 0, 30, 20});
        REQUIRE_EQUAL(layout.outerRect, (bgeo::BlockRectangle{0, 0, 30, 20}));
        REQUIRE_EQUAL(layout.partRect, (bgeo::BlockRectangle{4, 1, 24, 16}));
        REQUIRE_EQUAL(layout.contentRect, (bgeo::BlockRectangle{12, 6, 10, 4}));
        const auto horizontalLayout = accessor.horizontalLayout(bgeo::BlockRectangle{0, 0, 30, 20});
        REQUIRE_EQUAL(horizontalLayout.outerRect, (bgeo::BlockRectangle{0, 0, 30, 20}));
        REQUIRE_EQUAL(horizontalLayout.partRect, (bgeo::BlockRectangle{4, 0, 24, 20}));
        REQUIRE_EQUAL(horizontalLayout.contentRect, (bgeo::BlockRectangle{12, 0, 10, 20}));
    }

    void testColorSequenceOverridesStaticColorForAnimation() {
        auto builder = theme::ThemeBuilder{};
        builder.edit(theme::Selector{theme::Element::Surface, theme::Part::Text})
            .setColorSequence(ColorSequence{Color{fg::Red}, Color{fg::Blue}});
        const auto activeTheme = builder.build();
        const auto accessor =
            theme::ThemeAccessor{activeTheme, theme::Selector{theme::Element::Surface, theme::Part::Text}};

        REQUIRE_EQUAL(accessor.color(0).fg(), fg::Red);
        REQUIRE_EQUAL(accessor.color(1).fg(), fg::Blue);
        REQUIRE_EQUAL(accessor.color(2).fg(), fg::Red);
    }

    void testThemePainterFillsFromBlockRoles() {
        auto builder = theme::ThemeBuilder{};
        builder.edit(theme::Selector{theme::Element::Panel, theme::Part::Background})
            .setColor(Color{fg::Red, bg::Blue})
            .setBlocks(U"abcdefghi0123456"_el);
        const auto activeTheme = builder.build();
        auto buffer = Buffer{bgeo::BlockSize{3, 3}};
        const auto accessor =
            theme::ThemeAccessor{activeTheme, theme::Selector{theme::Element::Panel, theme::Part::Background}};

        theme::ThemePainter{buffer, accessor}.fill(buffer.rect());

        REQUIRE_EQUAL(
            buffer.get(bgeo::BlockPosition{0, 0}), (Block{U'a', Color{fg::Red, bg::Blue}, BlockAttributes{}}));
        REQUIRE_EQUAL(
            buffer.get(bgeo::BlockPosition{1, 1}), (Block{U'e', Color{fg::Red, bg::Blue}, BlockAttributes{}}));
        REQUIRE_EQUAL(
            buffer.get(bgeo::BlockPosition{2, 2}), (Block{U'i', Color{fg::Red, bg::Blue}, BlockAttributes{}}));
    }
};
