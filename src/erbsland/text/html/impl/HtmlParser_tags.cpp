// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HtmlParser.hpp"

#include "../../impl/LinkData.hpp"
#include "../../Literals.hpp"
#include "../../StringBuilder.hpp"
#include "../../StringCharReader.hpp"

#include <memory>
#include <utility>

namespace erbsland::text::html::impl {

using namespace literals;

void HtmlParser::applyAttributes(const TextNodePtr &node, const StringView &tagName, const Attributes &attributes) {
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

auto HtmlParser::addNodeForTag(const StringView &tagName, const TextNode::Level listLevel) -> TextNodePtr {
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

auto HtmlParser::normalizeWhitespace(StringView text, bool &whitespaceOnly) -> StringView {
    auto reader = StringCharReader{text};
    auto builder = StringBuilder{};
    auto lastWasWhitespace = false;
    whitespaceOnly = true;
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (character.isAsciiWhitespace()) {
            if (!lastWasWhitespace) {
                builder.append(U' ');
                lastWasWhitespace = true;
            }
            continue;
        }
        builder.append(character);
        lastWasWhitespace = false;
        whitespaceOnly = false;
    }
    return builder.takeString();
}

auto HtmlParser::toLowerAscii(StringView text) -> StringView {
    auto reader = StringCharReader{text};
    auto builder = StringBuilder{};
    while (!reader.isAtEnd()) {
        builder.append(reader.read().toAsciiLowercase());
    }
    return builder.takeString();
}

auto HtmlParser::findTagInfo(const StringView &tagName) -> const TagInfo * {
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
        {StringView{"section"_el}, TagInfo{TextNode::Type::Section, NB::Block}},
        {StringView{"div"_el}, TagInfo{TextNode::Type::Section, NB::Block}},
        {StringView{"blockquote"_el}, TagInfo{TextNode::Type::Blockquote, NB::Block}},
        {StringView{"p"_el}, TagInfo{TextNode::Type::Paragraph, NB::Block}},
        {StringView{"span"_el}, TagInfo{TextNode::Type::Span}},
        {StringView{"br"_el}, TagInfo{TextNode::Type::LineBreak}},
        {StringView{"hr"_el}, TagInfo{TextNode::Type::HorizontalLine, NB::Block}},
        {StringView{"ul"_el}, TagInfo{TextNode::Type::BulletList, LM::UseListLevel, NB::Block}},
        {StringView{"ol"_el}, TagInfo{TextNode::Type::NumberedList, LM::UseListLevel, NB::Block}},
        {StringView{"li"_el}, TagInfo{TextNode::Type::None, NB::Block}},
        {StringView{"a"_el}, TagInfo{TextNode::Type::Link}},
        {StringView{"b"_el}, TagInfo{TextNode::Type::Strong}},
        {StringView{"strong"_el}, TagInfo{TextNode::Type::Strong}},
        {StringView{"i"_el}, TagInfo{TextNode::Type::Emphasis}},
        {StringView{"em"_el}, TagInfo{TextNode::Type::Emphasis}},
        {StringView{"u"_el}, TagInfo{TextNode::Type::Underline}},
        {StringView{"code"_el}, TagInfo{TextNode::Type::Code}},
        {StringView{"pre"_el}, TagInfo{TextNode::Type::CodeBlock, NB::Block}},
        {StringView{"h1"_el}, TagInfo{TextNode::Type::Heading, 1, NB::Block}},
        {StringView{"h2"_el}, TagInfo{TextNode::Type::Heading, 2, NB::Block}},
        {StringView{"h3"_el}, TagInfo{TextNode::Type::Heading, 3, NB::Block}},
        {StringView{"h4"_el}, TagInfo{TextNode::Type::Heading, 4, NB::Block}},
        {StringView{"h5"_el}, TagInfo{TextNode::Type::Heading, 5, NB::Block}},
        {StringView{"h6"_el}, TagInfo{TextNode::Type::Heading, 6, NB::Block}},
        {StringView{"dl"_el}, TagInfo{TextNode::Type::DefinitionList, NB::Block}},
        {StringView{"dt"_el}, TagInfo{TextNode::Type::DefinitionTerm, NB::Block}},
        {StringView{"dd"_el}, TagInfo{TextNode::Type::DefinitionDescription, NB::Block}},
        {StringView{"img"_el}, TagInfo{"image"_el}},
        {StringView{"table"_el}, TagInfo{"table"_el, SB::Suppress}},
        {StringView{"form"_el}, TagInfo{"form"_el, SB::Suppress}},
        {StringView{"svg"_el}, TagInfo{"svg"_el, SB::Suppress}},
        {StringView{"head"_el}, TagInfo{TR::Transparent, SB::Suppress}},
        {StringView{"title"_el}, TagInfo{TR::Transparent, SB::Suppress}},
        {StringView{"script"_el}, TagInfo{TR::Transparent, SB::Suppress}},
        {StringView{"style"_el}, TagInfo{TR::Transparent, SB::Suppress}},
        {StringView{"link"_el}, TagInfo{TR::Transparent, SB::Suppress}},
        {StringView{"meta"_el}, TagInfo{TR::Transparent, SB::Suppress}},
        {StringView{"base"_el}, TagInfo{TR::Transparent, SB::Suppress}},
        {StringView{"noscript"_el}, TagInfo{TR::Transparent, SB::Suppress}},
        {StringView{"html"_el}, TagInfo{TR::Transparent}},
        {StringView{"body"_el}, TagInfo{TR::Transparent}},
        {StringView{"tr"_el}, TagInfo{TR::Transparent}},
        {StringView{"td"_el}, TagInfo{TR::Transparent}},
        {StringView{"th"_el}, TagInfo{TR::Transparent}},
        {StringView{"thead"_el}, TagInfo{TR::Transparent}},
        {StringView{"tbody"_el}, TagInfo{TR::Transparent}},
        {StringView{"tfoot"_el}, TagInfo{TR::Transparent}},
        {StringView{"caption"_el}, TagInfo{TR::Transparent}},
        {StringView{"col"_el}, TagInfo{TR::Transparent}},
        {StringView{"colgroup"_el}, TagInfo{TR::Transparent}},
        {StringView{"input"_el}, TagInfo{TR::Transparent}},
        {StringView{"label"_el}, TagInfo{TR::Transparent}},
        {StringView{"select"_el}, TagInfo{TR::Transparent}},
        {StringView{"option"_el}, TagInfo{TR::Transparent}},
        {StringView{"textarea"_el}, TagInfo{TR::Transparent}},
        {StringView{"button"_el}, TagInfo{TR::Transparent}},
        {StringView{"fieldset"_el}, TagInfo{TR::Transparent}},
        {StringView{"legend"_el}, TagInfo{TR::Transparent}},
        {StringView{"optgroup"_el}, TagInfo{TR::Transparent}},
        {StringView{"details"_el}, TagInfo{TR::Transparent}},
        {StringView{"summary"_el}, TagInfo{TR::Transparent}},
        {StringView{"figure"_el}, TagInfo{TR::Transparent}},
        {StringView{"figcaption"_el}, TagInfo{TR::Transparent}},
        {StringView{"mark"_el}, TagInfo{TR::Transparent}},
        {StringView{"ruby"_el}, TagInfo{TR::Transparent}},
        {StringView{"rt"_el}, TagInfo{TR::Transparent}},
        {StringView{"rp"_el}, TagInfo{TR::Transparent}},
        {StringView{"time"_el}, TagInfo{TR::Transparent}},
        {StringView{"meter"_el}, TagInfo{TR::Transparent}},
        {StringView{"progress"_el}, TagInfo{TR::Transparent}},
        {StringView{"canvas"_el}, TagInfo{TR::Transparent}},
        {StringView{"math"_el}, TagInfo{TR::Transparent}},
        {StringView{"iframe"_el}, TagInfo{TR::Transparent}},
        {StringView{"embed"_el}, TagInfo{TR::Transparent}},
        {StringView{"object"_el}, TagInfo{TR::Transparent}},
        {StringView{"video"_el}, TagInfo{TR::Transparent}},
        {StringView{"audio"_el}, TagInfo{TR::Transparent}},
        {StringView{"source"_el}, TagInfo{TR::Transparent}},
        {StringView{"track"_el}, TagInfo{TR::Transparent}},
        {StringView{"doctype"_el}, TagInfo{TR::Transparent}},
    };
    return cTagInfoMap;
}

}
