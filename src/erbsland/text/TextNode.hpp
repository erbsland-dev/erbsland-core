// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CodeSnippetMarker.hpp"
#include "EscapeAmount.hpp"
#include "EscapeFormat.hpp"
#include "String.hpp"
#include "StringList.hpp"
#include "StringTree.hpp"
#include "TextNode_fwd.hpp"
#include "TextNodeData_fwd.hpp"
#include "TextNodeType.hpp"
#include "TextWalkResult.hpp"
#include "TextWalkStatus.hpp"

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace erbsland::text {

/// A mutable node in a text document tree.
/// @tested{TextDocumentTest}
class TextNode : public std::enable_shared_from_this<TextNode> {
    struct PrivateTag {};

public:
    using Type = TextNodeType; ///< The node type wrapper.
    using Level = int;         ///< Heading or nesting level.

public:
    // defaults
    ~TextNode() = default;
    TextNode(const TextNode &) = delete;
    TextNode(TextNode &&) = delete;
    auto operator=(const TextNode &) -> TextNode & = delete;
    auto operator=(TextNode &&) -> TextNode & = delete;

public:
    /// Create a node with the given type.
    /// @param type The node type.
    /// @return The created node.
    [[nodiscard]] static auto create(Type type) -> TextNodePtr;
    /// Create a document root node.
    [[nodiscard]] static auto createDocument() -> TextNodePtr;
    /// Create a paragraph node.
    [[nodiscard]] static auto createParagraph() -> TextNodePtr;
    /// Create a section node.
    [[nodiscard]] static auto createSection() -> TextNodePtr;
    /// Create a blockquote node.
    [[nodiscard]] static auto createBlockquote() -> TextNodePtr;
    /// Create an explicit line-break node.
    [[nodiscard]] static auto createLineBreak() -> TextNodePtr;
    /// Create a heading node.
    /// @param level The heading level.
    [[nodiscard]] static auto createHeading(Level level) -> TextNodePtr;
    /// Create a bullet-list node.
    /// @param level The nesting level.
    [[nodiscard]] static auto createBulletList(Level level) -> TextNodePtr;
    /// Create a numbered-list node.
    /// @param level The nesting level.
    [[nodiscard]] static auto createNumberedList(Level level) -> TextNodePtr;
    /// Create a bullet-list-item node.
    [[nodiscard]] static auto createBulletListItem() -> TextNodePtr;
    /// Create a numbered-list-item node.
    [[nodiscard]] static auto createNumberedListItem() -> TextNodePtr;
    /// Create a definition-list node.
    [[nodiscard]] static auto createDefinitionList() -> TextNodePtr;
    /// Create a definition-term node.
    [[nodiscard]] static auto createDefinitionTerm() -> TextNodePtr;
    /// Create a definition-description node.
    [[nodiscard]] static auto createDefinitionDescription() -> TextNodePtr;
    /// Create a code-block node.
    /// @param language The optional language identifier.
    [[nodiscard]] static auto createCodeBlock(String language = {}) -> TextNodePtr;
    /// Create a line-oriented code snippet node.
    /// @param lines The source lines to include.
    /// @param startLine The original zero-based line index of the first line, or no-index for no line numbers.
    /// @param markers Optional marker ranges.
    /// @param language The optional language identifier.
    [[nodiscard]] static auto createCodeSnippet(
        StringList lines,
        unit::LineIndex startLine = unit::LineIndex::zero(),
        CodeSnippetMarkerList markers = {},
        String language = {}) -> TextNodePtr;
    /// Create a horizontal-line node.
    [[nodiscard]] static auto createHorizontalLine() -> TextNodePtr;
    /// Create a plain text node.
    /// @param text The text content.
    [[nodiscard]] static auto createText(String text) -> TextNodePtr;
    /// Create an emphasis node.
    [[nodiscard]] static auto createEmphasis() -> TextNodePtr;
    /// Create a strong-emphasis node.
    [[nodiscard]] static auto createStrong() -> TextNodePtr;
    /// Create an underline node.
    [[nodiscard]] static auto createUnderline() -> TextNodePtr;
    /// Create a generic span node.
    [[nodiscard]] static auto createSpan() -> TextNodePtr;
    /// Create a link node.
    /// @param url The link target.
    [[nodiscard]] static auto createLink(String url = {}) -> TextNodePtr;
    /// Create an inline code node.
    [[nodiscard]] static auto createCode() -> TextNodePtr;
    /// Create an unsupported-content node.
    /// @param text The preserved content text.
    [[nodiscard]] static auto createUnsupported(String text = {}) -> TextNodePtr;
    /// Create an error-content node.
    /// @param text The preserved error text.
    [[nodiscard]] static auto createError(String text = {}) -> TextNodePtr;

public: // modifiers
    /// Add a child node with the given type.
    /// @param type The child node type.
    /// @return The added child node.
    auto add(Type type) -> TextNodePtr;
    /// Add an existing detached child node.
    /// @param child The child node to add. Must not be null.
    /// @return The added child node.
    auto add(TextNodePtr child) -> TextNodePtr;
    /// Add a paragraph child.
    auto addParagraph() -> TextNodePtr;
    /// Add a section child.
    auto addSection() -> TextNodePtr;
    /// Add a blockquote child.
    auto addBlockquote() -> TextNodePtr;
    /// Add an explicit line-break child.
    auto addLineBreak() -> TextNodePtr;
    /// Add a heading child.
    /// @param level The heading level.
    auto addHeading(Level level) -> TextNodePtr;
    /// Add a bullet-list child.
    /// @param level The nesting level.
    auto addBulletList(Level level = 0) -> TextNodePtr;
    /// Add a numbered-list child.
    /// @param level The nesting level.
    auto addNumberedList(Level level = 0) -> TextNodePtr;
    /// Add a list-item child matching this list node type.
    auto addListItem() -> TextNodePtr;
    /// Add a bullet-list-item child.
    auto addBulletListItem() -> TextNodePtr;
    /// Add a numbered-list-item child.
    auto addNumberedListItem() -> TextNodePtr;
    /// Add a definition-list child.
    auto addDefinitionList() -> TextNodePtr;
    /// Add a definition-term child.
    auto addDefinitionTerm() -> TextNodePtr;
    /// Add a definition-description child.
    auto addDefinitionDescription() -> TextNodePtr;
    /// Add a code-block child.
    /// @param language The optional language identifier.
    auto addCodeBlock(String language = {}) -> TextNodePtr;
    /// Add a line-oriented code snippet child.
    /// @param lines The source lines to include.
    /// @param startLine The original zero-based line index of the first line, or no-index for no line numbers.
    /// @param markers Optional marker ranges.
    /// @param language The optional language identifier.
    auto addCodeSnippet(
        StringList lines,
        unit::LineIndex startLine = unit::LineIndex::zero(),
        CodeSnippetMarkerList markers = {},
        String language = {}) -> TextNodePtr;
    /// Add a horizontal-line child.
    auto addHorizontalLine() -> TextNodePtr;
    /// Add a plain-text child.
    /// @param text The text content.
    auto addText(String text) -> TextNodePtr;
    /// Add safely escaped text as plain-text and escape-sequence children.
    /// @param text The potentially unsafe text to append.
    /// @param format The escape format to use.
    /// @param amount The amount of text to escape.
    /// @return This node.
    auto addEscapedText(const String &text, EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced)
        -> TextNode &;
    /// Add an emphasis child.
    auto addEmphasis() -> TextNodePtr;
    /// Add a strong-emphasis child.
    auto addStrong() -> TextNodePtr;
    /// Add an underline child.
    auto addUnderline() -> TextNodePtr;
    /// Add a generic span child.
    auto addSpan() -> TextNodePtr;
    /// Add a link child.
    /// @param url The link target.
    auto addLink(String url = {}) -> TextNodePtr;
    /// Add an inline-code child.
    auto addCode() -> TextNodePtr;
    /// Add an unsupported-content child.
    /// @param text The preserved content text.
    auto addUnsupported(String text = {}) -> TextNodePtr;
    /// Add an error-content child.
    /// @param text The preserved error text.
    auto addError(String text = {}) -> TextNodePtr;

public: // setters
    /// Replace the text payload.
    auto setText(String text) noexcept -> TextNode &;
    /// Replace the identifier.
    auto setIdentifier(String identifier) noexcept -> TextNode &;
    /// Replace the style/class information.
    auto setStyle(String style) noexcept -> TextNode &;
    /// Replace the node-specific metadata.
    auto setData(TextNodeDataPtr data) noexcept -> TextNode &;
    /// Replace the heading or nesting level.
    auto setLevel(Level level) noexcept -> TextNode &;

public: // accessors
    /// Get the node type.
    [[nodiscard]] auto type() const noexcept -> Type { return _type; }
    /// Test if this node has child nodes.
    [[nodiscard]] auto hasChildren() const noexcept -> bool;
    /// Access the child nodes.
    /// A text node guarantees that this list never contains null pointers. Child pointers can only be added through
    /// the node API, and null children are rejected before they can enter the tree.
    [[nodiscard]] auto children() const noexcept -> const TextNodeList & { return _children; }
    /// Test if this node has a parent.
    [[nodiscard]] auto hasParent() const noexcept -> bool;
    /// Access the parent node, or an empty pointer for roots.
    [[nodiscard]] auto parent() const noexcept -> TextNodePtr;
    /// Access the text payload.
    [[nodiscard]] auto text() const noexcept -> String { return _text; }
    /// Access the identifier.
    [[nodiscard]] auto identifier() const noexcept -> String { return _identifier; }
    /// Access the style/class information.
    [[nodiscard]] auto style() const noexcept -> String { return _style; }
    /// Access the optional node-specific metadata.
    [[nodiscard]] auto data() const noexcept -> const TextNodeDataPtr & { return _data; }
    /// Access the heading or nesting level.
    [[nodiscard]] auto level() const noexcept -> Level { return _level; }

public:
    /// Walk this node and all descendants in pre-order.
    /// @param nodeFn The function called for every node.
    /// @return The final walk result.
    template <typename Fn>
        requires(
            std::invocable<Fn, const TextNode &> &&
            std::same_as<std::invoke_result_t<Fn, const TextNode &>, TextWalkStatus>)
    auto walk(Fn nodeFn) const -> TextWalkResult;

    /// Test if any node in this tree matches a predicate.
    /// @param nodeFn The predicate called for every node.
    /// @return `true` if any node matches.
    template <typename Fn>
        requires(
            std::invocable<Fn, const TextNode &> &&
            std::convertible_to<std::invoke_result_t<Fn, const TextNode &>, bool>)
    [[nodiscard]] auto anyOf(Fn nodeFn) const -> bool;
    /// Test if this node tree contains a node with the given type.
    /// @param type The node type to search for.
    /// @return `true` if this node or any descendant has the given type.
    [[nodiscard]] auto contains(Type type) const -> bool;

public: // conversion
    /// Create a deep copy of this node and all descendants.
    /// @return The cloned node tree.
    [[nodiscard]] auto clone() const -> TextNodePtr;
    /// Convert this node tree into a diagnostic tree.
    [[nodiscard]] auto toDiagnosticTree() const -> StringTree;

public: // private ctor
    /// @internal
    /// Create a node with explicit metadata.
    TextNode(Type type, String text, String identifier, String style, TextNodeDataPtr data, Level level, PrivateTag);

private:
    [[nodiscard]] static auto createNode(
        Type type,
        String text = {},
        String identifier = {},
        String style = {},
        TextNodeDataPtr data = {},
        Level level = 0) -> TextNodePtr;
    [[nodiscard]] static auto codeSnippetLineIndex(unit::LineIndex startLine, std::size_t localLine) noexcept
        -> unit::LineIndex;
    auto appendChild(TextNodePtr child) -> TextNodePtr;

private:
    Type _type;              ///< The semantic node type.
    TextNodeList _children;  ///< The child nodes.
    TextNodeWeakPtr _parent; ///< The parent node.
    String _text;            ///< Text payload.
    String _identifier;      ///< Optional identifier/anchor.
    String _style;           ///< Optional style/class information.
    TextNodeDataPtr _data;   ///< Node-specific extensible metadata.
    Level _level{0};         ///< Heading or nesting level.
};

}

#include "TextNode.tpp"
