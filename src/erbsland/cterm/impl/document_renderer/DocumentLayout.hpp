// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RenderBlock.hpp"

#include "../BlockStringBuilder.hpp"

#include "../../BlockStringEditor.hpp"
#include "../../CursorBuffer.hpp"
#include "../../ParagraphOptions.hpp"

#include <vector>

namespace erbsland::cterm::impl::document_renderer {

/// Materialize logical document blocks into exact physical terminal lines.
///
/// Paragraph content is first laid out at its available width. Frames and child margins are composed around each
/// completed content line afterwards, so decorations never affect content tokenization or tab resolution.
/// @tested{TerminalDocumentRendererTest}
class DocumentLayout final {
public:
    /// Create a document layout for an effective width.
    /// @param width The width in terminal cells. Values below one are treated as one.
    explicit DocumentLayout(int width);

    // defaults/deletions
    DocumentLayout(const DocumentLayout &) = delete;
    DocumentLayout(DocumentLayout &&) = delete;
    auto operator=(const DocumentLayout &) -> DocumentLayout & = delete;
    auto operator=(DocumentLayout &&) -> DocumentLayout & = delete;

public:
    /// Materialize all logical blocks.
    /// @param blocks The blocks created by `RenderEngine`.
    /// @return Styled physical lines without trailing buffer fill cells.
    [[nodiscard]] auto build(const std::vector<RenderBlock> &blocks) -> BlockStringLines;

private:
    /// Append one logical block as physical terminal lines.
    void appendBlock(const RenderBlock &block);
    /// Append a paragraph block with its frame and margins.
    void appendParagraph(const RenderBlock &block);
    /// Append a preformatted block with its frame and margins.
    void appendPreformatted(const RenderBlock &block);
    /// Append a block that consists of a filled line.
    void appendFilledLine(const RenderBlock &block);
    /// Append a horizontal-rule block.
    void appendHorizontalRule(const RenderBlock &block);
    /// Append empty framed lines for a vertical margin.
    void appendGap(int count, const BlockString &prefix);
    /// Append one content line with its frame decoration.
    void appendContentLine(const RenderBlock &block, const BlockString &content);
    /// Get paragraph options derived from a render block.
    [[nodiscard]] auto paragraphOptions(const RenderBlock &block) const -> ParagraphOptions;
    /// Get the width available for a block's content.
    [[nodiscard]] auto availableContentWidth(const RenderBlock &block) const noexcept -> int;
    /// Copy a buffer line without its trailing fill cells.
    [[nodiscard]] static auto trimmedLine(const CursorBuffer &buffer, int y) -> BlockString;
    /// Convert a block coordinate to a non-negative cell count.
    [[nodiscard]] static auto positive(bgeo::BlockCoordinate value) noexcept -> int;

private:
    int _width;
    BlockStringLines _lines;
    BlockStringBuilder _builder;
    BlockString _previousFramePrefix;
    int _previousBottomMargin{0};
    bool _firstBlock{true};
};

}
