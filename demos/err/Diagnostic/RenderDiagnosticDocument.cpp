// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/all_cterm.hpp>

namespace demo {

/// Diagnostic documents are renderer-neutral `TextDocument` trees.
/// The same document can become plain text for logs or styled terminal output for an interactive application.
void renderDiagnosticDocument() {
    auto context = el::ApplicationErrorContext{
        "The score could not be read."_el,
        "The tempo-marking value is out of range."_el,
    };
    context.setSourcePath("scores/朝の合奏.music"_el)
        .setCodeLocation(el::CodeLocation{.line = el::LineIndex{6}, .column = el::ColumnIndex{14}});
    const auto error = el::ApplicationError{std::move(context), {}};
    const auto document = el::DiagnosticHelper{error}.toDocument();

    el::io::printLine("--- Plain text ---"_el);
    el::io::printLine(document.toString());
    el::io::printLine("--- Terminal document ---"_el);
    const auto renderer = el::cterm::TerminalDocumentRenderer{el::application().systemOutputStyle()};
    renderer.renderTo(*el::application().terminal(), document);
}

}
