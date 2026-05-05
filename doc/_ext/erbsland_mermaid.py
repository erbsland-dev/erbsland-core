#  Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
#  SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import html

from docutils import nodes
from docutils.parsers.rst import Directive
from sphinx.application import Sphinx

_MERMAID_CONFIG = """---
config:
    theme: default
    fontFamily: Lato, proxima-nova, "Helvetica Neue", Arial, sans-serif
---"""


def _build_mermaid_html(diagram_source: str) -> str:
    """Build the HTML block for a Mermaid diagram."""
    content = f"{_MERMAID_CONFIG}\n{diagram_source.strip()}"
    escaped_content = html.escape(content, quote=False)
    return f'<div class="mermaid">\n{escaped_content}\n</div>'


class MermaidDirective(Directive):
    """Render Mermaid diagrams in HTML output."""

    has_content = True

    def run(self) -> list[nodes.raw]:
        """Convert the directive body into an HTML Mermaid container."""
        self.assert_has_content()
        diagram_source = "\n".join(self.content)
        return [nodes.raw("", _build_mermaid_html(diagram_source), format="html")]


def setup(app: Sphinx) -> dict[str, bool]:
    """Register the Mermaid directive and initialization assets."""
    app.add_directive("mermaid", MermaidDirective)
    app.add_js_file("mermaid-init.js")
    return {
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
