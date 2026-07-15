// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RenderBlock.hpp"

#include "../BlockStringBuilder.hpp"

#include "../../BlockString.hpp"
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

    DocumentLayout(const DocumentLayout &) = delete;
    DocumentLayout(DocumentLayout &&) = delete;
    auto operator=(const DocumentLayout &) -> DocumentLayout & = delete;
    auto operator=(DocumentLayout &&) -> DocumentLayout & = delete;

public:
    /// Materialize all logical blocks.
    /// @param blocks The blocks created by `RenderEngine`.
    /// @return Styled physical lines without trailing buffer fill cells.
    [[nodiscard]] auto build(const std::vector<RenderBlock> &blocks) -> std::vector<BlockString>;

private:
    void appendBlock(const RenderBlock &block);
    void appendParagraph(const RenderBlock &block);
    void appendPreformatted(const RenderBlock &block);
    void appendFilledLine(const RenderBlock &block);
    void appendHorizontalRule(const RenderBlock &block);
    void appendGap(int count, const BlockString &prefix);
    void appendContentLine(const RenderBlock &block, const BlockStringView &content);
    [[nodiscard]] auto paragraphOptions(const RenderBlock &block) const -> ParagraphOptions;
    [[nodiscard]] auto availableContentWidth(const RenderBlock &block) const noexcept -> int;
    [[nodiscard]] static auto trimmedLine(const CursorBuffer &buffer, int y) -> BlockString;
    [[nodiscard]] static auto positive(bgeo::BlockCoordinate value) noexcept -> int;

private:
    int _width;
    std::vector<BlockString> _lines;
    BlockStringBuilder _builder;
    BlockString _previousFramePrefix;
    int _previousBottomMargin{0};
    bool _firstBlock{true};
};

}
