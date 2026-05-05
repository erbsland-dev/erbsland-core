// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ThemeBuilder.hpp"

#include "../../text/Literals.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace erbsland::cterm::theme {

using namespace text::literals;

auto ThemeBuilder::registerTag() -> Tag {
    if (_nextTagBit >= 64) {
        throw std::logic_error{"Too many theme tags."};
    }
    const auto result = Tag{_nextTagBit};
    _nextTagBit += 1;
    return result;
}

auto ThemeBuilder::edit(const Selector selector) -> PropertyEditor {
    return PropertyEditor{definitionFor(selector).properties};
}

auto ThemeBuilder::build() const -> ThemeConstPtr {
    return std::make_shared<Theme>(_definitions, _nextTagBit);
}

auto ThemeBuilder::from(const ThemeConstPtr &theme) -> ThemeBuilder {
    auto result = ThemeBuilder{};
    if (theme != nullptr) {
        result._definitions = theme->definitions();
        result._nextTagBit = theme->registeredTagCount();
        for (const auto &[selector, definition] : result._definitions) {
            result._nextOrder = std::max(result._nextOrder, definition.order + 1);
        }
    }
    return result;
}

auto ThemeBuilder::dark() -> ThemeBuilder {
    auto builder = ThemeBuilder{};
    builder.addClassicBlocks();
    builder.edit(Selector{Element::StatusLine, Part::Background}).setColor(fg::White, bg::BrightBlack);
    builder.edit(Selector{Element::HeaderLine, Part::Background}).setColor(fg::White, bg::Blue);
    builder.edit(Selector{Element::FooterLine, Part::Background}).setColor(fg::White, bg::BrightBlack);
    builder.edit(Selector{Element::ActionHelp, Part::ActionName}).setColor(fg::BrightWhite);
    builder.edit(Selector{Element::ActionHelp, Part::Key}).setColor(fg::BrightYellow);
    builder.edit(Selector{Element::ActionHelp, Part::KeyBracket}).setColor(fg::Black);
    builder.edit(Selector{Element::ScrollCorner, Part::Background}).setColor(fg::Black, bg::BrightBlack);
    builder.edit(Selector{Element::HorizontalScrollBar, Part::Track}).setColor(fg::Black, bg::BrightBlack);
    builder.edit(Selector{Element::HorizontalScrollBar, Part::Thumb}).setColor(fg::Black, bg::White);
    builder.edit(Selector{Element::HorizontalScrollBar, Part::Decrease}).setColor(fg::Black, bg::BrightBlack);
    builder.edit(Selector{Element::HorizontalScrollBar, Part::Increase}).setColor(fg::Black, bg::BrightBlack);
    builder.edit(Selector{Element::VerticalScrollBar, Part::Track}).setColor(fg::Black, bg::BrightBlack);
    builder.edit(Selector{Element::VerticalScrollBar, Part::Thumb}).setColor(fg::Black, bg::White);
    builder.edit(Selector{Element::VerticalScrollBar, Part::Decrease}).setColor(fg::Black, bg::BrightBlack);
    builder.edit(Selector{Element::VerticalScrollBar, Part::Increase}).setColor(fg::Black, bg::BrightBlack);
    builder.edit(Selector{Element::Sections, Part::Border}).setColor(fg::BrightBlack);
    builder.edit(Selector{Element::Sections, Part::Text}).setColor(fg::BrightWhite);
    builder.edit(Selector{Element::Sections, Part::Title}).setColor(fg::BrightWhite);
    builder.edit(Selector{Element::Sections, Part::TitleBracket}).setColor(fg::BrightYellow);
    builder.edit(Selector{Element::Sections, Part::Border}.requireState(State::FocusWithin)).setColor(fg::BrightYellow);
    builder.edit(Selector{Element::Button, Part::Border}).setColor(fg::BrightBlue, bg::Blue);
    builder.edit(Selector{Element::Button, Part::Text}).setColor(fg::BrightWhite, bg::Blue);
    builder.edit(Selector{Element::Button, Part::Key}).setColor(fg::BrightYellow, bg::Blue);
    builder.edit(Selector{Element::Button, Part::KeyBracket}).setColor(fg::Black, bg::Blue);
    builder.edit(Selector{Element::Button, Part::Border}.requireState(State::Focused))
        .setColor(fg::BrightYellow, bg::Blue);
    builder.edit(Selector{Element::Button, Part::Text}.requireState(State::Disabled)).setColor(fg::BrightBlack);
    builder.edit(Selector{Element::Frame, Part::Background}).setColor(Color::reset());
    builder.edit(Selector{Element::Frame, Part::Border}).setColor(fg::White);
    builder.edit(Selector{Element::Frame, Part::Text}).setColor(fg::BrightWhite);
    builder.edit(Selector{Element::Frame, Part::Title}).setColor(fg::BrightWhite);
    builder.edit(Selector{Element::Frame, Part::TitleBracket}).setColor(fg::BrightCyan);
    builder.edit(Selector{Element::Choice, Part::Background}).setColor(fg::BrightBlack, bg::Default);
    builder.edit(Selector{Element::Choice, Part::Border}).setColor(fg::BrightCyan);
    builder.edit(Selector{Element::Choice, Part::Text}).setColor(fg::BrightWhite);
    builder.edit(Selector{Element::Choice, Part::Title}).setColor(fg::BrightWhite);
    builder.edit(Selector{Element::Choice, Part::TitleBracket}).setColor(fg::BrightCyan);
    builder.edit(Selector{Element::StaticText, Part::Text}).setColor(fg::BrightWhite);
    builder.edit(Selector{Element::TextBox, Part::Text}).setColor(fg::BrightWhite);
    builder.edit(Selector{Element::HelpViewer, Part::Background}).setColor(fg::BrightWhite, bg::Green);
    return builder;
}

