// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PlainTextRenderer_fwd.hpp"

#include "../String.hpp"
#include "../StringBuilder.hpp"
#include "../StringView.hpp"
#include "../TextDocument_fwd.hpp"
#include "../TextNode_fwd.hpp"

#include "../../unit/CpLength.hpp"

#include <cstddef>

namespace erbsland::text::impl {

/// Stateful implementation of plain-text document rendering.
/// @tested{TextDocumentTest}
class PlainTextRenderer final {
public:
    explicit PlainTextRenderer(const TextDocument &document) noexcept;

public:
    [[nodiscard]] auto build() -> String;
    auto appendTo(StringBuilder &builder) -> StringBuilder &;

private:
    void resetRenderState() noexcept;
    void renderNode(StringBuilder &builder, const TextNodePtr &node);
    void renderChildren(StringBuilder &builder, const TextNode &node);
    void renderCodeSnippet(StringBuilder &builder, const TextNode &node);
    void renderList(StringBuilder &builder, const TextNode &node, bool numbered);
    void renderListItem(StringBuilder &builder, const TextNode &node, StringView prefix);
    void renderTermList(StringBuilder &builder, const TextNode &node);
    void renderTermItem(StringBuilder &builder, const TextNode &node, unit::CpLength descriptionColumn);
    [[nodiscard]] static auto termName(const TextNode &node) -> String;
    [[nodiscard]] static auto termDescription(const TextNode &node) -> String;
    [[nodiscard]] static auto termListDescriptionColumn(const TextNode &node) -> unit::CpLength;
    static void appendInline(StringBuilder &builder, const TextNode &node);
    static void appendInlineChildren(StringBuilder &builder, const TextNode &node);
    static void appendPlaceholder(StringBuilder &builder, const TextNode &node, StringView prefix, StringView suffix);
    void appendLine(StringBuilder &builder, const StringView &prefix, const StringView &text);
    void appendWrappedLine(
        StringBuilder &builder, const StringView &prefix, const StringView &continuation, const StringView &text);
    static void appendIndent(StringBuilder &builder, unit::CpLength indent);
    static void appendNodeText(StringBuilder &builder, const TextNode &node);
    [[nodiscard]] static auto nodeText(const TextNode &node) -> String;
    [[nodiscard]] static auto listPrefix(std::size_t index, bool numbered) -> String;
    [[nodiscard]] static auto isNestedBlock(const TextNode &node) noexcept -> bool;
    [[nodiscard]] static auto isDiagnosticSectionHeading(const TextNode &node) noexcept -> bool;
    [[nodiscard]] auto isRootDiagnosticContent(const TextNode &node) const noexcept -> bool;

private:
    static constexpr auto cPlainTermListWidth = unit::CpLength{80U};
    static constexpr auto cPlainCodeSnippetWidth = 80;
    static constexpr auto cCodeLineNumberWidth = 4;
    static constexpr auto cTermDescriptionMinimumColumn = unit::CpLength{12U};
    static constexpr auto cTermDescriptionMaximumColumn = unit::CpLength{26U};

    const TextDocument &_document; ///< The rendered document.
    bool _firstLine{true};         ///< Set while no line was rendered yet.
    bool _diagnosticDocument{};    ///< Set when rendering an error diagnostic document.
    unit::CpLength _indent;        ///< The current block indentation.
};

}
