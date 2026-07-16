#  Copyright (c) 2024-2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
#  SPDX-License-Identifier: Apache-2.0
import re

from docutils import nodes
from sphinx.application import Sphinx
from sphinx.util.docutils import SphinxDirective


RE_PARAM_IN_TEXT = re.compile(R"^(.*?)<([^<>]+)>(.*?)$")
RE_PARAM_PATTERN = re.compile(R"<([^<>]+?)>")


def esc_code(name, rawtext, text, lineno, inliner, options=None, content=None):
    match = RE_PARAM_IN_TEXT.match(text)
    if match:
        prefix = match.group(1)
        parameter = match.group(2)
        suffix = match.group(3)
        result_nodes = []
        if prefix:
            result_nodes.append(nodes.inline(text=f"\\{prefix}", classes=["esc-expression"]))
        else:
            result_nodes.append(nodes.inline(text="\\", classes=["esc-expression"]))
        result_nodes.append(nodes.emphasis(text=parameter, classes=["esc-parameter"]))
        if suffix:
            result_nodes.append(nodes.inline(text=suffix, classes=["esc-expression"]))
    else:
        result_nodes = [nodes.inline(text=f"\\{text}", classes=["esc-expression"])]
    return result_nodes, []


def expression(name, rawtext, text, lineno, inliner, options=None, content=None):
    result_nodes = []
    last_end = 0

    for match in RE_PARAM_PATTERN.finditer(text):
        # Add text before the parameter
        if match.start() > last_end:
            result_nodes.append(nodes.inline(text=text[last_end : match.start()], classes=["esc-expression"]))

        # Add the parameter
        result_nodes.append(nodes.emphasis(text=match.group(1), classes=["esc-parameter"]))
        last_end = match.end()

    # Add remaining text after the last parameter
    if last_end < len(text):
        result_nodes.append(nodes.inline(text=text[last_end:], classes=["esc-expression"]))

    # If no parameters were found, treat entire text as expression
    if not result_nodes:
        result_nodes = [nodes.inline(text=text, classes=["esc-expression"])]

    return result_nodes, []


def unicode_codepoint(name, rawtext, text, lineno, inliner, options=None, content=None):
    code = int(text, 16)
    return [nodes.inline(text=f"U+{code:04X}", classes=["unicode-codepoint"])], []


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
    app.add_role("esc_code", esc_code)
    app.add_role("expression", expression)
    app.add_role("unicode", unicode_codepoint)
    app.add_directive("design-rationale", DesignRationaleDirective)

    return {
        "version": "1.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