auto ThemeBuilder::light() -> ThemeBuilder {
    auto builder = ThemeBuilder{};
    builder.addClassicBlocks();
    builder.edit(Selector{Element::StatusLine, Part::Background}).setColor(fg::Black, bg::White);
    builder.edit(Selector{Element::HeaderLine, Part::Background}).setColor(fg::White, bg::Blue);
    builder.edit(Selector{Element::FooterLine, Part::Background}).setColor(fg::Black, bg::White);
    builder.edit(Selector{Element::ActionHelp, Part::ActionName}).setColor(fg::Black);
    builder.edit(Selector{Element::ActionHelp, Part::Key}).setColor(fg::Blue);
    builder.edit(Selector{Element::ActionHelp, Part::KeyBracket}).setColor(fg::BrightBlack);
    builder.edit(Selector{Element::ScrollCorner, Part::Background}).setColor(fg::Black, bg::White);
    builder.edit(Selector{Element::HorizontalScrollBar, Part::Track}).setColor(fg::Black, bg::White);
    builder.edit(Selector{Element::HorizontalScrollBar, Part::Thumb}).setColor(fg::White, bg::BrightBlack);
    builder.edit(Selector{Element::VerticalScrollBar, Part::Track}).setColor(fg::Black, bg::White);
    builder.edit(Selector{Element::VerticalScrollBar, Part::Thumb}).setColor(fg::White, bg::BrightBlack);
    builder.edit(Selector{Element::Sections, Part::Border}).setColor(fg::BrightBlack);
    builder.edit(Selector{Element::Sections, Part::Text}).setColor(fg::Black);
    builder.edit(Selector{Element::Sections, Part::Title}).setColor(fg::Black);
    builder.edit(Selector{Element::Sections, Part::TitleBracket}).setColor(fg::Blue);
    builder.edit(Selector{Element::Sections, Part::Border}.requireState(State::FocusWithin)).setColor(fg::Blue);
    builder.edit(Selector{Element::Button, Part::Border}).setColor(fg::BrightBlack);
    builder.edit(Selector{Element::Button, Part::Text}).setColor(fg::Black);
    builder.edit(Selector{Element::Button, Part::Key}).setColor(fg::Blue);
    builder.edit(Selector{Element::Button, Part::KeyBracket}).setColor(fg::BrightBlack);
    builder.edit(Selector{Element::Button, Part::Border}.requireState(State::Focused)).setColor(fg::Blue);
    builder.edit(Selector{Element::Button, Part::Text}.requireState(State::Disabled)).setColor(fg::BrightBlack);
    builder.edit(Selector{Element::Frame, Part::Background}).setColor(Color::reset());
    builder.edit(Selector{Element::Frame, Part::Border}).setColor(fg::Black);
    builder.edit(Selector{Element::Frame, Part::Text}).setColor(fg::Black);
    builder.edit(Selector{Element::Frame, Part::Indicator}).setColor(fg::Blue);
    builder.edit(Selector{Element::Choice, Part::Background}).setColor(Color::reset());
    builder.edit(Selector{Element::Choice, Part::Border}).setColor(fg::Black);
    builder.edit(Selector{Element::Choice, Part::Text}).setColor(fg::Black);
    builder.edit(Selector{Element::Choice, Part::Indicator}).setColor(fg::Blue);
    builder.edit(Selector{Element::StaticText, Part::Text}).setColor(fg::Black);
    return builder;
}

