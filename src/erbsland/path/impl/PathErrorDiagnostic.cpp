// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathErrorDiagnostic.hpp"

#include "../../err/ErrorDocumentBuilder.hpp"
#include "../../i18n/DisplayTextMap.hpp"
#include "../../system/PlatformErrorContext.hpp"
#include "../../text/EscapeFormat.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringBuilder.hpp"
#include "../../text/TextDocument.hpp"
#include "../../text/TextNode.hpp"
#include "../../text/TextNodeType.hpp"

#include <utility>

namespace erbsland::path::impl {

using namespace text::literals;

PathErrorDiagnostic::PathErrorDiagnostic(PathErrorContext context) noexcept : _context{std::move(context)} {
}

auto PathErrorDiagnostic::sourcePath() const noexcept -> text::StringView {
    return !_context.sourcePath().isEmpty() ? _context.sourcePath() : _context.targetPath();
}

auto PathErrorDiagnostic::toString() const noexcept -> text::StringView {
    return _context.title();
}

void PathErrorDiagnostic::appendPath(text::TextNode &content, const text::StringView path) {
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

auto PathErrorDiagnostic::toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const -> text::TextDocument {
    const auto resolvedDisplayText = displayText != nullptr ? displayText : i18n::DisplayTextMap::defaultMap();
    auto builder = err::ErrorDocumentBuilder{_context.title(), _context.description(), resolvedDisplayText};
    const auto help = _context.help();
    if (!help.isEmpty()) {
        builder.root()->addParagraph()->setStyle("diagnostic-help"_el).addText(help);
    }

    const auto hasSource = !_context.sourcePath().isEmpty();
    const auto hasTarget = !_context.targetPath().isEmpty();
    if (hasSource || hasTarget) {
        builder.addSection(resolvedDisplayText->text("PathValuesHeading"_el));
        auto list = builder.root()->add(text::TextNodeType::FieldList);
        const auto addPath = [&list](const text::StringView label, const text::StringView path) -> void {
            auto item = list->add(text::TextNodeType::FieldItem);
            item->add(text::TextNodeType::FieldLabel)->addText(label);
            auto content = item->add(text::TextNodeType::FieldContent);
            content->setStyle("path"_el);
            PathErrorDiagnostic::appendPath(*content, path);
        };
        if (hasSource) {
            addPath(hasTarget ? "source path"_el : "path"_el, _context.sourcePath());
        }
        if (hasTarget) {
            addPath("target path"_el, _context.targetPath());
        }
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
