# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import subprocess
import sys
import tempfile
import unittest
from io import StringIO
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from dev.dev_setup import DevSetupRunner
from lib.error import UtilityError
from run import REGISTERED_UTILITIES


class DevSetupTest(unittest.TestCase):
    """Tests for the development setup utility."""

    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory(dir="/private/tmp")
        self.project_dir = Path(self.temp_dir.name)
        self.commands: list[tuple[list[str], Path]] = []

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def venv_python(self) -> Path:
        """Get the expected virtual environment Python path."""
        return DevSetupRunner(self.project_dir).venv_python

    def command_runner(self, command, cwd: Path) -> None:
        """Record setup commands and emulate venv creation."""
        command = list(command)
        self.commands.append((command, cwd))
        if command[:3] == [sys.executable, "-m", "venv"]:
            python_path = self.venv_python()
            python_path.parent.mkdir(parents=True, exist_ok=True)
            python_path.write_text("", encoding="utf-8")

    def run_setup(self) -> None:
        """Run the setup runner with the recording command runner."""
        DevSetupRunner(self.project_dir, command_runner=self.command_runner, output_stream=StringIO()).run()

    def test_utility_is_registered(self) -> None:
        self.assertIn("dev_setup", REGISTERED_UTILITIES)

    def test_runs_setup_commands_in_order(self) -> None:
        self.run_setup()

        self.assertEqual(4, len(self.commands))
        self.assertEqual([sys.executable, "-m", "venv", str(self.project_dir / ".venv")], self.commands[0][0])
        self.assertEqual(
            [
                str(self.venv_python()),
                "-m",
                "pip",
                "install",
                "-r",
                "utilities/requirements-dev.txt",
                "-r",
                "doc/requirements-doc.txt",
            ],
            self.commands[1][0],
        )
        self.assertEqual(
            [
                "git",
                "-C",
                str(self.project_dir),
                "submodule",
                "update",
                "--init",
                "--recursive",
                "test/erbsland-unittest",
            ],
            self.commands[2][0],
        )
        self.assertEqual(
            [
                "cmake",
                "-S",
                ".",
                "-B",
                "cmake-build-debug",
                "-G",
                "Ninja",
                "-DCMAKE_BUILD_TYPE=Debug",
                "-DCMAKE_UNITY_BUILD=ON",
            ],
            self.commands[3][0],
        )
        self.assertTrue(all(cwd == self.project_dir for _, cwd in self.commands))

    def test_existing_virtual_environment_is_reused(self) -> None:
        venv_python = self.venv_python()
        venv_python.parent.mkdir(parents=True)
        venv_python.write_text("", encoding="utf-8")

        self.run_setup()

        self.assertEqual(3, len(self.commands))
        self.assertEqual(str(venv_python), self.commands[0][0][0])
        self.assertEqual("git", self.commands[1][0][0])
        self.assertEqual("cmake", self.commands[2][0][0])

    def test_subprocess_failure_is_reported_as_utility_error(self) -> None:
        def failing_runner(command, cwd: Path) -> None:
            raise subprocess.CalledProcessError(23, command)

        runner = DevSetupRunner(self.project_dir, command_runner=failing_runner, output_stream=StringIO())

        with self.assertRaisesRegex(UtilityError, "Creating .venv virtual environment failed with exit code 23"):
            runner.run()


if __name__ == "__main__":
    unittest.main()
