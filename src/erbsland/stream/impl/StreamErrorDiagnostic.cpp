// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StreamErrorDiagnostic.hpp"

#include "../../err/ErrorDocumentBuilder.hpp"
#include "../../i18n/DisplayTextMap.hpp"
#include "../../system/PlatformErrorContext.hpp"
#include "../../text/EscapeFormat.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringBuilder.hpp"
#include "../../text/TextDocument.hpp"
#include "../../text/TextNode.hpp"
#include "../../text/TextNodeType.hpp"
#include "../../text/u8/U8StringConstIterator.hpp"

#include <utility>

namespace erbsland::stream::impl {

using namespace text::literals;

StreamErrorDiagnostic::StreamErrorDiagnostic(StreamErrorContext context) noexcept : _context{std::move(context)} {
}

auto StreamErrorDiagnostic::sourcePath() const noexcept -> text::StringView {
    return _context.path();
}

auto StreamErrorDiagnostic::toString() const noexcept -> text::StringView {
    return _context.title();
}

void StreamErrorDiagnostic::appendPath(text::TextNode &content, const text::StringView path) {
    auto segment = text::StringBuilder{};
    const auto flushSegment = [&content, &segment]() -> void {
        if (!segment.isEmpty()) {
            content.addEscapedText(segment.toString(), text::EscapeFormat::Display);
            segment.clear();
        }
    };
    for (const auto character : path) {
        if (character == U'/' || character == U'\\') {
            flushSegment();
            content.add(text::TextNodeType::Separator)->addText(text::String::fromCharacter(character));
            continue;
        }
        segment.append(character);
    }
    flushSegment();
}

auto StreamErrorDiagnostic::toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const
    -> text::TextDocument {
    const auto resolvedDisplayText = displayText != nullptr ? displayText : i18n::DisplayTextMap::defaultMap();
    auto builder = err::ErrorDocumentBuilder{_context.title(), _context.description(), resolvedDisplayText};
    const auto help = _context.help();
    if (!help.isEmpty()) {
        builder.root()->addParagraph()->setStyle("diagnostic-help"_el).addText(help);
    }

    if (!_context.path().isEmpty()) {
        builder.addSection(resolvedDisplayText->text("PathValuesHeading"_el));
        auto list = builder.root()->add(text::TextNodeType::FieldList);
        auto item = list->add(text::TextNodeType::FieldItem);
        item->add(text::TextNodeType::FieldLabel)->addText("path"_el);
        auto content = item->add(text::TextNodeType::FieldContent);
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
