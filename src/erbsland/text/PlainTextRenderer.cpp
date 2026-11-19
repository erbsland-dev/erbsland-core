// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PlainTextRenderer.hpp"

#include "TextDocument.hpp"

#include "../cterm/impl/document_renderer/Renderer.hpp"

namespace erbsland::text {

PlainTextRenderer::PlainTextRenderer(const TextDocument &document) : _document{document} {
}

auto PlainTextRenderer::build() -> String {
    auto builder = AnyStringBuilder{};
    appendTo(builder);
    return builder.toString();
}

auto PlainTextRenderer::appendTo(AnyStringBuilder &builder) -> AnyStringBuilder & {
    cterm::impl::document_renderer::Renderer{cterm::TerminalDocumentStyle::defaultSystemOutput()}.appendPlainText(
        builder, *_document.get().root());
    return builder;
}

}
