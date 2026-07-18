// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HtmlParser.hpp"

#include "../../impl/LinkData.hpp"
#include "../../Literals.hpp"
#include "../../StringCharReader.hpp"
#include "../../StringEditor.hpp"

#include <memory>
#include <utility>

namespace erbsland::text::html::impl {

using namespace literals;

void HtmlParser::applyAttributes(const TextNodePtr &node, const String &tagName, const Attributes &attributes) {
    if (!node) {
        return;
    }
    for (const auto &attribute : attributes) {
        if (attribute.name() == "id"_el) {
            node->setIdentifier(attribute.value());
            continue;
        }
        if (attribute.name() == "class"_el) {
            node->setStyle(attribute.value());
            continue;
        }
        if (attribute.name() == "href"_el && tagName == "a"_el) {
            node->setData(std::make_shared<text::impl::LinkData>(attribute.value()));
        }
    }
}

auto HtmlParser::addNodeForTag(const String &tagName, const TextNode::Level listLevel) -> TextNodePtr {
    if (const auto *tagInfo = findTagInfo(tagName)) {
        if (tagName == "ul"_el) {
            return _current->addBulletList(listLevel);
        }
        if (tagName == "ol"_el) {
            return _current->addNumberedList(listLevel);
        }
        if (tagInfo->hasNodeType()) {
            if (tagInfo->nodeType() == TextNode::Type::Heading) {
                return _current->addHeading(tagInfo->level());
            }
            return _current->add(tagInfo->nodeType());
        }
    }
    return {};
}

auto HtmlParser::addListItemNode() -> TextNodePtr {
    if (!_current) {
        return {};
    }
    if (auto node = _current->addListItem()) {
        return node;
    }
    return _current->addBulletListItem();
}

auto HtmlParser::isSuppressed() const noexcept -> bool {
    return _suppressedFrameDepth > 0;
}

auto HtmlParser::isPreservingWhitespace() const noexcept -> bool {
    return _preservedWhitespaceFrameDepth > 0;
}

auto HtmlParser::currentNodeType() const noexcept -> TextNode::Type {
    if (!_current) {
        return TextNode::Type::Document;
    }
    return _current->type();
}

auto HtmlParser::currentNodeHasChildren() const noexcept -> bool {
    return _current && _current->hasChildren();
}

auto HtmlParser::countOpenListLevels() const noexcept -> TextNode::Level {
    return _openListLevel;
}

void HtmlParser::updateFrameStateForPush(const Frame &frame) noexcept {
    if (frame.suppressSubtree()) {
        _suppressedFrameDepth += 1;
    }
    if (frame.preserveWhitespace()) {
        _preservedWhitespaceFrameDepth += 1;
    }
    if (frame.node() && frame.node()->type().isListContainer()) {
        _openListLevel += 1;
    }
}

void HtmlParser::updateFrameStateForPop(const Frame &frame) noexcept {
    if (frame.suppressSubtree()) {
        _suppressedFrameDepth -= 1;
    }
    if (frame.preserveWhitespace()) {
        _preservedWhitespaceFrameDepth -= 1;
    }
    if (frame.node() && frame.node()->type().isListContainer()) {
        _openListLevel -= 1;
    }
}

auto HtmlParser::normalizeWhitespace(const String &text, bool &whitespaceOnly) -> String {
    auto reader = StringCharReader{text};
    auto result = StringEditor{};
    auto lastWasWhitespace = false;
    whitespaceOnly = true;
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (character.isAsciiWhitespace()) {
            if (!lastWasWhitespace) {
                result.append(U' ');
                lastWasWhitespace = true;
            }
            continue;
        }
        result.append(character);
        lastWasWhitespace = false;
        whitespaceOnly = false;
    }
    return result;
}

auto HtmlParser::toLowerAscii(const String &text) -> String {
    auto reader = StringCharReader{text};
    auto result = StringEditor{};
    while (!reader.isAtEnd()) {
        result.append(reader.read().toAsciiLowercase());
    }
    return result;
}

auto HtmlParser::findTagInfo(const String &tagName) -> const TagInfo * {
    for (const auto &[name, tagInfo] : tagInfoMap()) {
        if (name == tagName) {
            return &tagInfo;
        }
    }
    return nullptr;
}

