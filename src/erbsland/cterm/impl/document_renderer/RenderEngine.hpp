// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockScope.hpp"
#include "InlineTextBuilder.hpp"
#include "RenderContext.hpp"

#include "../BlockStringBuilder.hpp"

#include "../../../text/StringEditor_fwd.hpp"
#include "../../../text/TextNode.hpp"
#include "../../TerminalDocumentStyle.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace erbsland::cterm::impl::document_renderer {

/// Convert a semantic text tree into width-aware logical document blocks.
/// @tested{TerminalDocumentRendererTest}
class RenderEngine final {
    /// Store a rendered term name and its layout properties.
    struct TermNameRenderData final {
        InlineContent text;                       ///< Rendered name content.
        bool allowsOptionFirstLineIndent{false};  ///< If the name allows an option indent.
        bool enablesOptionFirstLineIndent{false}; ///< If the name enables an option indent.
    };

    /// Store rendered data for one term-list item.
    struct TermItemRenderData final {
        text::TextNodePtr node;                         ///< Source term-item node.
        InlineContent name;                             ///< Rendered term name.
        int nameFirstLineIndent{0};                     ///< First-line indentation of the name.
        bool allowsOptionFirstLineIndent{false};        ///< If the name allows an option indent.
        bool enablesOptionFirstLineIndent{false};       ///< If the name enables an option indent.
        InlineContent description;                      ///< Rendered term description.
        ParagraphIndents indents;                       ///< Description paragraph indentation.
        std::vector<text::TextNodePtr> nestedTermLists; ///< Nested term-list nodes.
    };

    /// Describe the resolved layout of a term list.
    struct TermListLayout final {
        int descriptionStartColumn{0}; ///< Column where descriptions begin.
        bool stacked{false};           ///< If terms and descriptions use separate lines.
        bool fieldList{false};         ///< If the list has field-list styling.
    };

public:
    /// Create a render engine.
    /// @param style The document style to use.
    /// @param width The effective document width in terminal cells.
    RenderEngine(const TerminalDocumentStyle &style, int width) noexcept;
    /// Build logical blocks for one document node tree.
    /// @param node The root node to render.
    /// @return The prepared logical blocks.
    [[nodiscard]] auto build(const text::TextNode &node) -> std::vector<RenderBlock>;

