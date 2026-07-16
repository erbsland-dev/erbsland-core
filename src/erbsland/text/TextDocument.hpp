// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CodeSnippetMarker.hpp"
#include "String.hpp"
#include "StringView.hpp"
#include "StringViewList.hpp"
#include "TextDocument_fwd.hpp"
#include "TextNode.hpp"

namespace erbsland::text {

/// A mutable text document with a valid document root node.
/// @tested{TextDocumentTest}
class TextDocument final {
public:
    /// Create an empty text document.
    TextDocument();

    // defaults
    ~TextDocument() = default;
    TextDocument(const TextDocument &) = delete;
    TextDocument(TextDocument &&) noexcept = default;
    auto operator=(const TextDocument &) -> TextDocument & = delete;
    auto operator=(TextDocument &&) noexcept -> TextDocument & = default;

public:
    /// Add a child node with the given type to the document root.
    auto add(TextNodeType type) -> TextNodePtr;
    /// Add a paragraph to the document root.
    auto addParagraph() -> TextNodePtr;
    /// Add a section to the document root.
    auto addSection() -> TextNodePtr;
    /// Add a blockquote to the document root.
    auto addBlockquote() -> TextNodePtr;
    /// Add an explicit line break to the document root.
    auto addLineBreak() -> TextNodePtr;
    /// Add a heading to the document root.
    /// @param level The heading level.
    auto addHeading(TextNode::Level level) -> TextNodePtr;
    /// Add a bullet list to the document root.
    /// @param level The nesting level.
    auto addBulletList(TextNode::Level level = 0) -> TextNodePtr;
    /// Add a numbered list to the document root.
    /// @param level The nesting level.
    auto addNumberedList(TextNode::Level level = 0) -> TextNodePtr;
    /// Add a definition list to the document root.
    auto addDefinitionList() -> TextNodePtr;
    /// Add a code block to the document root.
    /// @param language The optional language identifier.
    auto addCodeBlock(StringView language = {}) -> TextNodePtr;
    /// Add a line-oriented code snippet to the document root.
    /// @param lines The source lines to include.
    /// @param startLine The original zero-based line index of the first line, or no-index for no line numbers.
    /// @param markers Optional marker ranges.
    /// @param language The optional language identifier.
    auto addCodeSnippet(
        StringViewList lines,
        unit::LineIndex startLine = unit::LineIndex::zero(),
        CodeSnippetMarkerList markers = {},
        StringView language = {}) -> TextNodePtr;
    /// Add a horizontal line to the document root.
    auto addHorizontalLine() -> TextNodePtr;
    /// Add plain text to the document root.
    /// @param text The text content.
    auto addText(StringView text) -> TextNodePtr;
    /// Add an unsupported-content block to the document root.
    /// @param text The preserved content text.
    auto addUnsupported(StringView text = {}) -> TextNodePtr;
    /// Add an error-content block to the document root.
    /// @param text The preserved error text.
    auto addError(StringView text = {}) -> TextNodePtr;

public: // accessors
    /// Test if the document root has no children.
    [[nodiscard]] auto isEmpty() const noexcept -> bool;
    /// Access the document root node.
    [[nodiscard]] auto root() noexcept -> TextNodePtr { return _root; }
    /// Access the document root node.
    [[nodiscard]] auto root() const noexcept -> TextNodePtr { return _root; }

public: // conversion
    /// Render the document tree as plain UTF-8 text.
    [[nodiscard]] auto toString() const -> String;

private:
    TextNodePtr _root; ///< The document root node.
};

}
