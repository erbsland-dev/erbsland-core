#  Copyright (c) 2024-2025 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
#  SPDX-License-Identifier: Apache-2.0
from docutils import nodes
from sphinx.application import Sphinx
from sphinx.util.docutils import SphinxDirective


class SpecialAdmonitionDirective(SphinxDirective):
    """
    A special admonition directive that uses an additional class for styling.
    """

    has_content = True

    ADMONITION_CLASS = "example"
    ADMONITION_TITLE = "Example"

    def run(self):
        admonition_node = nodes.admonition()
        admonition_node["classes"].append(self.ADMONITION_CLASS)
        title_node = nodes.title(text=self.ADMONITION_TITLE)
        admonition_node += title_node
        self.state.nested_parse(self.content, self.content_offset, admonition_node)
        return [admonition_node]


class DesignRationaleDirective(SpecialAdmonitionDirective):
    """Directive to create a 'Design Rationale' admonition."""

    ADMONITION_CLASS = "design-rationale"
    ADMONITION_TITLE = "Design Rationale"


def setup(app: Sphinx):
    app.add_directive("design-rationale", DesignRationaleDirective)

    return {
        "version": "1.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