    // defaults/deletions
    RenderEngine(const RenderEngine &) = delete;
    RenderEngine(RenderEngine &&) = delete;
    auto operator=(const RenderEngine &) -> RenderEngine & = delete;
    auto operator=(RenderEngine &&) -> RenderEngine & = delete;

private:
    /// Append blocks for a node and its descendants.
    void appendNode(const text::TextNode &node, const RenderContext &context);
    /// Append blocks for a container node.
    void appendContainer(const text::TextNode &node, const RenderContext &context);
    /// Append blocks for a code-snippet node.
    void appendCodeSnippet(const text::TextNode &node, const RenderContext &context);
    /// Append blocks for one code-snippet line.
    void appendCodeSnippetLine(const text::TextNode &line, const RenderContext &context);
    /// Create the gutter for a code-snippet line.
    [[nodiscard]] auto makeCodeSnippetGutter(const text::String &number, bool hasNumber, const RenderContext &context)
        -> BlockString;
    /// Append blocks for a list node.
    void appendList(const text::TextNode &node, text::TextNodeType itemType, const RenderContext &context);
    /// Append blocks for a list-item node.
    void appendListItem(
        const text::TextNode &node,
        const TerminalDocumentStyleRule &listItemRule,
        ListItemLayout listItemLayout,
        const RenderContext &context);
    /// Append a marker-free list item.
    void appendListItemWithoutMarker(const text::TextNode &node, const RenderContext &context);
    /// Append one child of a list item.
    void appendListItemChild(const text::TextNode &node, const RenderContext &context);
    /// Append a consecutive inline run from a list item.
    void appendListItemInlineRun(std::vector<text::TextNodePtr> &nodes, const RenderContext &context);
    /// Append blocks for a term list.
    void appendTermList(const text::TextNode &node, const RenderContext &context);
    /// Collect render data for the items in a term list.
    [[nodiscard]] auto collectTermItems(const text::TextNode &node, const RenderContext &context)
        -> std::vector<TermItemRenderData>;
    /// Determine the layout shared by term-list items.
    [[nodiscard]] auto termListLayout(const std::vector<TermItemRenderData> &items, bool fieldList) const noexcept
        -> TermListLayout;
    /// Append a term item using the normal term-list layout.
    void appendNormalTermItem(const TermItemRenderData &item, const TermListLayout &layout);
    /// Render a term name and determine its layout properties.
    [[nodiscard]] auto renderTermName(const text::TextNode &node, const RenderContext &context) -> TermNameRenderData;
    /// Append blocks for a paragraph-like node.
    void appendParagraphLikeNode(const text::TextNode &node, const RenderContext &context);
    /// Create a paragraph block from a text node.
    [[nodiscard]] auto paragraph(
        const text::TextNode &node, const TerminalDocumentStyleRule &rule, const RenderContext &context) -> RenderBlock;
    /// Create a paragraph block from styled block text.
    [[nodiscard]] auto paragraph(BlockString text, const TerminalDocumentStyleRule &rule, const RenderContext &context)
        -> RenderBlock;
    /// Create a paragraph block from rendered inline content.
    [[nodiscard]] auto paragraph(
        InlineContent content, const TerminalDocumentStyleRule &rule, const RenderContext &context) -> RenderBlock;
    /// Create a heading block from a text node.
    [[nodiscard]] auto heading(const text::TextNode &node, const RenderContext &context) -> RenderBlock;
    /// Create a horizontal-rule block.
    [[nodiscard]] auto horizontalRule(const RenderContext &context) -> RenderBlock;
    /// Create layout data for a numbered or bulleted list item.
    [[nodiscard]] auto makeListItemLayout(
        const TerminalDocumentStyleRule &listItemRule, const RenderContext &context, std::size_t number)
        -> ListItemLayout;
    /// Render a node's inline text with the requested style.
    [[nodiscard]] auto renderInlineText(const text::TextNode &node, BlockStyle style, bool preserveWhitespace)
        -> InlineContent;
    /// Append inline content for one node.
    void appendInlineNode(const text::TextNode &node, BlockStyle style, bool preserveWhitespace);
    /// Append inline content for a node's children.
    void appendInlineChildren(const text::TextNode &node, BlockStyle style, bool preserveWhitespace);
    /// Find the style rule for a node.
    [[nodiscard]] auto ruleFor(const text::TextNode &node) -> TerminalDocumentStyleRule;
    /// Find the style rule for a node at the specified level.
    [[nodiscard]] auto ruleFor(const text::TextNode &node, int level) -> TerminalDocumentStyleRule;
    /// Find the style rule matching a node type, level, and tokens.
    [[nodiscard]] auto ruleFor(
        text::TextNodeType nodeType, std::optional<int> level, const TerminalDocumentStyleSelector::TokenList &tokens)
        -> TerminalDocumentStyleRule;
    /// Emit a block into the active rendering scope.
    void emitBlock(RenderBlock block);
    /// Open a nested rendering scope.
    void openScope(
        block::Margins margins,
        std::optional<ListItemLayout> listItemLayout = std::nullopt,
        std::optional<BlockString> linePrefix = std::nullopt);
    /// Close the current rendering scope.
    void closeScope();
    /// Test whether the current list item already contains blocks.
    [[nodiscard]] auto currentListItemHasBlocks() const noexcept -> bool;
    /// Collapse one vertical margin of a block with its predecessor.
    void collapseVerticalMargin(RenderBlock &block, block::Margins::Side side, block::Coordinate margin);
    /// Emit the pending block, if any.
    void flushPendingBlock();
    /// Test whether a node type has a style level.
    [[nodiscard]] static auto usesLevel(text::TextNodeType nodeType) noexcept -> bool;
    /// Convert a non-negative block coordinate to an integer.
    [[nodiscard]] static auto positive(block::Coordinate value) noexcept -> int;
    /// Get the width available inside the document frame.
    [[nodiscard]] auto frameWidth() const noexcept -> int;
    /// Append plain text from a node and its descendants.
    static void appendNodeText(text::StringEditor &builder, const text::TextNode &node);
    /// Build plain text from a node and its descendants.
    [[nodiscard]] static auto nodeText(const text::TextNode &node) -> text::String;

private:
    static constexpr auto cLongOnlyOptionFirstLineIndent = 4;
    static constexpr auto cCodeLineNumberWidth = 4;

private:
    const TerminalDocumentStyle &_style;        ///< The style sheet to use.
    int _width{80};                             ///< Effective document width.
    InlineTextBuilder _inlineTextBuilder;       ///< Reusable inline text builder.
    BlockStringBuilder _blockBuilder;           ///< Reusable block text builder.
    BlockStringBuilder _decorationBuilder;      ///< Reusable decoration builder.
    std::vector<BlockScope> _scopes;            ///< Currently open rendering scopes.
    std::vector<text::TextNodeType> _ancestors; ///< Active node chain, including the current node.
    std::optional<RenderBlock> _pendingBlock;   ///< Pending block for margin collapse.
    std::vector<RenderBlock> _blocks;           ///< Prepared logical blocks.
};

}
