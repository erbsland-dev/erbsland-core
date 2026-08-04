// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HtmlAttribute.hpp"
#include "HtmlParser_fwd.hpp"
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
    /// The parsed form of an HTML attribute.
    using Attribute = HtmlAttribute;
    /// The attributes of one HTML tag.
    using Attributes = std::vector<Attribute>;
    /// State retained for one open HTML element.
    using Frame = HtmlParserFrame;
    /// The stack of open HTML element frames.
    using FrameList = std::vector<Frame>;
    /// Metadata that describes an HTML tag.
    using TagInfo = HtmlTagInfo;
    /// The tag metadata indexed by lowercase tag name.
    using TagInfoMap = std::vector<std::pair<String, TagInfo>>;
    /// The generator that yields HTML tokens.
    using TokenGenerator = HtmlTokenizer::TokenGenerator;

private:
    /// Update parser state after pushing `frame`.
    void updateFrameStateForPush(const Frame &frame) noexcept;
    /// Update parser state before popping `frame`.
    void updateFrameStateForPop(const Frame &frame) noexcept;
    /// Load the next token from `generator`.
    void loadNextToken(TokenGenerator &generator);
    /// Advance the current token using `generator`.
    void advanceToken(TokenGenerator &generator);
    /// Add normalized text to the current document node.
    void handleText(String text);
    /// Process an opening tag and its attributes.
    void handleOpenTag(String tagName, const Attributes &attributes);
    /// Process a closing tag.
    void handleCloseTag(const String &tagName);
    /// Push a document node frame for `tagName`.
    void pushNodeFrame(String tagName, TextNodePtr node, bool preserveWhitespace = false);
    /// Push a frame that does not create a document node.
    void pushTransparentFrame(String tagName, bool suppressSubtree = false, bool preserveWhitespace = false);
    /// Close frames until the stack contains `targetSize` frames.
    void closeFramesTo(std::size_t targetSize);
    /// Refresh the current node from the top frame.
    void refreshCurrent() noexcept;
    /// Close all currently open inline frames.
    void closeInlineFrames();
    /// Close the currently open paragraph frame.
    void closeParagraphFrame();
    /// Close the currently open list item.
    void closeCurrentListItem();
    /// Close the currently open definition entry.
    void closeCurrentDefinitionEntry();
    /// Close frames that cannot contain a new block.
    void closeForBlockStart();
    /// Create an implied list before adding a list item.
    void ensureImplicitListForItem();
    /// Create an implied definition list when required.
    void ensureImplicitDefinitionList();
    /// Ensure that text has a valid current container.
    void ensureTextContainer();
    /// Apply supported HTML attributes to `node`.
    void applyAttributes(const TextNodePtr &node, const String &tagName, const Attributes &attributes);
    /// Create and add the document node represented by `tagName`.
    [[nodiscard]] auto addNodeForTag(const String &tagName, TextNode::Level listLevel) -> TextNodePtr;
    /// Create and add one list item node.
    [[nodiscard]] auto addListItemNode() -> TextNodePtr;
    /// Test whether the current subtree is suppressed.
    [[nodiscard]] auto isSuppressed() const noexcept -> bool;
    /// Test whether the current frame preserves whitespace.
    [[nodiscard]] auto isPreservingWhitespace() const noexcept -> bool;
    /// Get the current node type, or the default root type.
    [[nodiscard]] auto currentNodeType() const noexcept -> TextNode::Type;
    /// Test whether the current node already has children.
    [[nodiscard]] auto currentNodeHasChildren() const noexcept -> bool;
    /// Count the open list container levels.
    [[nodiscard]] auto countOpenListLevels() const noexcept -> TextNode::Level;

    /// Collapse runs of whitespace and report whether the input contained only whitespace.
    [[nodiscard]] static auto normalizeWhitespace(const String &text, bool &whitespaceOnly) -> String;
    /// Convert ASCII letters in `text` to lowercase.
    [[nodiscard]] static auto toLowerAscii(const String &text) -> String;
    /// Find metadata for `tagName`.
    [[nodiscard]] static auto findTagInfo(const String &tagName) -> const TagInfo *;
    /// Access the parser's known HTML tag metadata.
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