auto ThemeBuilder::monochrome() -> ThemeBuilder {
    auto builder = ThemeBuilder{};
    builder.addPlainBlocks();
    builder.edit(Selector{Element::Base}).setStyle(BlockStyle::reset());
    builder.edit(Selector{Element::HeaderLine, Part::Background}).setAttributes(BlockAttributes::Bold);
    builder.edit(Selector{Element::ActionHelp, Part::Key}).setAttributes(BlockAttributes::Bold);
    builder.edit(Selector{Element::Sections, Part::Title}).setAttributes(BlockAttributes::Bold);
    builder.edit(Selector{Element::Sections, Part::TitleBracket}).setAttributes(BlockAttributes::Bold);
    builder.edit(Selector{Element::Sections, Part::Border}.requireState(State::FocusWithin))
        .setAttributes(BlockAttributes::Bold);
    builder.edit(Selector{Element::Button, Part::Key}).setAttributes(BlockAttributes::Bold);
    builder.edit(Selector{Element::Frame, Part::Indicator}).setAttributes(BlockAttributes::Bold);
    builder.edit(Selector{Element::Choice, Part::Indicator}).setAttributes(BlockAttributes::Bold);
    return builder;
}

auto ThemeBuilder::zero() -> ThemeBuilder {
    return ThemeBuilder{};
}

auto ThemeBuilder::definitionFor(Selector selector) -> Theme::Definition & {
    if (!selector.element().isValid()) {
        selector = selector.withElement(Element::Base);
    }
    const auto [definitionIt, inserted] = _definitions.try_emplace(selector);
    if (inserted) {
        definitionIt->second.order = _nextOrder;
        _nextOrder += 1;
    }
    return definitionIt->second;
}

