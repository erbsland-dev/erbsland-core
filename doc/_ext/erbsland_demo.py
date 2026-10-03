#  Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
#  SPDX-License-Identifier: Apache-2.0

"""Present synchronized demo blocks without executing or rewriting their contents."""

from __future__ import annotations

import hashlib
from pathlib import Path

from docutils import nodes
from docutils.parsers.rst import Directive, directives
from sphinx.application import Sphinx
from sphinx.transforms import SphinxTransform
from sphinx.util import logging

logger = logging.getLogger(__name__)
MAX_SOURCE_SIZE = 1024 * 1024


class DemoStart(nodes.General, nodes.Element):
    pass


class DemoEnd(nodes.General, nodes.Element):
    pass


class ErbslandDemoDirective(Directive):
    """Mark a synchronized excerpt and its source."""

    has_content = False
    option_spec = {
        "source": directives.unchanged_required,
        "files": directives.unchanged_required,
        "show": directives.unchanged_required,
        "hide": directives.unchanged_required,
        "sha256": directives.unchanged,
        "exec": directives.unchanged,
        "exec-exit-code": directives.nonnegative_int,
        "show-cmd-line": directives.flag,
        **{f"exec-{index}": directives.unchanged for index in range(2, 100)},
        **{f"exec-{index}-exit-code": directives.nonnegative_int for index in range(2, 100)},
    }

    def run(self):
        node = DemoStart(options=dict(self.options))
        node.source, node.line = self.state_machine.get_source_and_line(self.lineno)
        return [node]


class ErbslandDemoEndDirective(Directive):
    has_content = False

    def run(self):
        return [DemoEnd()]


class ErbslandDemoRunDirective(Directive):
    """Group one command and its captured output using ordinary document nodes."""

    has_content = True

    def run(self):
        node = nodes.container(classes=["erbsland-demo-run"])
        self.state.nested_parse(self.content, self.content_offset, node)
        if node.children and isinstance(node[0], nodes.literal_block) and node[0].get("language") == "console":
            node[0]["classes"].append("demo-command")
        else:
            node.insert(0, nodes.paragraph("", "Output", classes=["demo-command"]))
        return [node]


def read_source(root: Path, name: str) -> tuple[Path, str]:
    """Restrict source expansion to small regular UTF-8 files inside demos."""
    relative = Path(name)
    if not name or relative.is_absolute() or ".." in relative.parts:
        raise ValueError("Source must be a relative path inside demos.")
    path = root / relative
    for ancestor in (path, *path.parents):
        if ancestor == root.parent:
            break
        if ancestor.is_symlink():
            raise ValueError("Demo source cannot use symbolic links.")
    if not path.resolve().is_relative_to(root.resolve()) or not path.is_file():
        raise ValueError(f"Demo source does not exist inside demos: {name}")
    if path.stat().st_size > MAX_SOURCE_SIZE:
        raise ValueError("Demo source exceeds 1 MiB.")
    return path, path.read_text(encoding="utf-8")


class DemoTransform(SphinxTransform):
    default_priority = 500

    def apply(self, **kwargs):
        root = Path(self.env.srcdir).parent / "demos"
        sources = {}
        for start in list(self.document.findall(DemoStart)):
            parent = start.parent
            index = parent.index(start)
            tail = parent.children[index + 1 :]
            end = next((n for n in tail if isinstance(n, (DemoEnd, DemoStart))), None)
            if not isinstance(end, DemoEnd):
                logger.warning("Missing erbsland-demo-end directive.", location=start)
                start.replace_self([])
                continue
            finish = parent.index(end)
            content = list(parent.children[index + 1 : finish])
            panel = nodes.container(classes=["erbsland-demo"])
            name = start["options"].get("source", "")
            header = nodes.paragraph("", f"{name}", classes=["demo-source-header"])
            try:
                path, text = read_source(root, name)
                self.env.note_dependency(str(path))
                source_id = "demo-source-" + hashlib.sha256(name.encode()).hexdigest()[:16]
                if source_id not in sources:
                    entry = nodes.container(ids=[source_id], classes=["demo-full-source"])
                    entry += nodes.rubric("", f"{name}")
                    entry += nodes.literal_block(text, text, language="cpp")
                    sources[source_id] = entry
                expand_icon = [nodes.raw("", "<i class=\"fa-solid fa-arrows-up-down\"></i>", format="html")]
                header += nodes.reference("", "", children=expand_icon, refuri="#" + source_id, classes=["demo-source-link"])
            except (OSError, UnicodeError, ValueError) as error:
                logger.warning("Cannot expand demo source: %s", error, location=start)
            panel += header
            excerpt = nodes.container(classes=["demo-excerpt"])
            panel += excerpt
            # Old generated blocks used a rubric followed by the ANSI container.
            pending = None
            for child in content:
                is_ansi = type(child).__name__ == "ANSILiteralBlock"
                if isinstance(child, nodes.rubric):
                    pending = child
                    continue
                if is_ansi:
                    run = nodes.container(classes=["erbsland-demo-run"])
                    if pending is not None:
                        command = "$ " + pending.astext().removeprefix("$ ")
                        run += nodes.literal_block(command, command, language="console", classes=["demo-command"])
                        pending = None
                    else:
                        run += nodes.paragraph("", "Output", classes=["demo-command"])
                    run += child
                    panel += run
                else:
                    if pending is not None:
                        panel += pending
                        pending = None
                    if "erbsland-demo-run" in child.get("classes", []):
                        panel += child
                    else:
                        excerpt += child
            if pending is not None:
                panel += pending
            parent.children[index : finish + 1] = [panel]
            panel.parent = parent
        for end in list(self.document.findall(DemoEnd)):
            logger.warning("Unexpected erbsland-demo-end directive.", location=end)
            end.replace_self([])
        if sources:
            # HTML-only appendix is also the no-JavaScript fallback. JS moves entries
            # temporarily into panels, rather than duplicating complete source text.
            from sphinx import addnodes

            appendix = addnodes.only(expr="html")
            container = nodes.container(classes=["demo-source-appendix"])
            container += nodes.rubric("", "Complete demo sources")
            for entry in sources.values():
                container += entry
            appendix += container
            self.document += appendix


def setup(app: Sphinx):
    app.add_directive("erbsland-demo", ErbslandDemoDirective)
    app.add_directive("erbsland-demo-end", ErbslandDemoEndDirective)
    app.add_directive("erbsland-demo-run", ErbslandDemoRunDirective)
    app.add_transform(DemoTransform)
    app.add_js_file("demo-panels.js")
    return {"version": "2", "parallel_read_safe": True, "parallel_write_safe": True}
