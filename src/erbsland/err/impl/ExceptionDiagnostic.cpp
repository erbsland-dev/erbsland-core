// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ExceptionDiagnostic.hpp"

#include "../ErrorDocumentBuilder.hpp"

#include "../../i18n/DisplayTextMap.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringBuilder.hpp"
#include "../../text/TextDocument.hpp"
#include "../../text/TextNode.hpp"
#include "../../text/TextNodeType.hpp"

#include <utility>

namespace erbsland::err::impl {

using namespace text::literals;

ExceptionDiagnostic::ExceptionDiagnostic(
    text::StringView message, const i18n::DisplayTextMapConstPtr &displayText) noexcept :
    _message{std::move(message)}, _displayText{displayText} {
}

auto ExceptionDiagnostic::setSourceName(text::StringView sourceName) noexcept -> ExceptionDiagnostic & {
    _sourceName = std::move(sourceName);
    return *this;
}

auto ExceptionDiagnostic::setSourcePath(text::StringView sourcePath) noexcept -> ExceptionDiagnostic & {
    _sourcePath = std::move(sourcePath);
    return *this;
}

auto ExceptionDiagnostic::setLocation(unit::CodeLocation location) noexcept -> ExceptionDiagnostic & {
    _location = location;
    return *this;
}

auto ExceptionDiagnostic::appendField(text::StringView label, text::StringView value, text::StringView style)
    -> ExceptionDiagnostic & {
    _fields.append(Field{std::move(label), std::move(value), std::move(style)});
    return *this;
}

auto ExceptionDiagnostic::sourceName() const noexcept -> text::StringView {
    return _sourceName;
}

auto ExceptionDiagnostic::sourcePath() const noexcept -> text::StringView {
    return _sourcePath;
}

auto ExceptionDiagnostic::location() const noexcept -> unit::CodeLocation {
    return _location;
}

auto ExceptionDiagnostic::toString() const noexcept -> text::StringView {
    return _message;
}

auto ExceptionDiagnostic::toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const -> text::TextDocument {
    const auto resolvedDisplayText = displayText != nullptr
        ? displayText
        : (_displayText != nullptr ? _displayText : i18n::DisplayTextMap::defaultMap());
    auto builder = ErrorDocumentBuilder{
        _message.isEmpty() ? resolvedDisplayText->text("ExceptionWithoutDiagnosticText"_el) : _message,
        {},
        resolvedDisplayText};
    if (!_fields.isEmpty()) {
        auto list = builder.root()->add(text::TextNodeType::FieldList);
        for (const auto &field : _fields) {
            auto item = list->add(text::TextNodeType::FieldItem);
            item->add(text::TextNodeType::FieldLabel)->addText(field.label);
            auto content = item->add(text::TextNodeType::FieldContent);
            content->setStyle(field.style);
            if (field.style.contains("path"_el)) {
                auto segment = text::StringBuilder{};
                for (const auto character : field.value) {
                    if (character == U'/') {
                        if (!segment.isEmpty()) {
                            content->addText(segment.toString());
                            segment.clear();
                        }
                        content->add(text::TextNodeType::Separator)->addText("/"_el);
                    } else {
                        segment.append(character);
                    }
                }
                if (!segment.isEmpty()) {
                    content->addText(segment.toString());
                }
            } else {
                content->addText(field.value);
            }
        }
    }
    return builder.takeDocument();
}

}
