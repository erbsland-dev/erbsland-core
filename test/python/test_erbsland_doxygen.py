# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

DOC_DIR = Path(__file__).resolve().parents[2] / "doc"
sys.path.insert(0, str(DOC_DIR))
sys.path.insert(0, str(DOC_DIR / "_ext"))

import erbsland_doxygen
from sphinx.errors import ExtensionError


class DoxygenCacheTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.output = self.root / "build" / "erbsland-core"
        self.index = self.output / "xml" / "index.xml"
        self.sources = self.root / "input with spaces"
        self.sources.mkdir()
        self.marker = self.sources / ".preprocess-sources-run"
        self.marker.write_text("initial sources")
        self.app = SimpleNamespace(
            config=SimpleNamespace(
                breathe_projects={"erbsland-core": self.index.parent},
                breathe_doxygen_config_options={"RECURSIVE": "yes"},
                breathe_doxygen_aliases={},
            )
        )
        self.preprocess = self.enterContext(
            patch.object(erbsland_doxygen, "preprocess_sources", return_value=self.sources)
        )
        self.doxygen = self.enterContext(
            patch.object(erbsland_doxygen.subprocess, "run", side_effect=self.generate_xml)
        )

    def generate_xml(self, *args, **kwargs) -> None:
        self.index.parent.mkdir(parents=True, exist_ok=True)
        self.index.write_text("<doxygenindex/>")

    def test_unchanged_build_does_not_run_doxygen_or_touch_xml(self) -> None:
        erbsland_doxygen.prepare_doxygen(self.app)
        initial_mtime = self.index.stat().st_mtime_ns
        erbsland_doxygen.prepare_doxygen(self.app)
        self.doxygen.assert_called_once()
        self.assertEqual(initial_mtime, self.index.stat().st_mtime_ns)

    def test_source_configuration_alias_and_missing_xml_invalidate_cache(self) -> None:
        erbsland_doxygen.prepare_doxygen(self.app)
        self.marker.write_text("changed sources")
        erbsland_doxygen.prepare_doxygen(self.app)
        self.app.config.breathe_doxygen_config_options["RECURSIVE"] = "no"
        erbsland_doxygen.prepare_doxygen(self.app)
        self.app.config.breathe_doxygen_aliases["test"] = "replacement"
        erbsland_doxygen.prepare_doxygen(self.app)
        self.index.unlink()
        erbsland_doxygen.prepare_doxygen(self.app)
        self.assertEqual(5, self.doxygen.call_count)

    def test_failed_generation_invalidates_stamp_and_retries(self) -> None:
        erbsland_doxygen.prepare_doxygen(self.app)
        self.marker.write_text("changed sources")
        self.doxygen.side_effect = subprocess.CalledProcessError(1, ["doxygen"])
        with self.assertRaises(ExtensionError):
            erbsland_doxygen.prepare_doxygen(self.app)
        self.assertFalse((self.output / ".doxygen-input-sha256").exists())
        self.doxygen.side_effect = self.generate_xml
        erbsland_doxygen.prepare_doxygen(self.app)
        self.assertEqual(3, self.doxygen.call_count)

    def test_paths_with_spaces_are_quoted_and_no_shell_is_used(self) -> None:
        erbsland_doxygen.prepare_doxygen(self.app)
        configuration = (self.output.parent / "erbsland-core.cfg").read_text()
        self.assertIn(f'INPUT = "{self.sources / "erbsland"}"', configuration)
        self.doxygen.assert_called_once_with(
            ["doxygen", str(self.output.parent / "erbsland-core.cfg")], cwd=self.output.parent, check=True
        )

    def test_compaction_preserves_documentation_and_other_graphs(self) -> None:
        self.index.parent.mkdir(parents=True)
        content = (
            b'<doxygen>\n    <incdepgraph><node id="1"/></incdepgraph>\n'
            b'    <invincdepgraph><node id="2"/></invincdepgraph>\n'
            b"<detaileddescription>&lt;incdepgraph&gt; text</detaileddescription>"
            b'<inheritancegraph><node id="3"/></inheritancegraph></doxygen>'
        )
        self.index.write_bytes(content)
        erbsland_doxygen.remove_include_graphs(self.index.parent)
        self.assertEqual(
            b"<doxygen>\n<detaileddescription>&lt;incdepgraph&gt; text</detaileddescription>"
            b'<inheritancegraph><node id="3"/></inheritancegraph></doxygen>',
            self.index.read_bytes(),
        )
        mtime = self.index.stat().st_mtime_ns
        erbsland_doxygen.remove_include_graphs(self.index.parent)
        self.assertEqual(mtime, self.index.stat().st_mtime_ns)
        self.index.write_bytes(content.replace(b"\n", b"\r\n"))
        erbsland_doxygen.remove_include_graphs(self.index.parent)
        self.assertNotIn(b"<incdepgraph>", self.index.read_bytes())
        self.assertIn(b"<inheritancegraph>", self.index.read_bytes())


if __name__ == "__main__":
    unittest.main()


class PreprocessorInvocationTest(unittest.TestCase):
    def test_executable_override_is_passed_as_one_argument_and_checked(self) -> None:
        executable = Path("/a path/tool")
        with patch.dict("os.environ", {"ERBSLAND_CORE_TOOL": str(executable)}), patch.object(
            Path, "is_file", return_value=True
        ), patch.object(erbsland_doxygen.subprocess, "run") as run:
            result = erbsland_doxygen.preprocess_sources()
        project = DOC_DIR.parent
        self.assertEqual(result, project / "_doxygen_input")
        run.assert_called_once_with(
            [str(executable), "doc-preprocess-sources", "--project", str(project)], cwd=project, check=True
        )

    def test_missing_or_failed_tool_reports_extension_error(self) -> None:
        with patch.object(Path, "is_file", return_value=False), self.assertRaises(ExtensionError):
            erbsland_doxygen.preprocess_sources()
        with patch.object(Path, "is_file", return_value=True), patch.object(
            erbsland_doxygen.subprocess, "run", side_effect=subprocess.CalledProcessError(1, ["tool"])
        ), self.assertRaises(ExtensionError):
            erbsland_doxygen.preprocess_sources()
