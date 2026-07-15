// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Renderer.hpp"

#include "DocumentLayout.hpp"
#include "RenderEngine.hpp"

namespace erbsland::cterm::impl::document_renderer {

Renderer::Renderer(const TerminalDocumentStyle &style) noexcept : _style{style} {
}

void Renderer::renderTo(CursorWriter &writer, const text::TextNode &node) {
    const auto width = effectiveWidth(writer);
    auto blocks = RenderEngine{_style, width}.build(node);
    auto lines = DocumentLayout{width}.build(blocks);
    for (const auto &line : lines) {
        writer.write(line);
        writer.writeLineBreak();
    }
}

auto Renderer::effectiveWidth(const CursorWriter &writer) noexcept -> int {
    const auto width = writer.size().width().toRawValue();
    return width >= 60 ? width : 80;
}

}
