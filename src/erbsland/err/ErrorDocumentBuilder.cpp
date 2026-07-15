// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ErrorDocumentBuilder.hpp"

#include "../i18n/DisplayTextMap.hpp"
#include "../text/Literals.hpp"
#include "../text/String.hpp"
#include "../text/TextNode.hpp"

namespace erbsland::err {

using namespace text::literals;

ErrorDocumentBuilder::ErrorDocumentBuilder(
    text::StringView title, text::StringView description, const i18n::DisplayTextMapConstPtr &displayText) :
    _displayText{displayText != nullptr ? displayText : i18n::DisplayTextMap::defaultMap()} {
    _document.root()->setStyle("error"_el);
    auto titleNode = _document.addHeading(1);
    titleNode->setStyle("diagnostic-title error"_el);
    titleNode->addText(title.isEmpty() ? _displayText->text("UnknownError"_el) : std::move(title));
    if (!description.isEmpty()) {
        _document.addParagraph()->setStyle("diagnostic-description"_el).addText(std::move(description));
    }
}

auto ErrorDocumentBuilder::addSection(text::StringView title) -> text::TextNodePtr {
    auto heading = _document.addHeading(2);
    heading->setStyle("diagnostic-section"_el);
    heading->addText(std::move(title));
    return heading;
}

void ErrorDocumentBuilder::addSource(
    const text::StringView &sourceName, const text::StringView &sourcePath, const unit::CodeLocation location) {
    if (sourceName.isEmpty() && sourcePath.isEmpty() && location.line.isNoIndex() && location.column.isNoIndex() &&
        location.position.isNoIndex()) {
        return;
    }
    addSection(_displayText->text("ErrorSourceHeading"_el));
    auto list = _document.root()->add(text::TextNodeType::FieldList);
    const auto addField = [&](const text::StringView &label, text::StringView value, const text::StringView &style) {
        auto item = list->add(text::TextNodeType::FieldItem);
        item->setStyle(style);
        item->add(text::TextNodeType::FieldLabel)->addText(label);
        item->add(text::TextNodeType::FieldContent)->addText(std::move(value));
    };
    if (!sourceName.isEmpty()) {
        addField(_displayText->text("SourceNameLabel"_el), sourceName, "diagnostic-source-name"_el);
    }
    if (!sourcePath.isEmpty()) {
        addField(_displayText->text("SourcePathLabel"_el), sourcePath, "diagnostic-source-name"_el);
    }
    if (!location.line.isNoIndex()) {
        addField(
            _displayText->text("LineLabel"_el),
            text::String::fromInteger(location.line.toSizeT() + 1U),
            "diagnostic-source-location"_el);
    }
    if (!location.column.isNoIndex()) {
        addField(
            _displayText->text("ColumnLabel"_el),
            text::String::fromInteger(location.column.toSizeT() + 1U),
            "diagnostic-source-location"_el);
    }
    if (!location.position.isNoIndex()) {
        addField(
            _displayText->text("PositionLabel"_el),
            text::String::fromInteger(location.position.toSizeT() + 1U),
            "diagnostic-source-location"_el);
    }
}

void ErrorDocumentBuilder::addSourceField(
    const text::StringView &label, text::StringView value, const text::StringView &style) {
    auto paragraph = _document.addParagraph();
    paragraph->setStyle(style);
    paragraph->addText(label);
    paragraph->addText(":"_el);
    paragraph->addText(" "_el);
    paragraph->addText(std::move(value));
}

auto ErrorDocumentBuilder::takeDocument() -> text::TextDocument {
    return std::move(_document);
}

}
