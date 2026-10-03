# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0
import io
import re
import sys
import tempfile
import unittest
from pathlib import Path
from sphinx.application import Sphinx

PROJECT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(PROJECT / "doc/_ext"))
from erbsland_demo import read_source


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
            self.assertIn("excerpt();", text)
            self.assertIn("Success", text)
            self.assertIn("Legacy output", text)
            self.assertNotIn("refreshed full source", text)

    def test_source_restrictions(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "source.cpp").write_text("good")
            (root / "link.cpp").symlink_to(root / "source.cpp")
            for name in ["../outside.cpp", "/absolute.cpp", "link.cpp", "missing.cpp"]:
                with self.subTest(name=name), self.assertRaises(ValueError):
                    read_source(root, name)
