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
    struct TermNameRenderData final {
        InlineContent text;
        bool allowsOptionFirstLineIndent{false};
        bool enablesOptionFirstLineIndent{false};
    };

    struct TermItemRenderData final {
        text::TextNodePtr node;
        InlineContent name;
        int nameFirstLineIndent{0};
        bool allowsOptionFirstLineIndent{false};
        bool enablesOptionFirstLineIndent{false};
        InlineContent description;
        ParagraphIndents indents;
        std::vector<text::TextNodePtr> nestedTermLists;
    };

    struct TermListLayout final {
        int descriptionStartColumn{0};
        bool stacked{false};
        bool fieldList{false};
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

    RenderEngine(const RenderEngine &) = delete;
    RenderEngine(RenderEngine &&) = delete;
    auto operator=(const RenderEngine &) -> RenderEngine & = delete;
    auto operator=(RenderEngine &&) -> RenderEngine & = delete;

private:
    void appendNode(const text::TextNode &node, const RenderContext &context);
    void appendContainer(const text::TextNode &node, const RenderContext &context);
    void appendCodeSnippet(const text::TextNode &node, const RenderContext &context);
    void appendCodeSnippetLine(const text::TextNode &line, const RenderContext &context);
    [[nodiscard]] auto makeCodeSnippetGutter(const text::String &number, bool hasNumber, const RenderContext &context)
        -> BlockString;
    void appendList(const text::TextNode &node, text::TextNodeType itemType, const RenderContext &context);
    void appendListItem(
        const text::TextNode &node,
        const TerminalDocumentStyleRule &listItemRule,
        ListItemLayout listItemLayout,
        const RenderContext &context);
    void appendListItemWithoutMarker(const text::TextNode &node, const RenderContext &context);
    void appendListItemChild(const text::TextNode &node, const RenderContext &context);
    void appendListItemInlineRun(std::vector<text::TextNodePtr> &nodes, const RenderContext &context);
    void appendTermList(const text::TextNode &node, const RenderContext &context);
    [[nodiscard]] auto collectTermItems(const text::TextNode &node, const RenderContext &context)
        -> std::vector<TermItemRenderData>;
    [[nodiscard]] auto termListLayout(const std::vector<TermItemRenderData> &items, bool fieldList) const noexcept
        -> TermListLayout;
    void appendNormalTermItem(const TermItemRenderData &item, const TermListLayout &layout);
    [[nodiscard]] auto renderTermName(const text::TextNode &node, const RenderContext &context) -> TermNameRenderData;
    void appendParagraphLikeNode(const text::TextNode &node, const RenderContext &context);
    [[nodiscard]] auto paragraph(
        const text::TextNode &node, const TerminalDocumentStyleRule &rule, const RenderContext &context) -> RenderBlock;
    [[nodiscard]] auto paragraph(BlockString text, const TerminalDocumentStyleRule &rule, const RenderContext &context)
        -> RenderBlock;
    [[nodiscard]] auto paragraph(
        InlineContent content, const TerminalDocumentStyleRule &rule, const RenderContext &context) -> RenderBlock;
    [[nodiscard]] auto heading(const text::TextNode &node, const RenderContext &context) -> RenderBlock;
    [[nodiscard]] auto horizontalRule(const RenderContext &context) -> RenderBlock;
    [[nodiscard]] auto makeListItemLayout(
        const TerminalDocumentStyleRule &listItemRule, const RenderContext &context, std::size_t number)
        -> ListItemLayout;
    [[nodiscard]] auto renderInlineText(const text::TextNode &node, BlockStyle style, bool preserveWhitespace)
        -> InlineContent;
    void appendInlineNode(const text::TextNode &node, BlockStyle style, bool preserveWhitespace);
    void appendInlineChildren(const text::TextNode &node, BlockStyle style, bool preserveWhitespace);
    [[nodiscard]] auto ruleFor(const text::TextNode &node) -> TerminalDocumentStyleRule;
    [[nodiscard]] auto ruleFor(const text::TextNode &node, int level) -> TerminalDocumentStyleRule;
    [[nodiscard]] auto ruleFor(
        text::TextNodeType nodeType, std::optional<int> level, const TerminalDocumentStyleSelector::TokenList &tokens)
        -> TerminalDocumentStyleRule;
    void emitBlock(RenderBlock block);
    void openScope(
        bgeo::BlockMargins margins,
        std::optional<ListItemLayout> listItemLayout = std::nullopt,
        std::optional<BlockString> linePrefix = std::nullopt);
    void closeScope();
    [[nodiscard]] auto currentListItemHasBlocks() const noexcept -> bool;
    void collapseVerticalMargin(RenderBlock &block, bgeo::BlockMargins::Side side, bgeo::BlockCoordinate margin);
    void flushPendingBlock();
    [[nodiscard]] static auto usesLevel(text::TextNodeType nodeType) noexcept -> bool;
    [[nodiscard]] static auto positive(bgeo::BlockCoordinate value) noexcept -> int;
    [[nodiscard]] auto frameWidth() const noexcept -> int;
    static void appendNodeText(text::StringEditor &builder, const text::TextNode &node);
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