auto HtmlParser::tagInfoMap() -> const TagInfoMap & {
    using NB = TagInfo::NodeBehavior;
    using LM = TagInfo::LevelMode;
    using SB = TagInfo::SubtreeBehavior;
    using TR = TagInfo::Transparency;

    static const auto cTagInfoMap = TagInfoMap{
        {"section"_el, TagInfo{TextNode::Type::Section, NB::Block}},
        {"div"_el, TagInfo{TextNode::Type::Section, NB::Block}},
        {"blockquote"_el, TagInfo{TextNode::Type::Blockquote, NB::Block}},
        {"p"_el, TagInfo{TextNode::Type::Paragraph, NB::Block}},
        {"span"_el, TagInfo{TextNode::Type::Span}},
        {"br"_el, TagInfo{TextNode::Type::LineBreak}},
        {"hr"_el, TagInfo{TextNode::Type::HorizontalLine, NB::Block}},
        {"ul"_el, TagInfo{TextNode::Type::BulletList, LM::UseListLevel, NB::Block}},
        {"ol"_el, TagInfo{TextNode::Type::NumberedList, LM::UseListLevel, NB::Block}},
        {"li"_el, TagInfo{TextNode::Type::None, NB::Block}},
        {"a"_el, TagInfo{TextNode::Type::Link}},
        {"b"_el, TagInfo{TextNode::Type::Strong}},
        {"strong"_el, TagInfo{TextNode::Type::Strong}},
        {"i"_el, TagInfo{TextNode::Type::Emphasis}},
        {"em"_el, TagInfo{TextNode::Type::Emphasis}},
        {"u"_el, TagInfo{TextNode::Type::Underline}},
        {"code"_el, TagInfo{TextNode::Type::Code}},
        {"pre"_el, TagInfo{TextNode::Type::CodeBlock, NB::Block}},
        {"h1"_el, TagInfo{TextNode::Type::Heading, 1, NB::Block}},
        {"h2"_el, TagInfo{TextNode::Type::Heading, 2, NB::Block}},
        {"h3"_el, TagInfo{TextNode::Type::Heading, 3, NB::Block}},
        {"h4"_el, TagInfo{TextNode::Type::Heading, 4, NB::Block}},
        {"h5"_el, TagInfo{TextNode::Type::Heading, 5, NB::Block}},
        {"h6"_el, TagInfo{TextNode::Type::Heading, 6, NB::Block}},
        {"dl"_el, TagInfo{TextNode::Type::DefinitionList, NB::Block}},
        {"dt"_el, TagInfo{TextNode::Type::DefinitionTerm, NB::Block}},
        {"dd"_el, TagInfo{TextNode::Type::DefinitionDescription, NB::Block}},
        {"img"_el, TagInfo{"image"_el}},
        {"table"_el, TagInfo{"table"_el, SB::Suppress}},
        {"form"_el, TagInfo{"form"_el, SB::Suppress}},
        {"svg"_el, TagInfo{"svg"_el, SB::Suppress}},
        {"head"_el, TagInfo{TR::Transparent, SB::Suppress}},
        {"title"_el, TagInfo{TR::Transparent, SB::Suppress}},
        {"script"_el, TagInfo{TR::Transparent, SB::Suppress}},
        {"style"_el, TagInfo{TR::Transparent, SB::Suppress}},
        {"link"_el, TagInfo{TR::Transparent, SB::Suppress}},
        {"meta"_el, TagInfo{TR::Transparent, SB::Suppress}},
        {"base"_el, TagInfo{TR::Transparent, SB::Suppress}},
        {"noscript"_el, TagInfo{TR::Transparent, SB::Suppress}},
        {"html"_el, TagInfo{TR::Transparent}},
        {"body"_el, TagInfo{TR::Transparent}},
        {"tr"_el, TagInfo{TR::Transparent}},
        {"td"_el, TagInfo{TR::Transparent}},
        {"th"_el, TagInfo{TR::Transparent}},
        {"thead"_el, TagInfo{TR::Transparent}},
        {"tbody"_el, TagInfo{TR::Transparent}},
        {"tfoot"_el, TagInfo{TR::Transparent}},
        {"caption"_el, TagInfo{TR::Transparent}},
        {"col"_el, TagInfo{TR::Transparent}},
        {"colgroup"_el, TagInfo{TR::Transparent}},
        {"input"_el, TagInfo{TR::Transparent}},
        {"label"_el, TagInfo{TR::Transparent}},
        {"select"_el, TagInfo{TR::Transparent}},
        {"option"_el, TagInfo{TR::Transparent}},
        {"textarea"_el, TagInfo{TR::Transparent}},
        {"button"_el, TagInfo{TR::Transparent}},
        {"fieldset"_el, TagInfo{TR::Transparent}},
        {"legend"_el, TagInfo{TR::Transparent}},
        {"optgroup"_el, TagInfo{TR::Transparent}},
        {"details"_el, TagInfo{TR::Transparent}},
        {"summary"_el, TagInfo{TR::Transparent}},
        {"figure"_el, TagInfo{TR::Transparent}},
        {"figcaption"_el, TagInfo{TR::Transparent}},
        {"mark"_el, TagInfo{TR::Transparent}},
        {"ruby"_el, TagInfo{TR::Transparent}},
        {"rt"_el, TagInfo{TR::Transparent}},
        {"rp"_el, TagInfo{TR::Transparent}},
        {"time"_el, TagInfo{TR::Transparent}},
        {"meter"_el, TagInfo{TR::Transparent}},
        {"progress"_el, TagInfo{TR::Transparent}},
        {"canvas"_el, TagInfo{TR::Transparent}},
        {"math"_el, TagInfo{TR::Transparent}},
        {"iframe"_el, TagInfo{TR::Transparent}},
        {"embed"_el, TagInfo{TR::Transparent}},
        {"object"_el, TagInfo{TR::Transparent}},
        {"video"_el, TagInfo{TR::Transparent}},
        {"audio"_el, TagInfo{TR::Transparent}},
        {"source"_el, TagInfo{TR::Transparent}},
        {"track"_el, TagInfo{TR::Transparent}},
        {"doctype"_el, TagInfo{TR::Transparent}},
    };
    return cTagInfoMap;
}

}
