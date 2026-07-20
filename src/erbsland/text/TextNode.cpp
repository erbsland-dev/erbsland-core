// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TextNode.hpp"

#include "AnyStringBuilder.hpp"
#include "Literals.hpp"
#include "StringCharReader.hpp"
#include "StringEditor.hpp"
#include "StringTree.hpp"
#include "TextNodeData.hpp"

#include "impl/CodeBlockData.hpp"
#include "impl/CodeLineMarkerData.hpp"
#include "impl/CodeSnippetData.hpp"
#include "impl/EscapeFormatter.hpp"
#include "impl/LinkData.hpp"

#include "../err/ParameterError.hpp"
#include "../unit/ElementIndex.hpp"
#include "../unit/LineIndex.hpp"

#include <memory>
#include <utility>

namespace erbsland::text {

using namespace literals;

auto TextNode::codeSnippetLineIndex(const unit::LineIndex startLine, const std::size_t localLine) noexcept
    -> unit::LineIndex {
    if (startLine.isNoIndex()) {
        return unit::LineIndex::fromSizeT(localLine);
    }
    return unit::LineIndex::fromSizeT(startLine.toSizeT() + localLine);
}

TextNode::TextNode(
    Type type, String text, String identifier, String style, TextNodeDataPtr data, const Level level, PrivateTag) :
    _type{type},
    _text{std::move(text)},
    _identifier{std::move(identifier)},
    _style{std::move(style)},
    _data{std::move(data)},
    _level{level} {
}

auto TextNode::create(Type type) -> TextNodePtr {
    return createNode(type);
}

auto TextNode::createDocument() -> TextNodePtr {
    return createNode(Type::Document);
}

auto TextNode::createParagraph() -> TextNodePtr {
    return createNode(Type::Paragraph);
}

auto TextNode::createSection() -> TextNodePtr {
    return createNode(Type::Section);
}

auto TextNode::createBlockquote() -> TextNodePtr {
    return createNode(Type::Blockquote);
}

auto TextNode::createLineBreak() -> TextNodePtr {
    return createNode(Type::LineBreak);
}

auto TextNode::createHeading(const Level level) -> TextNodePtr {
    return createNode(Type::Heading, {}, {}, {}, {}, level);
}

auto TextNode::createBulletList(const Level level) -> TextNodePtr {
    return createNode(Type::BulletList, {}, {}, {}, {}, level);
}

auto TextNode::createNumberedList(const Level level) -> TextNodePtr {
    return createNode(Type::NumberedList, {}, {}, {}, {}, level);
}

auto TextNode::createBulletListItem() -> TextNodePtr {
    return createNode(Type::BulletListItem);
}

auto TextNode::createNumberedListItem() -> TextNodePtr {
    return createNode(Type::NumberedListItem);
}

auto TextNode::createDefinitionList() -> TextNodePtr {
    return createNode(Type::DefinitionList);
}

auto TextNode::createDefinitionTerm() -> TextNodePtr {
    return createNode(Type::DefinitionTerm);
}

auto TextNode::createDefinitionDescription() -> TextNodePtr {
    return createNode(Type::DefinitionDescription);
}

auto TextNode::createCodeBlock(String language) -> TextNodePtr {
    auto data = TextNodeDataPtr{};
    if (!language.isEmpty()) {
        data = std::make_shared<impl::CodeBlockData>(std::move(language));
    }
    return createNode(Type::CodeBlock, {}, {}, {}, std::move(data));
}

auto TextNode::createCodeSnippet(CodeSnippet snippet, CodeSnippetMarkerList markers) -> TextNodePtr {
    auto data = TextNodeDataPtr{};
    if (!snippet.language.isEmpty()) {
        data = std::make_shared<impl::CodeSnippetData>(std::move(snippet.language));
    }
    auto result = createNode(Type::CodeSnippet, {}, {}, {}, std::move(data));
    for (auto localIndex = std::size_t{0U}; localIndex < snippet.lines.count().toSizeT(); ++localIndex) {
        const auto lineIndex = codeSnippetLineIndex(snippet.startLine, localIndex);
        auto line = result->add(Type::CodeLine);
        if (!snippet.startLine.isNoIndex()) {
            line->add(Type::CodeLineNumber)->addText(String::fromInteger(lineIndex.toSizeT() + 1U));
        }
        line->add(Type::CodeLineText)->addText(snippet.lines.get(unit::ElementIndex::fromSizeT(localIndex)));
        for (const auto &marker : markers) {
            if (marker.line() == lineIndex && !marker.column().isNoIndex()) {
                line->add(Type::CodeLineMarker)
                    ->setData(std::make_shared<impl::CodeLineMarkerData>(marker.column(), marker.length()))
                    .setStyle(marker.style())
                    .addText(marker.label());
            }
        }
    }
    return result;
}

auto TextNode::createHorizontalLine() -> TextNodePtr {
    return createNode(Type::HorizontalLine);
}

auto TextNode::createText(String text) -> TextNodePtr {
    return createNode(Type::Text, std::move(text));
}

auto TextNode::createEmphasis() -> TextNodePtr {
    return createNode(Type::Emphasis);
}

auto TextNode::createStrong() -> TextNodePtr {
    return createNode(Type::Strong);
}

auto TextNode::createUnderline() -> TextNodePtr {
    return createNode(Type::Underline);
}

auto TextNode::createSpan() -> TextNodePtr {
    return createNode(Type::Span);
}

auto TextNode::createLink(String url) -> TextNodePtr {
    auto data = TextNodeDataPtr{};
    if (!url.isEmpty()) {
        data = std::make_shared<impl::LinkData>(std::move(url));
    }
    return createNode(Type::Link, {}, {}, {}, std::move(data));
}

auto TextNode::createCode() -> TextNodePtr {
    return createNode(Type::Code);
}

auto TextNode::createUnsupported(String text) -> TextNodePtr {
    return createNode(Type::Unsupported, std::move(text));
}

auto TextNode::createError(String text) -> TextNodePtr {
    return createNode(Type::Error, std::move(text));
}

auto TextNode::add(Type type) -> TextNodePtr {
    return appendChild(create(type));
}

auto TextNode::add(TextNodePtr child) -> TextNodePtr {
    return appendChild(std::move(child));
}

auto TextNode::addParagraph() -> TextNodePtr {
    return appendChild(createParagraph());
}

auto TextNode::addSection() -> TextNodePtr {
    return appendChild(createSection());
}

auto TextNode::addBlockquote() -> TextNodePtr {
    return appendChild(createBlockquote());
}

auto TextNode::addLineBreak() -> TextNodePtr {
    return appendChild(createLineBreak());
}

auto TextNode::addHeading(const Level level) -> TextNodePtr {
    return appendChild(createHeading(level));
}

auto TextNode::addBulletList(const Level level) -> TextNodePtr {
    return appendChild(createBulletList(level));
}

auto TextNode::addNumberedList(const Level level) -> TextNodePtr {
    return appendChild(createNumberedList(level));
}

auto TextNode::addListItem() -> TextNodePtr {
    if (_type == Type::BulletList) {
        return addBulletListItem();
    }
    if (_type == Type::NumberedList) {
        return addNumberedListItem();
    }
    return {};
}

auto TextNode::addBulletListItem() -> TextNodePtr {
    return appendChild(createBulletListItem());
}

auto TextNode::addNumberedListItem() -> TextNodePtr {
    return appendChild(createNumberedListItem());
}

auto TextNode::addDefinitionList() -> TextNodePtr {
    return appendChild(createDefinitionList());
}

auto TextNode::addDefinitionTerm() -> TextNodePtr {
    return appendChild(createDefinitionTerm());
}

auto TextNode::addDefinitionDescription() -> TextNodePtr {
    return appendChild(createDefinitionDescription());
}

auto TextNode::addCodeBlock(String language) -> TextNodePtr {
    return appendChild(createCodeBlock(std::move(language)));
}

auto TextNode::addCodeSnippet(CodeSnippet snippet, CodeSnippetMarkerList markers) -> TextNodePtr {
    return appendChild(createCodeSnippet(std::move(snippet), std::move(markers)));
}

auto TextNode::addHorizontalLine() -> TextNodePtr {
    return appendChild(createHorizontalLine());
}

auto TextNode::addText(String text) -> TextNodePtr {
    return appendChild(createText(std::move(text)));
}

auto TextNode::addEscapedText(const String &text, const EscapeFormat format, const EscapeAmount amount) -> TextNode & {
    const auto formatter = impl::EscapeFormatter::forFormat(format);
    auto reader = StringCharReader{text};
    auto plainText = AnyStringBuilder{};
    const auto flushPlainText = [this, &plainText]() -> void {
        if (!plainText.isEmpty()) {
            addText(plainText.toString());
            plainText.clear();
        }
    };
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (!formatter->needsEscape(character, amount)) {
            plainText.append(character);
            continue;
        }
        flushPlainText();
        auto escapeText = AnyStringBuilder{};
        formatter->escape(character, escapeText);
        add(Type::EscapeSequence)->setText(escapeText.toString());
    }
    flushPlainText();
    return *this;
}

