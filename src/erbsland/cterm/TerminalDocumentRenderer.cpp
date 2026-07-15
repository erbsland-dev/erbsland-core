// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TerminalDocumentRenderer.hpp"

#include "impl/document_renderer/Renderer.hpp"

#include <utility>

namespace erbsland::cterm {

TerminalDocumentRenderer::TerminalDocumentRenderer() : _style{TerminalDocumentStyle::defaultPlain()} {
}

TerminalDocumentRenderer::TerminalDocumentRenderer(TerminalDocumentStyle style) : _style{std::move(style)} {
}

void TerminalDocumentRenderer::setStyle(TerminalDocumentStyle style) noexcept {
    _style = std::move(style);
}

void TerminalDocumentRenderer::renderTo(CursorWriter &writer, const text::TextDocument &document) const {
    renderTo(writer, *document.root());
}

void TerminalDocumentRenderer::renderTo(CursorWriter &writer, const text::TextNode &node) const {
    auto renderer = impl::document_renderer::Renderer{_style};
    renderer.renderTo(writer, node);
}

}
