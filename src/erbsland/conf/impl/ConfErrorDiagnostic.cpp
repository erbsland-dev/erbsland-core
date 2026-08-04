// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConfErrorDiagnostic.hpp"

#include "../../err/ErrorDocumentBuilder.hpp"
#include "../../text/CodeSnippetMarker.hpp"
#include "../../text/Literals.hpp"
#include "../../text/PlainTextRenderer.hpp"
#include "../../text/TextNode.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::conf::impl {

using namespace text::literals;

ConfErrorDiagnostic::ConfErrorDiagnostic(ConfErrorContext context) noexcept : _context{std::move(context)} {
}

auto ConfErrorDiagnostic::sourcePath() const noexcept -> text::String {
    return _context.filePath().has_value() ? _context.filePath()->toString() : text::String{};
}

auto ConfErrorDiagnostic::location() const noexcept -> unit::CodeLocation {
    return _context.location().value_or(unit::CodeLocation{});
}

auto ConfErrorDiagnostic::toString() const noexcept -> text::String {
    try {
        auto document = toTextDocument();
        return text::PlainTextRenderer{document}.build();
    } catch (...) {
        return _context.title();
    }
}

auto ConfErrorDiagnostic::toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const -> text::TextDocument {
    auto builder = err::ErrorDocumentBuilder{_context.title(), _context.description(), displayText};
    builder.addSource({}, sourcePath(), location());

    builder.addSection("Configuration Error Details"_el);
    auto details = builder.root()->add(text::TextNodeType::FieldList);
    auto category = details->add(text::TextNodeType::FieldItem);
    category->add(text::TextNodeType::FieldLabel)->addText("category"_el);
    category->add(text::TextNodeType::FieldContent)->addText(_context.category().toText());
    if (_context.namePath().has_value()) {
        auto namePath = details->add(text::TextNodeType::FieldItem);
        namePath->add(text::TextNodeType::FieldLabel)->addText("name path"_el);
        namePath->add(text::TextNodeType::FieldContent)->addText(_context.namePath()->toText());
    }

    if (_context.codeSnippet().has_value()) {
        auto markers = text::CodeSnippetMarkerList{};
        if (_context.location().has_value() && !_context.location()->line().isNoIndex() &&
            !_context.location()->column().isNoIndex() && !_context.codeSnippet()->startLine.isNoIndex() &&
            _context.location()->line() >= _context.codeSnippet()->startLine) {
            const auto localLine = _context.location()->line().toSizeT() - _context.codeSnippet()->startLine.toSizeT();
            if (localLine < _context.codeSnippet()->lines.count().toSizeT()) {
                const auto &sourceLine = _context.codeSnippet()->lines.get(unit::ItemIndex{localLine});
                const auto lineLength = sourceLine.characterLength().toSizeT();
                const auto clippedColumn = lineLength == 0U
                    ? unit::ColumnIndex::zero()
                    : unit::ColumnIndex::fromSizeT(std::min(_context.location()->column().toSizeT(), lineLength - 1U));
                markers.append(
                    text::CodeSnippetMarker{
                        _context.location()->line(),
                        clippedColumn,
                        lineLength == 0U ? unit::ColumnCount{} : unit::ColumnCount::one(),
                        {},
                        "error"_el});
            }
        }
        builder.root()->addCodeSnippet(*_context.codeSnippet(), std::move(markers));
    }
    return builder.takeDocument();
}

}