void ThemeBuilder::addClassicBlocks() {
    edit(Selector{Element::Base, Part::Ellipsis}).setBlock(BlockRole::Main, U'…');
    edit(Selector{Element::Base, Part::KeyBracket}).setBracketBlocks(U'[', U']', U'/');
    edit(Selector{Element::Base, Part::TitleBracket}).setBracketBlocks(U'⟨', U'⟩').setPadding(bgeo::BlockMargins{1, 0});
    edit(Selector{Element::FooterLine, Part::Text}).setMargins(bgeo::BlockMargins{1, 0});
    edit(Selector{Element::ActionHelp, Part::ActionName}).setMargins(bgeo::BlockMargins{0, 2, 0, 1});
    edit(Selector{Element::ActionHelp, Part::Ellipsis}).setBlock(BlockRole::Single, U'…');
    edit(Selector{Element::HorizontalScrollBar, Part::Track})
        .setBlocks(U"░░░░░░░░░←░→░░░←"_el)
        .setMargins(bgeo::BlockMargins{1, 0});
    edit(Selector{Element::HorizontalScrollBar, Part::Thumb}).setBlocks(U"         ▏ ▕    "_el);
    edit(Selector{Element::HorizontalScrollBar, Part::Decrease}).setBlock(BlockRole::Main, U'←');
    edit(Selector{Element::HorizontalScrollBar, Part::Increase}).setBlock(BlockRole::Main, U'→');
    edit(Selector{Element::VerticalScrollBar, Part::Track})
        .setBlocks(U"░░░░░░░░░░░░↑░↓↑"_el)
        .setMargins(bgeo::BlockMargins{0, 1});
    edit(Selector{Element::VerticalScrollBar, Part::Thumb}).setBlocks(U"            ▔ ▁ "_el);
    edit(Selector{Element::VerticalScrollBar, Part::Decrease}).setBlock(BlockRole::Main, U'↑');
    edit(Selector{Element::VerticalScrollBar, Part::Increase}).setBlock(BlockRole::Main, U'↓');
    edit(Selector{Element::Sections, Part::Border}).setBlocks(U'─');
    edit(Selector{Element::Sections, Part::Title})
        .setPadding(bgeo::BlockMargins{1, 0})
        .setMargins(bgeo::BlockMargins{2, 0});
    edit(Selector{Element::Sections, Part::Text}).setMargins(bgeo::BlockMargins{2, 0});
    edit(Selector{Element::Buttons, Part::Spacing}).setBlocks(U' ').setMargins(bgeo::BlockMargins{1, 0});
    edit(Selector{Element::Button, Part::Background}).setBlocks(U' ');
    edit(Selector{Element::Button, Part::Text}).setPadding(bgeo::BlockMargins{0, 1, 0, 0});
    edit(Selector{Element::Button, Part::Border})
        .setBracketBlocks(U'▌', U'▐')
        .setMargins(bgeo::BlockMargins{2, 0})
        .setPadding(bgeo::BlockMargins{2, 0});
    edit(Selector{Element::Frame, Part::Background}).setBlocks(U' ');
    edit(Selector{Element::Frame, Part::Border}).setBlocks(U"┌─┐│ │└─┘├─┤┬│┴┼"_el).setMargins(bgeo::BlockMargins{2});
    edit(Selector{Element::Frame, Part::Title}).setMargins(bgeo::BlockMargins{2, 0});
    edit(Selector{Element::Choice, Part::Text}).setMargins(bgeo::BlockMargins{0, 0, 1, 0});
    edit(Selector{Element::Choice, Part::Background}).setBlocks(U' ');
    edit(Selector{Element::Choice, Part::Border}).setBlocks(U"┌─┐│ │└─┘├─┤┬│┴┼"_el).setMargins(bgeo::BlockMargins{2});
    edit(Selector{Element::Choice, Part::Title}).setMargins(bgeo::BlockMargins{2, 0});
}

void ThemeBuilder::addPlainBlocks() {
    addClassicBlocks();
    edit(Selector{Element::Base, Part::TitleBracket}).setBracketBlocks(U'<', U'>');
    edit(Selector{Element::FooterLine, Part::Background}).setMargins(bgeo::BlockMargins{});
    edit(Selector{Element::HorizontalScrollBar, Part::Track})
        .setBlocks(U"                "_el)
        .setMargins(bgeo::BlockMargins{});
    edit(Selector{Element::HorizontalScrollBar, Part::Thumb}).setBlocks(U"         #######"_el);
    edit(Selector{Element::VerticalScrollBar, Part::Track})
        .setBlocks(U"                "_el)
        .setMargins(bgeo::BlockMargins{});
    edit(Selector{Element::VerticalScrollBar, Part::Thumb}).setBlocks(U"         #######"_el);
    edit(Selector{Element::Button, Part::Border}).setBracketBlocks(U'{', U'}');
    edit(Selector{Element::Frame, Part::Text}).setPadding(bgeo::BlockMargins{1, 0});
    edit(Selector{Element::Choice, Part::Text}).setMargins(bgeo::BlockMargins{0, 0, 1, 0});
}

}
