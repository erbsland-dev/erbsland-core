// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Renderer.hpp"

#include "DocumentLayout.hpp"
#include "RenderEngine.hpp"

#include "../TerminalDefaults.hpp"

namespace erbsland::cterm::impl::document_renderer {

using text::TextNode;

Renderer::Renderer(const TerminalDocumentStyle &style) noexcept : _style{style} {
}

void Renderer::renderTo(CursorWriter &writer, const TextNode &node) {
    for (const auto &line : layout(node, effectiveWidth(writer))) {
        writer.write(line);
        writer.writeLineBreak();
    }
}

void Renderer::appendPlainText(text::AnyStringBuilder &builder, const TextNode &node) const {
    auto pendingBreaks = unit::CpCount{};
    for (const auto &line : layout(node, cDefaultTerminalWidth)) {
        auto end = line.end();
        while (end != line.begin() && (end[-1] == U' ' || end[-1].isEmpty())) {
            --end;
        }
        if (end != line.begin()) {
            builder.append(text::Char{U'\n'}, pendingBreaks);
            pendingBreaks = {};
            for (auto it = line.begin(); it != end; ++it) {
                for (const auto character : it->characters()) {
                    if (character.isNull()) {
                        break;
                    }
                    builder.append(character);
                }
            }
        }
        pendingBreaks += unit::CpCount::one();
    }
}

auto Renderer::layout(const TextNode &node, const int width) const -> BlockStringLines {
    const auto blocks = RenderEngine{_style, width}.build(node);
    return DocumentLayout{width}.build(blocks);
}

auto Renderer::effectiveWidth(const CursorWriter &writer) noexcept -> int {
    const auto width = writer.size().width().toRawValue();
    if (width >= 60) {
        return width;
    }
    return cDefaultTerminalWidth;
}

}
