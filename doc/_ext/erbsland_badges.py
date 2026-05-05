#  Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
#  SPDX-License-Identifier: Apache-2.0

from docutils import nodes
from docutils.parsers.rst import roles


def el_tested_role(name, rawtext, text, lineno, inliner, options=None, content=None):
    """Role for :el-tested: - renders a success test icon with hover text."""
    if options is None:
        options = {}
    html = f'<span title="{text}"><i class="fa-solid fa-vial-circle-check sd-text-success"></i></span>'
    node = nodes.raw("", html, format="html")
    return [node], []


def el_needtest_role(name, rawtext, text, lineno, inliner, options=None, content=None):
    """Role for :el-needtest: - renders a warning test icon with hover text."""
    if options is None:
        options = {}
    html = f'<span title="{text}"><i class="fa-solid fa-vial-vertical sd-text-warning"></i></span>'
    node = nodes.raw("", html, format="html")
    return [node], []


def el_notest_role(name, rawtext, text, lineno, inliner, options=None, content=None):
    """Role for :el-notest: - renders a muted test icon with hover text."""
    if options is None:
        options = {}
    html = f'<span title="{text}"><i class="fa-solid fa-vial-vertical sd-text-muted"></i></span>'
    node = nodes.raw("", html, format="html")
    return [node], []


def el_unicode_db_role(name, rawtext, text, lineno, inliner, options=None, content=None):
    """Role for :el-unicode-db: - renders unicode database icon with hover text."""
    if options is None:
        options = {}
    if not text:
        text = "Links the Unicode database"
    html = f'<span title="$text"><i class="fa-solid fa-symbols sd-text-info"></i></span>'
    node = nodes.raw("", html, format="html")
    return [node], []


def setup(app):
    """Setup function for the Sphinx extension."""
    roles.register_local_role("el-tested", el_tested_role)
    roles.register_local_role("el-needtest", el_needtest_role)
    roles.register_local_role("el-notest", el_notest_role)
    roles.register_local_role("el-unicode-db", el_unicode_db_role)

    return {
        "version": "0.1",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