auto TextNode::addEmphasis() -> TextNodePtr {
    return appendChild(createEmphasis());
}

auto TextNode::addStrong() -> TextNodePtr {
    return appendChild(createStrong());
}

auto TextNode::addUnderline() -> TextNodePtr {
    return appendChild(createUnderline());
}

auto TextNode::addSpan() -> TextNodePtr {
    return appendChild(createSpan());
}

auto TextNode::addLink(String url) -> TextNodePtr {
    return appendChild(createLink(std::move(url)));
}

auto TextNode::addCode() -> TextNodePtr {
    return appendChild(createCode());
}

auto TextNode::addUnsupported(String text) -> TextNodePtr {
    return appendChild(createUnsupported(std::move(text)));
}

auto TextNode::addError(String text) -> TextNodePtr {
    return appendChild(createError(std::move(text)));
}

auto TextNode::setText(String text) noexcept -> TextNode & {
    _text = std::move(text);
    return *this;
}

auto TextNode::setIdentifier(String identifier) noexcept -> TextNode & {
    _identifier = std::move(identifier);
    return *this;
}

auto TextNode::setStyle(String style) noexcept -> TextNode & {
    _style = std::move(style);
    return *this;
}

auto TextNode::setData(TextNodeDataPtr data) noexcept -> TextNode & {
    _data = std::move(data);
    return *this;
}

