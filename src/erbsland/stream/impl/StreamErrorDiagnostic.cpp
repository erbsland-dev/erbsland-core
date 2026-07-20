// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StreamErrorDiagnostic.hpp"

#include "../../err/ErrorDocumentBuilder.hpp"
#include "../../i18n/DisplayTextMap.hpp"
#include "../../system/PlatformErrorContext.hpp"
#include "../../text/EscapeFormat.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringEditor.hpp"
#include "../../text/TextDocument.hpp"
#include "../../text/TextNode.hpp"
#include "../../text/TextNodeType.hpp"
#include "../../text/u8/U8StringConstIterator.hpp"

#include <utility>

namespace erbsland::stream::impl {

using namespace text::literals;
using namespace text;

StreamErrorDiagnostic::StreamErrorDiagnostic(StreamErrorContext context) noexcept : _context{std::move(context)} {
}

auto StreamErrorDiagnostic::sourcePath() const noexcept -> String {
    return _context.path();
}

auto StreamErrorDiagnostic::toString() const noexcept -> String {
    return _context.title();
}

void StreamErrorDiagnostic::appendPath(TextNode &content, const String &path) {
    auto segment = StringEditor{};
    const auto flushSegment = [&content, &segment]() -> void {
        if (!segment.isEmpty()) {
            content.addEscapedText(segment, EscapeFormat::Display);
            segment.clear();
        }
    };
    for (const auto character : path) {
        if (character == U'/' || character == U'\\') {
            flushSegment();
            content.add(TextNodeType::Separator)->addText(String::fromCharacter(character));
            continue;
        }
        segment.append(character);
    }
    flushSegment();
}

auto StreamErrorDiagnostic::toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const -> TextDocument {
    const auto resolvedDisplayText = displayText != nullptr ? displayText : i18n::DisplayTextMap::defaultMap();
    auto builder = err::ErrorDocumentBuilder{_context.title(), _context.description(), resolvedDisplayText};
    const auto help = _context.help();
    if (!help.isEmpty()) {
        builder.root()->addParagraph()->setStyle("diagnostic-help"_el).addText(help);
    }

    if (!_context.path().isEmpty()) {
        builder.addSection(resolvedDisplayText->text("PathValuesHeading"_el));
        auto list = builder.root()->add(TextNodeType::FieldList);
        auto item = list->add(TextNodeType::FieldItem);
        item->add(TextNodeType::FieldLabel)->addText("path"_el);
        auto content = item->add(TextNodeType::FieldContent);
        content->setStyle("path"_el);
        appendPath(*content, _context.path());
    }

    if (_context.platformContext() != nullptr) {
        builder.addSection(resolvedDisplayText->text("PlatformErrorHeading"_el));
        auto platformDocument = _context.platformContext()->toTextDocument();
        for (const auto &child : platformDocument.root()->children()) {
            builder.root()->add(child->clone());
        }
    }
    return builder.takeDocument();
}

}
