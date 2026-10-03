// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConfErrorDiagnostic.hpp"

#include "../../err/ErrorDocumentBuilder.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../text/CodeSnippetMarker.hpp"
#include "../../text/Literals.hpp"
#include "../../text/PlainTextRenderer.hpp"
#include "../../text/TextNode.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::conf::impl {

using namespace text;
using namespace unit;
using namespace text::literals;

ConfErrorDiagnostic::ConfErrorDiagnostic(ConfErrorContext context) noexcept : _context{std::move(context)} {
}

auto ConfErrorDiagnostic::sourcePath() const noexcept -> String {
    return _context.filePath().has_value() ? _context.filePath()->toString() : String{};
}

auto ConfErrorDiagnostic::location() const noexcept -> CodeLocation {
    return _context.location().value_or(CodeLocation{});
}

auto ConfErrorDiagnostic::toString() const noexcept -> String {
    try {
        auto document = toTextDocument();
        return PlainTextRenderer{document}.build();
    } catch (const err::RuntimeError &) {
        return _context.title();
    }
}

auto ConfErrorDiagnostic::toTextDocument() const -> TextDocument {
    auto builder = err::ErrorDocumentBuilder{_context.title(), _context.description()};
    builder.addSource({}, sourcePath(), location());

    builder.addSection("Configuration Error Details"_el);
    auto details = builder.root()->add(TextNodeType::FieldList);
    auto category = details->add(TextNodeType::FieldItem);
    category->add(TextNodeType::FieldLabel)->addText("category"_el);
    category->add(TextNodeType::FieldContent)->addText(_context.category().toText());
    if (_context.namePath().has_value()) {
        auto namePath = details->add(TextNodeType::FieldItem);
        namePath->add(TextNodeType::FieldLabel)->addText("name path"_el);
        namePath->add(TextNodeType::FieldContent)->addText(_context.namePath()->toText());
    }

    if (_context.codeSnippet().has_value()) {
        auto markers = CodeSnippetMarkerList{};
        if (_context.location().has_value() && !_context.location()->line().isNoIndex() &&
            !_context.location()->column().isNoIndex() && !_context.codeSnippet()->startLine.isNoIndex() &&
            _context.location()->line() >= _context.codeSnippet()->startLine) {
            const auto lineDistance = _context.codeSnippet()->startLine.absoluteDistanceTo(_context.location()->line());
            const auto localLine = ItemIndex{lineDistance.toRawValue(), ItemIndex::Unchecked{}};
            if (localLine.isWithin(_context.codeSnippet()->lines.count())) {
                const auto &sourceLine = _context.codeSnippet()->lines.get(localLine);
                const auto lineLength = sourceLine.characterLength().toSizeT();
                const auto clippedColumn = lineLength == 0U
                    ? ColumnIndex::zero()
                    : ColumnIndex::fromSizeT(std::min(_context.location()->column().toSizeT(), lineLength - 1U));
                markers.append(
                    CodeSnippetMarker{
                        _context.location()->line(),
                        clippedColumn,
                        lineLength == 0U ? ColumnCount{} : ColumnCount::one(),
                        {},
                        "error"_el});
            }
        }
        builder.root()->addCodeSnippet(*_context.codeSnippet(), std::move(markers));
    }
    return builder.takeDocument();
}

}