auto TextNode::setLevel(const Level level) noexcept -> TextNode & {
    _level = level;
    return *this;
}

auto TextNode::hasChildren() const noexcept -> bool {
    return !_children.isEmpty();
}

auto TextNode::hasParent() const noexcept -> bool {
    return !_parent.expired();
}

auto TextNode::parent() const noexcept -> TextNodePtr {
    return _parent.lock();
}

auto TextNode::contains(const Type type) const -> bool {
    return anyOf([type](const TextNode &node) -> bool { return node.type() == type; });
}

auto TextNode::clone() const -> TextNodePtr {
    auto result = createNode(_type, _text, _identifier, _style, _data, _level);
    for (const auto &child : _children) {
        result->appendChild(child->clone());
    }
    return result;
}

auto TextNode::toDiagnosticTree() const -> StringTree {
    auto result = StringTree{_type.toString()};
    if (_level != 0) {
        result.append("level"_el, _level);
    }
    if (!_identifier.isEmpty()) {
        result.append("identifier"_el, _identifier);
    }
    if (!_style.isEmpty()) {
        result.append("style"_el, _style);
    }
    if (_data != nullptr) {
        result.append("data"_el, _data->toString());
    }
    if (!_text.isEmpty()) {
        result.append("text"_el, _text);
    }
    if (!_children.isEmpty()) {
        result.appendList(
            "children"_el, _children, [](const TextNodePtr &child) -> StringTree { return child->toDiagnosticTree(); });
    }
    return result;
}

auto TextNode::createNode(
    Type type, String text, String identifier, String style, TextNodeDataPtr data, const Level level) -> TextNodePtr {
    return std::make_shared<TextNode>(
        type, std::move(text), std::move(identifier), std::move(style), std::move(data), level, PrivateTag{});
}

auto TextNode::appendChild(TextNodePtr child) -> TextNodePtr {
    if (child == nullptr) {
        throw err::ParameterError{"The child text node must not be null."_el, "child"_el};
    }
    child->_parent = shared_from_this();
    _children.append(child);
    return child;
}

}
