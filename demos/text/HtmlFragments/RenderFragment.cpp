// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/cterm/TerminalDocumentRenderer.hpp>
#include <erbsland/text/html/HtmlParser.hpp>

namespace demo {

/// Parse a small HTML fragment into a text document and render it with terminal styles.
///
/// `HtmlParser` accepts markup embedded in ordinary text and produces a semantic `TextDocument`. The same document
/// can be rendered with different `TerminalDocumentStyle` presets without parsing the HTML again.
void renderFragment() {
    // Parse the description of a synthesizer patch. The Turkish words are input data, not parser commands.
    const auto html =
        u8"<h2>Yankı</h2><p>A <strong>bright</strong> patch with <em>soft</em> echoes &amp; a long tail.</p>"_el;
    const auto document = el::html::HtmlParser{html}.parse();

    // Render the same document using a restrained and a more decorative terminal style.
    el::io::printLine("Simple style:"_el);
    el::cterm::TerminalDocumentRenderer{el::cterm::TerminalDocumentStyle::defaultSimple()}.renderTo(
        *el::application().terminal(), document);
    el::io::printLine("Styled style:"_el);
    el::cterm::TerminalDocumentRenderer{el::cterm::TerminalDocumentStyle::defaultStyled()}.renderTo(
        *el::application().terminal(), document);
}

}
