// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HtmlAttribute.hpp"
#include "HtmlParserFrame.hpp"
#include "HtmlTagInfo.hpp"
#include "HtmlTokenizer.hpp"

#include "../../AnyString.hpp"
#include "../../String.hpp"
#include "../../TextDocument.hpp"
#include "../../TextNode.hpp"

#include <cstddef>
#include <utility>
#include <vector>

namespace erbsland::text::html::impl {

/// A tolerant HTML parser implementation.
/// @tested{HtmlParserTest}
class HtmlParser final {
public:
    /// Create a parser implementation for the given HTML text.
    /// @param html The HTML fragment or document to parse.
    explicit HtmlParser(AnyString html);

    // defaults
    ~HtmlParser() = default;
    HtmlParser(const HtmlParser &) = delete;
    HtmlParser(HtmlParser &&) = delete;
    auto operator=(const HtmlParser &) -> HtmlParser & = delete;
    auto operator=(HtmlParser &&) -> HtmlParser & = delete;

public:
    /// Parse the given HTML into a text document.
    /// @return The parsed document.
    /// @throws err::ParseError If a future unrecoverable parser condition is detected.
    [[nodiscard]] auto parse() -> TextDocument;

private:
    using Attribute = HtmlAttribute;
    using Attributes = std::vector<Attribute>;
    using Frame = HtmlParserFrame;
    using FrameList = std::vector<Frame>;
    using TagInfo = HtmlTagInfo;
    using TagInfoMap = std::vector<std::pair<String, TagInfo>>;
    using TokenGenerator = HtmlTokenizer::TokenGenerator;

private:
    void updateFrameStateForPush(const Frame &frame) noexcept;
    void updateFrameStateForPop(const Frame &frame) noexcept;
    void loadNextToken(TokenGenerator &generator);
    void advanceToken(TokenGenerator &generator);
    void handleText(String text);
    void handleOpenTag(String tagName, const Attributes &attributes);
    void handleCloseTag(const String &tagName);
    void pushNodeFrame(String tagName, TextNodePtr node, bool preserveWhitespace = false);
    void pushTransparentFrame(String tagName, bool suppressSubtree = false, bool preserveWhitespace = false);
    void closeFramesTo(std::size_t targetSize);
    void refreshCurrent() noexcept;
    void closeInlineFrames();
    void closeParagraphFrame();
    void closeCurrentListItem();
    void closeCurrentDefinitionEntry();
    void closeForBlockStart();
    void ensureImplicitListForItem();
    void ensureImplicitDefinitionList();
    void ensureTextContainer();
    void applyAttributes(const TextNodePtr &node, const String &tagName, const Attributes &attributes);
    [[nodiscard]] auto addNodeForTag(const String &tagName, TextNode::Level listLevel) -> TextNodePtr;
    [[nodiscard]] auto addListItemNode() -> TextNodePtr;
    [[nodiscard]] auto isSuppressed() const noexcept -> bool;
    [[nodiscard]] auto isPreservingWhitespace() const noexcept -> bool;
    [[nodiscard]] auto currentNodeType() const noexcept -> TextNode::Type;
    [[nodiscard]] auto currentNodeHasChildren() const noexcept -> bool;
    [[nodiscard]] auto countOpenListLevels() const noexcept -> TextNode::Level;

    [[nodiscard]] static auto normalizeWhitespace(const String &text, bool &whitespaceOnly) -> String;
    [[nodiscard]] static auto toLowerAscii(const String &text) -> String;
    [[nodiscard]] static auto findTagInfo(const String &tagName) -> const TagInfo *;
    [[nodiscard]] static auto tagInfoMap() -> const TagInfoMap &;

private:
    AnyString _html;                       ///< The parsed HTML text.
    HtmlToken _currentToken;               ///< The current token being processed.
    HtmlToken _nextToken;                  ///< The next token used for lookahead.
    TextDocument _document;                ///< The document being built.
    TextNodePtr _root;                     ///< The root document node.
    TextNodePtr _current;                  ///< The current node.
    FrameList _frames;                     ///< The open HTML element frames.
    int _suppressedFrameDepth{0};          ///< The number of frames that suppress their subtree.
    int _preservedWhitespaceFrameDepth{0}; ///< The number of frames that preserve whitespace.
    TextNode::Level _openListLevel{0};     ///< The number of open list container frames.
};

}
