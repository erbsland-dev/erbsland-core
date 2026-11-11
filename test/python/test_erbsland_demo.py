# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0
import io
import re
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from docutils import nodes
from docutils.core import publish_doctree
from docutils.parsers.rst import directives
from sphinx.application import Sphinx
from sphinx.util.docutils import docutils_namespace

PROJECT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(PROJECT / "doc/_ext"))
from erbsland_demo import (
    ErbslandDemoDirective,
    ErbslandDemoEndDirective,
    ErbslandInPracticeBeginDirective,
    ErbslandInPracticeEndDirective,
    InPracticeBegin,
    InPracticeEnd,
    InPracticeTransform,
    read_source,
)


class DemoPresentationTest(unittest.TestCase):
    def test_html_and_text_presentation(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            doc = root / "doc"
            doc.mkdir()
            demos = root / "demos"
            demos.mkdir()
            source = demos / "sample.cpp"
            source.write_text('// complete source\nconst char *value = "<script>";\n')
            (doc / "conf.py").write_text(
                f"import sys\nsys.path.insert(0, {str(PROJECT / 'doc/_ext')!r})\n"
                "extensions = ['erbsland_demo', 'erbsland.sphinx.ansi']\nroot_doc = 'index'\n"
            )
            block = """
.. erbsland-in-practice-begin:: A practical <example>

Explanation before the demo.

.. erbsland-demo::
    :source: sample.cpp

.. code-block:: cpp

    excerpt();

.. erbsland-demo-run::

    .. code-block:: console

        $ example --first

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[32mSuccess␛[0m

.. erbsland-demo-end::

Explanation after the demo.

.. erbsland-in-practice-end::

Outside the discussion.
"""
            legacy = """
.. erbsland-demo::
    :source: sample.cpp

.. code-block:: cpp

    second();

.. rubric:: ``$ example --second``

.. erbsland-ansi::
    :escape-char: ␛

    Legacy output

.. erbsland-demo-end::
"""
            (doc / "index.rst").write_text("Demo\n====\n" + block + legacy)
            warnings = io.StringIO()
            app = Sphinx(
                str(doc),
                str(doc),
                str(root / "html"),
                str(root / "cache"),
                "html",
                status=io.StringIO(),
                warning=warnings,
                freshenv=True,
            )
            app.build()
            self.assertEqual("", warnings.getvalue())
            html = (root / "html/index.html").read_text()
            self.assertEqual(2, html.count('class="erbsland-demo docutils container"'))
            self.assertEqual(2, html.count('class="erbsland-demo-run docutils container"'))
            self.assertEqual(1, html.count('class="demo-full-source docutils container"'))
            self.assertEqual(1, html.count('class="erbsland-in-practice docutils container"'))
            self.assertIn('class="erbsland-in-practice-title">A practical &lt;example&gt;', html)
            tree = app.env.get_doctree("index")
            panel = next(n for n in tree.findall(nodes.container) if "erbsland-in-practice" in n["classes"])
            self.assertEqual(
                ["A practical <example>", "Explanation before the demo.", "Explanation after the demo."],
                [n.astext() for n in panel.children if isinstance(n, nodes.paragraph)],
            )
            self.assertEqual(1, len([n for n in panel.children if "erbsland-demo" in n.get("classes", [])]))
            self.assertNotIn("Outside the discussion.", panel.astext())
            self.assertNotIn("complete source", panel.astext())
            self.assertNotIn('<script>";', html)
            self.assertIn("&lt;script&gt;", html)
            source_targets = re.findall(r'class="demo-source-link[^\"]*"[^>]*href="#([^\"]+)"', html)
            self.assertEqual(2, len(source_targets))
            self.assertEqual(1, len(set(source_targets)))
            self.assertIn(f'id="{source_targets[0]}"', html)
            self.assertIn("Legacy output", html)
            self.assertIn(source.resolve(), {Path(p).resolve() for p in app.env.dependencies["index"]})
            source.write_text("// refreshed full source\n")
            app.build()
            self.assertIn("refreshed full source", (root / "html/index.html").read_text())
            warnings = io.StringIO()
            app = Sphinx(
                str(doc),
                str(doc),
                str(root / "text"),
                str(root / "text-cache"),
                "text",
                status=io.StringIO(),
                warning=warnings,
                freshenv=True,
            )
            app.build()
            text = (root / "text/index.txt").read_text()
            self.assertIn("A practical <example>", text)
            self.assertIn("Explanation before the demo.", text)
            self.assertIn("Explanation after the demo.", text)
            self.assertIn("Outside the discussion.", text)
            self.assertIn("excerpt();", text)
            self.assertIn("Success", text)
            self.assertIn("Legacy output", text)
            self.assertNotIn("refreshed full source", text)

    def test_template_execution_options_are_reported_even_with_cached_output(self):
        for option in [
            "exec: event/startup",
            "exec-2: event/startup",
            "exec-exit-code: 0",
            "exec-2-exit-code: 0",
            "show-cmd-line:",
            "files: input.txt",
        ]:
            with self.subTest(option=option), docutils_namespace():
                directives.register_directive("erbsland-demo", ErbslandDemoDirective)
                directives.register_directive("erbsland-demo-end", ErbslandDemoEndDirective)
                with patch("erbsland_demo.logger.warning") as warning:
                    publish_doctree(
                        ".. erbsland-demo::\n    :source: event/templates/Startup/main.cpp\n"
                        f"    :{option}\n    :sha256: cached\n\nSaved output.\n\n.. erbsland-demo-end::\n"
                    )
                warning.assert_called_once()
                self.assertIn("execution options are forbidden", warning.call_args.args[0])

    def test_template_source_only_and_runnable_categories_are_allowed(self):
        for category in ["templates", "apps", "snippets"]:
            with self.subTest(category=category), docutils_namespace():
                directives.register_directive("erbsland-demo", ErbslandDemoDirective)
                directives.register_directive("erbsland-demo-end", ErbslandDemoEndDirective)
                execution = "" if category == "templates" else "    :exec: event/example\n"
                with patch("erbsland_demo.logger.warning") as warning:
                    publish_doctree(
                        f".. erbsland-demo::\n    :source: event/{category}/Example/main.cpp\n"
                        f"{execution}\n.. erbsland-demo-end::\n"
                    )
                warning.assert_not_called()
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "event/templates/Startup/main.cpp"
            source.parent.mkdir(parents=True)
            source.write_text("// source-only template\n")
            self.assertEqual((source, source.read_text()), read_source(root, "event/templates/Startup/main.cpp"))

    def test_source_restrictions(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "source.cpp").write_text("good")
            (root / "link.cpp").symlink_to(root / "source.cpp")
            for name in ["../outside.cpp", "/absolute.cpp", "link.cpp", "missing.cpp"]:
                with self.subTest(name=name), self.assertRaises(ValueError):
                    read_source(root, name)


class InPracticeTransformTest(unittest.TestCase):
    def parse(self, source):
        with docutils_namespace():
            directives.register_directive("erbsland-in-practice-begin", ErbslandInPracticeBeginDirective)
            directives.register_directive("erbsland-in-practice-end", ErbslandInPracticeEndDirective)
            document = publish_doctree(source, source_path="practice.rst")
        InPracticeTransform(document).apply()
        self.assertFalse(list(document.findall(InPracticeBegin)))
        self.assertFalse(list(document.findall(InPracticeEnd)))
        for parent in document.findall(nodes.Element):
            for child in parent.children:
                self.assertIs(parent, child.parent)
        return document

    def test_nested_and_adjacent_discussions(self):
        document = self.parse("""
Before.

.. erbsland-in-practice-begin::

* First item.
* Second item.

.. erbsland-in-practice-begin:: Inner label

.. code-block:: cpp

    work();

.. erbsland-in-practice-end::

Outer continuation.

.. erbsland-in-practice-end::

.. erbsland-in-practice-begin:: Adjacent label

Another discussion.

.. erbsland-in-practice-end::

After.
""")
        panels = [n for n in document.findall(nodes.container) if "erbsland-in-practice" in n["classes"]]
        outer, inner, adjacent = panels
        self.assertEqual("In Practice", outer[0].astext())
        self.assertEqual("Inner label", inner[0].astext())
        self.assertIs(inner.parent, outer)
        self.assertIs(adjacent.parent, outer.parent)
        self.assertIsInstance(outer[1], nodes.bullet_list)
        self.assertEqual("work();", inner[1].astext())
        self.assertEqual("Outer continuation.", outer[-1].astext())
        self.assertEqual("Before.", document[0].astext())
        self.assertEqual("After.", document[-1].astext())

    def test_unmatched_markers_warn_without_losing_content(self):
        cases = [
            (".. erbsland-in-practice-begin::\n\nKeep this text.\n", ["Missing"]),
            (".. erbsland-in-practice-end::\n\nKeep this text.\n", ["Unexpected"]),
            (
                "First\n=====\n\n.. erbsland-in-practice-begin::\n\nKeep this text.\n\n"
                "Second\n======\n\n.. erbsland-in-practice-end::\n",
                ["Missing", "Unexpected"],
            ),
        ]
        for source, expected in cases:
            with self.subTest(source=source), patch("erbsland_demo.logger.warning") as warning:
                document = self.parse(source)
                self.assertIn("Keep this text.", document.astext())
                self.assertFalse(list(document.findall(nodes.container)))
                self.assertEqual(expected, [call.args[0].split()[0] for call in warning.call_args_list])
                for call in warning.call_args_list:
                    self.assertEqual("practice.rst", call.kwargs["location"].source)
                    self.assertIsInstance(call.kwargs["location"].line, int)
