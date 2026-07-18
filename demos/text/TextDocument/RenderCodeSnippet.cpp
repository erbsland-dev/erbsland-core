// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/all_cterm.hpp>

namespace demo {

namespace {
const auto cConfigurationLines = el::StringList{{
    "# A configuration file written in the Erbsland Configuration Language."_el,
    "--[ Clang Format ]----------------------------------------------------------------------------------------------------"_el,
    "# Marker file used to remember when clang-format last processed all files."_el,
    "Marker File    : \".clang-format-last-run\""_el,
    ""_el,
    "# The next line contains an intentionally long line, to see it's rendering on small terminals."_el,
    "Intervals : 1 hour, 2 hours, 10 hours, 12 hours, 16 hours, 21 hours, 26 hours, 28 hours, 29 hours, 31 hours, 32 hours, 34 hours, 45 hours, $49 hours, 57 hours"_el,
    ""_el,
    "# The next list has indentations. These need to be preserved."_el,
    "Extensions     :"_el,
    "    * \".cpp\""_el,
    "    * \".hpp\""_el,
    "    * \".tpp\""_el,
    ""_el,
}};
}

/// `TextNode` can insert a semantic code snippet into structured text.
/// Renderers add the line-number gutter, crop unmarked context and wrap marked lines while preserving the exact
/// diagnostic location.
void renderCodeSnippet() {
    el::TextDocument document;
    document.addHeading(1)->addText("Code Snippet"_el);
    document.addCodeSnippet(
        cConfigurationLines.slice({el::ElementIndex{0}, el::ElementIndex{12}}),
        el::LineIndex{0},
        el::CodeSnippetMarkerList{
            el::CodeSnippetMarker{
                el::LineIndex{3},
                el::ColumnIndex{18},
                el::ColumnCount{22},
                "Here!"_el,
                "error"_el,
            },
            el::CodeSnippetMarker{
                el::LineIndex{6},
                el::ColumnIndex{139},
                el::ColumnCount{0},
                "Error"_el,
                "error"_el,
            },
        });

    el::stdOut()->printLine("--- Styled Terminal Output ---"_el);

    const auto renderer = el::cterm::TerminalDocumentRenderer{el::application().systemOutputStyle()};
    renderer.renderTo(*el::application().terminal(), document);

    el::stdOut()->printLine("--- Plain Text Output ---"_el);
    el::stdOut()->printLine(document.toString());
}

}
