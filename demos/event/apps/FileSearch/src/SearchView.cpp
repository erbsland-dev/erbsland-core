// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SearchView.hpp"

#include <erbsland/Alignment.hpp>
#include <erbsland/CpRange.hpp>
#include <erbsland/cterm/BlockStringEditor.hpp>
#include <erbsland/cterm/FrameStyle.hpp>
#include <erbsland/StringFormat.hpp>

namespace demo {

using namespace el::text::literals;
using namespace el::cterm;
using el::block::Coordinate;

auto SearchView::cropLeft(const el::String &text, const Coordinate width) -> BlockString {
    if (width <= Coordinate{}) {
        return {};
    }
    auto result = BlockString{text};
    if (result.displayWidth() <= width.toRawValue()) {
        return result;
    }
    auto cropped = BlockStringEditor{};
    cropped.append("…"_el);
    cropped.append(result.croppedToDisplayWidth(width - Coordinate{1}, el::Alignment::Right));
    return cropped;
}

auto SearchView::render(
    const el::Path &root,
    const QueryEditor &query,
    SearchResults &results,
    const el::block::Size size,
    const bool cursorVisible) -> const Buffer & {
    _buffer.resize(size);
    _buffer.fill(Block{U' ', fg::White, bg::Black});
    if (size.width() < Coordinate{48} || size.height() < Coordinate{12}) {
        _buffer.drawBlockText({0, 0}, BlockString{"Resize to at least 48x12. Esc exits."_el});
        return _buffer;
    }
    const auto layout = Layout{
        .width = size.width(),
        .contentWidth = size.width() - Coordinate{4},
        .resultRows = size.height() - Coordinate{10},
        .inputY = size.height() - Coordinate{3},
        .helpY = size.height() - Coordinate{1}};
    results.resize(el::ItemCount::fromSizeTOrThrow(layout.resultRows.toSizeT()));
    drawRoot(root, layout);
    drawResults(results, layout);
    drawInput(query, layout, cursorVisible);
    drawHelp(layout);
    return _buffer;
}

void SearchView::drawRoot(const el::Path &root, const Layout &layout) {
    _buffer.drawFrame({Coordinate{0}, Coordinate{0}, layout.width, Coordinate{3}}, FrameStyle::Light, fg::Cyan);
    _buffer.drawBlockText({2, 0}, BlockString{" Search root "_el});
    _buffer.drawBlockText({2, 1}, cropLeft(root.toNativeString(), layout.contentWidth));
}

void SearchView::drawResults(const SearchResults &results, const Layout &layout) {
    _buffer.drawFrame(
        {Coordinate{0}, Coordinate{3}, layout.width, layout.resultRows + Coordinate{3}}, FrameStyle::Light, fg::Cyan);
    _buffer.drawBlockText({2, 3}, BlockString{el::StringFormat{" Results: {} "_el}.build(results.paths().count())});
    for (auto row = Coordinate{}; row < layout.resultRows; ++row) {
        const auto index = results.first() + el::ItemCount::fromSizeTOrThrow(row.toSizeT());
        if (!index.isWithin(results.paths().count())) {
            break;
        }
        drawResultRow(results, layout, row);
    }
    _buffer.drawBlockText(
        {Coordinate{2}, layout.resultRows + Coordinate{4}},
        BlockString{results.status()}
            .croppedToDisplayWidth(layout.contentWidth, el::Alignment::Left)
            .withBase({fg::Yellow}));
}

void SearchView::drawResultRow(const SearchResults &results, const Layout &layout, const Coordinate row) {
    const auto index = results.first() + el::ItemCount::fromSizeTOrThrow(row.toSizeT());
    auto style = BlockStyle{fg::White, bg::Black};
    if (index == results.selected()) {
        style = BlockStyle{fg::BrightWhite, bg::Blue};
    }
    const auto y = row + Coordinate{4};
    _buffer.fill({Coordinate{1}, y, layout.width - Coordinate{2}, Coordinate{1}}, Block{U' ', style});
    _buffer.drawBlockText(
        {Coordinate{2}, y},
        cropLeft(results.paths().getRef(index).toNativeString(), layout.contentWidth).withBase(style));
}

void SearchView::drawInput(const QueryEditor &query, const Layout &layout, const bool cursorVisible) {
    const auto inputY = layout.inputY;
    _buffer.drawFrame(
        {Coordinate{0}, inputY - Coordinate{1}, layout.width, Coordinate{3}}, FrameStyle::Light, fg::Cyan);
    _buffer.drawBlockText({Coordinate{2}, inputY - Coordinate{1}}, BlockString{" Glob (relative path) "_el});
    const auto text = query.text();
    const auto prefix =
        BlockString{text.slice(el::CpRange{el::CpIndex{}, query.cursor().distanceFromZero()})}.croppedToDisplayWidth(
            layout.contentWidth - Coordinate{1}, el::Alignment::Right);
    const auto cursorX = Coordinate{2} + Coordinate{prefix.displayWidth()};
    _buffer.drawBlockText({Coordinate{2}, inputY}, prefix);
    _buffer.drawBlockText(
        {cursorX, inputY},
        BlockString{text.slice(el::CpRange{query.cursor(), el::CpCount::infinite()})}.croppedToDisplayWidth(
            layout.width - Coordinate{2} - cursorX, el::Alignment::Left));
    if (cursorVisible) {
        const auto position = el::block::Position{cursorX, inputY};
        _buffer.set(position, _buffer.get(position).withOverlay({fg::Black, bg::BrightWhite}));
    }
}

void SearchView::drawHelp(const Layout &layout) {
    _buffer.drawBlockText(
        {Coordinate{1}, layout.helpY},
        BlockString{"↑/↓ PgUp/PgDn select · Enter choose · Esc quit"_el}.croppedToDisplayWidth(
            layout.width - Coordinate{2}, el::Alignment::Left));
}

}
