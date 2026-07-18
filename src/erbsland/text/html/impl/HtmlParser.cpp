// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HtmlParser.hpp"

#include "../../impl/LinkData.hpp"
#include "../../Literals.hpp"
#include "../../StringCharReader.hpp"

#include <memory>
#include <utility>

namespace erbsland::text::html::impl {

using namespace literals;

HtmlParser::HtmlParser(AnyString html) : _html{std::move(html)} {
}

auto HtmlParser::parse() -> TextDocument {
    _document = TextDocument{};
    _root = _document.root();
    _current = _root;
    _currentToken = {};
    _nextToken = {};
    _frames.clear();
    _suppressedFrameDepth = 0;
    _preservedWhitespaceFrameDepth = 0;
    _openListLevel = 0;

    auto tokenizer = HtmlTokenizer{_html};
    auto generator = tokenizer.tokenize();
    loadNextToken(generator);
    while (_nextToken.type != HtmlTokenType::End) {
        advanceToken(generator);
        switch (_currentToken.type) {
        case HtmlTokenType::Text:
            handleText(std::move(_currentToken.value));
            break;
        case HtmlTokenType::TagOpen: {
            auto tagName = toLowerAscii(_currentToken.value);
            auto attributes = std::move(_currentToken.attributes);
            for (auto &attribute : attributes) {
                attribute.setName(toLowerAscii(attribute.name()));
            }
            const auto selfClosing = _currentToken.selfClosing;
            if (selfClosing) {
                auto closeTagName = tagName;
                handleOpenTag(std::move(tagName), attributes);
                handleCloseTag(closeTagName);
            } else {
                handleOpenTag(std::move(tagName), attributes);
            }
            break;
        }
        case HtmlTokenType::TagClose:
            handleCloseTag(toLowerAscii(_currentToken.value));
            break;
        case HtmlTokenType::End:
        case HtmlTokenType::Comment:
        case HtmlTokenType::DocType:
            break;
        }
    }
    closeFramesTo(0);
    return std::move(_document);
}

void HtmlParser::loadNextToken(TokenGenerator &generator) {
    if (auto token = generator.next()) {
        _nextToken = std::move(*token);
        return;
    }
    _nextToken = {};
}

void HtmlParser::advanceToken(TokenGenerator &generator) {
    swap(_currentToken, _nextToken);
    loadNextToken(generator);
}

void HtmlParser::handleText(String text) {
    if (isSuppressed()) {
        return;
    }

    const auto preserveWhitespace = isPreservingWhitespace() || currentNodeType() == TextNode::Type::CodeBlock;
    auto whitespaceOnly = false;
    if (!preserveWhitespace) {
        text = normalizeWhitespace(std::move(text), whitespaceOnly);
    }
    if (text.isEmpty()) {
        return;
    }
    if (!preserveWhitespace && whitespaceOnly) {
        if (!currentNodeType().isInline()) {
            if (!currentNodeHasChildren()) {
                return;
            }
            if (_nextToken.type == HtmlTokenType::TagClose) {
                return;
            }
            const auto *nextTagInfo =
                _nextToken.type == HtmlTokenType::TagOpen ? findTagInfo(toLowerAscii(_nextToken.value)) : nullptr;
            if (nextTagInfo != nullptr && nextTagInfo->isBlock()) {
                return;
            }
        }
    }

    ensureTextContainer();
    _current->addText(std::move(text));
}

void HtmlParser::handleOpenTag(String tagName, const Attributes &attributes) {
    if (isSuppressed()) {
        pushTransparentFrame(std::move(tagName), true);
        return;
    }
    const auto *tagInfo = findTagInfo(tagName);
    const auto isBlock = tagInfo != nullptr && tagInfo->isBlock();
    const auto isPlaceholder = tagInfo != nullptr && !tagInfo->placeholderText().isEmpty();
    const auto isTransparent = tagInfo != nullptr && tagInfo->transparent();
    const auto suppressSubtree = tagInfo != nullptr && tagInfo->suppressSubtree();

    if (tagName == "li"_el) {
        closeCurrentListItem();
        ensureImplicitListForItem();
        closeForBlockStart();
        auto node = addListItemNode();
        applyAttributes(node, tagName, attributes);
        pushNodeFrame(std::move(tagName), std::move(node));
        return;
    }
    if (tagName == "dt"_el || tagName == "dd"_el) {
        closeCurrentDefinitionEntry();
        ensureImplicitDefinitionList();
        closeForBlockStart();
    } else if (isBlock) {
        closeForBlockStart();
    } else if (!isPlaceholder && !suppressSubtree && !isTransparent) {
        ensureTextContainer();
    }

    if (isPlaceholder) {
        auto node = _current->addUnsupported(tagInfo->placeholderText());
        applyAttributes(node, tagName, attributes);
        if (suppressSubtree) {
            pushTransparentFrame(std::move(tagName), true);
        }
        return;
    }
    if (suppressSubtree) {
        pushTransparentFrame(std::move(tagName), true);
        return;
    }
    if (isTransparent) {
        pushTransparentFrame(std::move(tagName));
        return;
    }

    auto node = addNodeForTag(tagName, countOpenListLevels());
    if (!node) {
        pushTransparentFrame(std::move(tagName));
        return;
    }
    applyAttributes(node, tagName, attributes);

    if (node->type() == TextNode::Type::LineBreak || node->type() == TextNode::Type::HorizontalLine) {
        return;
    }
    const auto preserveWhitespace = tagName == "pre"_el;
    pushNodeFrame(std::move(tagName), std::move(node), preserveWhitespace);
}

void HtmlParser::handleCloseTag(const String &tagName) {
    for (auto index = _frames.size(); index > 0; --index) {
        if (_frames[index - 1].tagName() == tagName) {
            closeFramesTo(index - 1);
            return;
        }
    }
}

void HtmlParser::pushNodeFrame(String tagName, TextNodePtr node, const bool preserveWhitespace) {
    _frames.push_back(Frame{std::move(tagName), std::move(node), false, false, preserveWhitespace});
    updateFrameStateForPush(_frames.back());
    refreshCurrent();
}

void HtmlParser::pushTransparentFrame(String tagName, const bool suppressSubtree, const bool preserveWhitespace) {
    _frames.push_back(Frame{std::move(tagName), {}, true, suppressSubtree, preserveWhitespace});
    updateFrameStateForPush(_frames.back());
}

void HtmlParser::closeFramesTo(const std::size_t targetSize) {
    while (_frames.size() > targetSize) {
        updateFrameStateForPop(_frames.back());
        _frames.pop_back();
    }
    refreshCurrent();
}

void HtmlParser::refreshCurrent() noexcept {
    _current = _root;
    for (auto index = _frames.size(); index > 0; --index) {
        if (_frames[index - 1].node()) {
            _current = _frames[index - 1].node();
            return;
        }
    }
}

void HtmlParser::closeInlineFrames() {
    auto targetSize = _frames.size();
    while (targetSize > 0) {
        const auto &frame = _frames[targetSize - 1];
        if (!frame.node()) {
            targetSize -= 1;
            continue;
        }
        if (frame.node()->type().isInline()) {
            targetSize -= 1;
            continue;
        }
        break;
    }
    closeFramesTo(targetSize);
}

void HtmlParser::closeParagraphFrame() {
    closeInlineFrames();
    if (!_frames.empty() && _frames.back().node() && _frames.back().node()->type() == TextNode::Type::Paragraph) {
        closeFramesTo(_frames.size() - 1);
    }
}

void HtmlParser::closeCurrentListItem() {
    closeParagraphFrame();
    closeInlineFrames();
    if (!_frames.empty() && _frames.back().node() && _frames.back().node()->type().isListItem()) {
        closeFramesTo(_frames.size() - 1);
    }
}

void HtmlParser::closeCurrentDefinitionEntry() {
    closeParagraphFrame();
    closeInlineFrames();
    if (_frames.empty() || !_frames.back().node()) {
        return;
    }
    const auto type = _frames.back().node()->type();
    if (type == TextNode::Type::DefinitionTerm || type == TextNode::Type::DefinitionDescription) {
        closeFramesTo(_frames.size() - 1);
    }
}

void HtmlParser::closeForBlockStart() {
    closeParagraphFrame();
    closeInlineFrames();
}

void HtmlParser::ensureImplicitListForItem() {
    closeInlineFrames();
    if (currentNodeType().isListContainer()) {
        return;
    }
    pushNodeFrame("ul"_el, _current->addBulletList(countOpenListLevels()));
}

void HtmlParser::ensureImplicitDefinitionList() {
    closeInlineFrames();
    if (currentNodeType() == TextNode::Type::DefinitionList) {
        return;
    }
    pushNodeFrame("dl"_el, _current->addDefinitionList());
}

void HtmlParser::ensureTextContainer() {
    while (true) {
        const auto type = currentNodeType();
        if (type.isTextContainer()) {
            return;
        }
        if (type.isListContainer()) {
            pushNodeFrame("li"_el, addListItemNode());
            continue;
        }
        if (type == TextNode::Type::DefinitionList) {
            pushNodeFrame("dd"_el, _current->addDefinitionDescription());
            continue;
        }
        pushNodeFrame("p"_el, _current->addParagraph());
        return;
    }
}

}
