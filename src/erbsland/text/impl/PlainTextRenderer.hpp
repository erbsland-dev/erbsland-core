// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PlainTextRenderer_fwd.hpp"

#include "../AnyStringBuilder.hpp"
#include "../String.hpp"
#include "../StringEditor.hpp"
#include "../TextDocument_fwd.hpp"
#include "../TextNode_fwd.hpp"

#include "../../unit/CpLength.hpp"

#include <cstddef>

namespace erbsland::text::impl {

/// Stateful implementation of plain-text document rendering.
/// @tested{TextDocumentTest}
class PlainTextRenderer final {
public:
    /// Create a renderer for a document.
    explicit PlainTextRenderer(const TextDocument &document) noexcept;

public:
    /// Render the document to a new string.
    [[nodiscard]] auto build() -> String;
    /// Append the rendered document to a builder.
    auto appendTo(AnyStringBuilder &builder) -> AnyStringBuilder &;

private:
    /// Reset the state used while rendering a document.
    void resetRenderState() noexcept;
    /// Render one document node.
    void renderNode(AnyStringBuilder &builder, const TextNodePtr &node);
    /// Render the children of one document node.
    void renderChildren(AnyStringBuilder &builder, const TextNode &node);
    /// Render a code-snippet node.
    void renderCodeSnippet(AnyStringBuilder &builder, const TextNode &node);
    /// Render an ordered or unordered list node.
    void renderList(AnyStringBuilder &builder, const TextNode &node, bool numbered);
    /// Render one list item with its prefix.
    void renderListItem(AnyStringBuilder &builder, const TextNode &node, const String &prefix);
    /// Render a term-list node.
    void renderTermList(AnyStringBuilder &builder, const TextNode &node);
    /// Render one term-list item at a description column.
    void renderTermItem(AnyStringBuilder &builder, const TextNode &node, unit::CpLength descriptionColumn);
    /// Get the term name from a term-list item.
    [[nodiscard]] static auto termName(const TextNode &node) -> String;
    /// Get the term description from a term-list item.
    [[nodiscard]] static auto termDescription(const TextNode &node) -> String;
    /// Determine the description column for a term list.
    [[nodiscard]] static auto termListDescriptionColumn(const TextNode &node) -> unit::CpLength;
    /// Append one node's inline representation.
    static void appendInline(AnyStringBuilder &builder, const TextNode &node);
    /// Append the inline representations of one node's children.
    static void appendInlineChildren(AnyStringBuilder &builder, const TextNode &node);
    /// Append a placeholder for a node with a prefix and suffix.
    static void appendPlaceholder(
        AnyStringBuilder &builder, const TextNode &node, const String &prefix, const String &suffix);
    /// Append one unwrapped line with its prefix.
    void appendLine(AnyStringBuilder &builder, const String &prefix, const String &text);
    /// Append a line wrapped using a continuation prefix.
    void appendWrappedLine(
        AnyStringBuilder &builder, const String &prefix, const String &continuation, const String &text);
    /// Append the requested indentation.
    static void appendIndent(AnyStringBuilder &builder, unit::CpLength indent);
    /// Append the textual content of one node.
    static void appendNodeText(AnyStringBuilder &builder, const TextNode &node);
    /// Get the textual content of one node.
    [[nodiscard]] static auto nodeText(const TextNode &node) -> String;
    /// Build a list-item prefix.
    [[nodiscard]] static auto listPrefix(std::size_t index, bool numbered) -> String;
    /// Test whether a node is nested within a block node.
    [[nodiscard]] static auto isNestedBlock(const TextNode &node) noexcept -> bool;
    /// Test whether a node is a diagnostic section heading.
    [[nodiscard]] static auto isDiagnosticSectionHeading(const TextNode &node) noexcept -> bool;
    /// Test whether a node is root-level diagnostic content.
    [[nodiscard]] auto isRootDiagnosticContent(const TextNode &node) const noexcept -> bool;

private:
    static constexpr auto cPlainTermListWidth = unit::CpLength{80U}; ///< The target width for term lists.
    static constexpr auto cPlainCodeSnippetWidth = 80;               ///< The target width for code snippets.
    static constexpr auto cCodeLineNumberWidth = 4;                  ///< The width reserved for code line numbers.
    static constexpr auto cTermDescriptionMinimumColumn = unit::CpLength{12U}; ///< The minimum term description column.
    static constexpr auto cTermDescriptionMaximumColumn = unit::CpLength{26U}; ///< The maximum term description column.

    const TextDocument &_document;                                             ///< The rendered document.
    bool _firstLine{true};                                                     ///< Set while no line was rendered yet.
    bool _diagnosticDocument{}; ///< Set when rendering an error diagnostic document.
    unit::CpLength _indent;     ///< The current block indentation.
};

}
