# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import html

from docutils import nodes
from docutils.parsers.rst import Directive
from sphinx.application import Sphinx

_DRAFT_ICON = '<i class="fa-solid fa-file-pen" aria-hidden="true"></i>'
_DEFAULT_DRAFT_MESSAGE = "This page is a draft."


def _build_draft_html(optional_text: str) -> str:
    """Build the HTML notice for a draft page."""
    message = _DEFAULT_DRAFT_MESSAGE
    if optional_text:
        message = f"{message} {optional_text}"
    return (
        f'<div class="erbsland-draft" role="status">{_DRAFT_ICON}'
        f"<span>{html.escape(message)}</span>{_DRAFT_ICON}</div>"
    )


class ErbslandDraftDirective(Directive):
    """Render a sticky notice that marks a documentation page as a draft."""

    has_content = False
    optional_arguments = 1
    final_argument_whitespace = True

    def run(self) -> list[nodes.raw]:
        """Render the draft notice for HTML output."""
        optional_text = self.arguments[0] if self.arguments else ""
        return [nodes.raw("", _build_draft_html(optional_text), format="html")]


def setup(app: Sphinx) -> dict[str, bool]:
    """Register the draft notice directive."""
    app.add_directive("erbsland-draft", ErbslandDraftDirective)
    return {
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
