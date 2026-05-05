# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from docutils import nodes
from docutils.parsers.rst import Directive, directives
from sphinx.application import Sphinx


class ErbslandDemoDirective(Directive):
    """Mark a documentation demo block managed by the demo-doc utility."""

    has_content = True
    optional_arguments = 0
    final_argument_whitespace = False
    option_spec = {
        "source": directives.unchanged_required,
        "exec": directives.unchanged,
        "source-sha256": directives.unchanged,
    }

    def run(self) -> list[nodes.Node]:
        """Produce no visible output."""
        return []


class ErbslandDemoEndDirective(Directive):
    """Mark the end of a documentation demo block."""

    has_content = False

    def run(self) -> list[nodes.Node]:
        """Produce no visible output."""
        return []


def setup(app: Sphinx) -> dict[str, bool]:
    """Register demo marker directives."""
    app.add_directive("erbsland-demo", ErbslandDemoDirective)
    app.add_directive("erbsland-demo-end", ErbslandDemoEndDirective)
    return {
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
