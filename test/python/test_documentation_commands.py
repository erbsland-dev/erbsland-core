# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Process-level checks for the C++ documentation command orchestration."""

from __future__ import annotations

import os
import subprocess
import tempfile
import unittest
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[2]
EXECUTABLE = Path(os.environ.get("ERBSLAND_CORE_TOOL", PROJECT / "erbsland-core-tool"))


@unittest.skipUnless(EXECUTABLE.is_file(), "Build or install the maintenance tool first")
class DocumentationCommandsTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        (self.root / "CMakeLists.txt").write_text("project(erbsland-core LANGUAGES CXX)\n")
        configuration = self.root / "tool/conf/erbsland-core-tool.elcl"
        configuration.parent.mkdir(parents=True)
        configuration.write_text("[rendering]\n")

    def command(self, *arguments: str) -> list[str]:
        return [str(EXECUTABLE), *arguments, "--project", str(self.root)]

    def test_parallel_demo_creation_preserves_every_part(self) -> None:
        processes = [
            subprocess.Popen(
                self.command("create-demo", f"example/Sample/{part}", "--yes"),
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            for part in ("First", "Second", "Third")
        ]
        try:
            for process in processes:
                output, error = process.communicate(timeout=30)
                self.assertEqual(process.returncode, 0, (output + error).decode())
        finally:
            for process in processes:
                if process.poll() is None:
                    process.kill()
                process.communicate()
        demo = self.root / "demos/example/snippets/Sample"
        for part in ("First", "Second", "Third"):
            self.assertTrue((demo / f"{part}.cpp").is_file())
            self.assertIn(f'"{part}"_el', (demo / "main.cpp").read_text())
        cmake = (demo / "CMakeLists.txt").read_text()
        self.assertLess(cmake.index("First.cpp"), cmake.index("Second.cpp"))
        self.assertLess(cmake.index("Second.cpp"), cmake.index("Third.cpp"))

    def test_confirmation_without_terminal_writes_nothing(self) -> None:
        result = subprocess.run(
            self.command("create-demo", "example/Sample/First"), input="n\n", capture_output=True, text=True, timeout=30
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("--yes", result.stdout + result.stderr)
        self.assertFalse((self.root / "demos").exists())
