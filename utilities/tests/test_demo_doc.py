# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import os
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from dev.demo_doc import DemoDocError, DemoDocSynchronizer, DemoExecutor
from lib.error import UtilityError


class DemoDocTest(unittest.TestCase):
    """Tests for demo documentation synchronization."""

    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory(dir="/private/tmp")
        self.project_dir = Path(self.temp_dir.name)
        self.demo_source_dir = self.project_dir / "demos" / "text"
        self.demo_source_dir.mkdir(parents=True)
        self.source_path = self.demo_source_dir / "Sample.cpp"
        self.source_path.write_text(
            "#include <DemoCommon.hpp>\n"
            "\n"
            "void helper();\n"
            "\n"
            "/// Demo documentation starts here.\n"
            "void sampleDemo() {\n"
            "    helper();\n"
            "}\n"
            "\n",
            encoding="utf-8",
        )
        self.demo_output_dir = self.project_dir / "cmake-build-debug" / "demo-apps"
        self.demo_output_dir.mkdir(parents=True)
        self.executable_path = self.demo_output_dir / "demoapp"
        self.executable_path.write_text("#!/bin/sh\nprintf 'demo output\\n'\n", encoding="utf-8")
        self.executable_path.chmod(0o755)

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def synchronizer(self, *, force: bool = False) -> DemoDocSynchronizer:
        """Create a synchronizer for the temp project."""
        return DemoDocSynchronizer(self.project_dir, force=force)

    def current_hash(self) -> str:
        """Get the current test source hash."""
        return self.synchronizer().source_hash("text/Sample.cpp")

    def test_zero_blocks_are_unchanged(self) -> None:
        text = "Plain document.\n"

        updated_text, issues = self.synchronizer().process_text(self.project_dir / "doc.rst", text)

        self.assertEqual(text, updated_text)
        self.assertEqual((), issues)

    def test_sync_generates_source_and_output_for_missing_hash(self) -> None:
        text = (
            "Before.\n"
            "\n"
            ".. erbsland-demo::\n"
            "    :source: text/Sample.cpp\n"
            "    :exec: demoapp --demo Sample\n"
            "\n"
            "    stale\n"
            "\n"
            ".. erbsland-demo-end::\n"
            "\n"
            "After.\n"
        )

        updated_text, issues = self.synchronizer().process_text(self.project_dir / "doc.rst", text)

        self.assertEqual((), issues)
        self.assertIn(f"    :source-sha256: {self.current_hash()}", updated_text)
        self.assertIn("    /// Demo documentation starts here.\n", updated_text)
        self.assertNotIn("#include <DemoCommon.hpp>", updated_text)
        self.assertIn(".. erbsland-ansi::\n", updated_text)
        self.assertIn("    demo output\n", updated_text)

    def test_matching_hash_leaves_block_unchanged_without_force(self) -> None:
        text = (
            ".. erbsland-demo::\n"
            "    :source: text/Sample.cpp\n"
            f"    :source-sha256: {self.current_hash()}\n"
            "\n"
            "    intentionally stale body\n"
            "\n"
            ".. erbsland-demo-end::\n"
        )

        updated_text, issues = self.synchronizer().process_text(self.project_dir / "doc.rst", text)

        self.assertEqual(text, updated_text)
        self.assertEqual((), issues)

    def test_force_regenerates_matching_hash(self) -> None:
        text = (
            ".. erbsland-demo::\n"
            "    :source: text/Sample.cpp\n"
            f"    :source-sha256: {self.current_hash()}\n"
            "\n"
            "    intentionally stale body\n"
            "\n"
            ".. erbsland-demo-end::\n"
        )

        updated_text, issues = self.synchronizer(force=True).process_text(self.project_dir / "doc.rst", text)

        self.assertEqual((), issues)
        self.assertIn(".. code-block:: cpp\n", updated_text)
        self.assertNotIn("intentionally stale body", updated_text)

    def test_multiple_blocks_are_processed(self) -> None:
        text = (
            ".. erbsland-demo::\n"
            "    :source: text/Sample.cpp\n"
            "\n"
            ".. erbsland-demo-end::\n"
            "\n"
            ".. erbsland-demo::\n"
            "    :source: text/Sample.cpp\n"
            "\n"
            ".. erbsland-demo-end::\n"
        )

        updated_text, issues = self.synchronizer().process_text(self.project_dir / "doc.rst", text)

        self.assertEqual((), issues)
        self.assertEqual(2, updated_text.count(".. code-block:: cpp"))

    def test_missing_documentation_comment_writes_note_and_issue(self) -> None:
        no_comment_path = self.demo_source_dir / "NoComment.cpp"
        no_comment_path.write_text("void noComment() {}\n", encoding="utf-8")
        text = ".. erbsland-demo::\n" "    :source: text/NoComment.cpp\n" "\n" ".. erbsland-demo-end::\n"

        updated_text, issues = self.synchronizer().process_text(self.project_dir / "doc.rst", text)

        self.assertEqual(1, len(issues))
        self.assertIn("has no /// documentation comment", issues[0])
        self.assertIn(".. note::\n", updated_text)

    def test_malformed_blocks_fail_clearly(self) -> None:
        with self.assertRaises(UtilityError):
            self.synchronizer().process_text(self.project_dir / "doc.rst", ".. erbsland-demo::\n")

        with self.assertRaises(UtilityError):
            self.synchronizer().process_text(self.project_dir / "doc.rst", ".. erbsland-demo-end::\n")

    def test_exec_validation_accepts_strict_demo_command(self) -> None:
        executor = DemoExecutor(self.project_dir)

        self.assertEqual(["demoapp", "--demo", "Sample"], executor.validate_command("demoapp --demo Sample"))

    def test_exec_validation_rejects_suspicious_commands(self) -> None:
        executor = DemoExecutor(self.project_dir)

        for command in ("../demoapp", "demo.app", "demoapp --demo Sample.cpp", "demoapp 'bad value'"):
            with self.subTest(command=command):
                with self.assertRaises(DemoDocError):
                    executor.validate_command(command)

    def test_executable_path_rejects_missing_non_executable_and_symlink(self) -> None:
        executor = DemoExecutor(self.project_dir)
        non_executable = self.demo_output_dir / "plain"
        non_executable.write_text("#!/bin/sh\n", encoding="utf-8")
        symlink = self.demo_output_dir / "linked"
        os.symlink(self.executable_path, symlink)

        with self.assertRaises(DemoDocError):
            executor.executable_path("missing")
        with self.assertRaises(DemoDocError):
            executor.executable_path("plain")
        with self.assertRaises(DemoDocError):
            executor.executable_path("linked")


if __name__ == "__main__":
    unittest.main()
