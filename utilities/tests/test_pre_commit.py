# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import subprocess
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path
from unittest.mock import MagicMock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from dev.pre_commit import PreCommitApp
from lib.error import UtilityError


class PreCommitTest(unittest.TestCase):
    """Tests for pre-commit git state checks."""

    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory(dir="/private/tmp")
        self.project_dir = Path(self.temp_dir.name)
        self.run_git("init")
        self.run_git("config", "user.email", "test@example.invalid")
        self.run_git("config", "user.name", "Pre Commit Test")
        self.run_git("config", "commit.gpgsign", "false")
        (self.project_dir / "file.txt").write_text("initial\n", encoding="utf-8")
        self.run_git("add", "file.txt")
        self.run_git("commit", "-m", "initial")
        self.app = PreCommitApp()
        self.app.project_root = self.project_dir

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def run_git(self, *args: str) -> None:
        """Run git in the temporary project."""
        subprocess.run(
            ["git", "-C", str(self.project_dir), *args],
            check=True,
            capture_output=True,
        )

    def test_git_state_detects_dirty_file_content_changes(self) -> None:
        (self.project_dir / "file.txt").write_text("dirty once\n", encoding="utf-8")
        initial_state = self.app.git_local_state()

        (self.project_dir / "file.txt").write_text("dirty twice\n", encoding="utf-8")

        with self.assertRaises(UtilityError):
            self.app.require_unchanged_git_state(initial_state)

    def test_git_state_detects_untracked_file_content_changes(self) -> None:
        (self.project_dir / "generated.txt").write_text("generated once\n", encoding="utf-8")
        initial_state = self.app.git_local_state()

        (self.project_dir / "generated.txt").write_text("generated twice\n", encoding="utf-8")

        with self.assertRaises(UtilityError):
            self.app.require_unchanged_git_state(initial_state)

    def test_git_state_accepts_unchanged_dirty_state(self) -> None:
        (self.project_dir / "file.txt").write_text("dirty once\n", encoding="utf-8")
        initial_state = self.app.git_local_state()

        self.app.require_unchanged_git_state(initial_state)

    def test_anti_pattern_scan_rejects_active_findings(self) -> None:
        scanner = MagicMock()
        scanner.active_findings = 3
        with (
            patch("dev.pre_commit.AntiPatternsApp", return_value=scanner),
            redirect_stdout(StringIO()),
            self.assertRaisesRegex(UtilityError, "3 active finding"),
        ):
            self.app.run_anti_patterns()

        scanner.run.assert_called_once_with([])

    def test_anti_pattern_scan_propagates_child_options(self) -> None:
        self.app.verbose = True
        self.app.private_config = True
        scanner = MagicMock()
        scanner.active_findings = 0
        with patch("dev.pre_commit.AntiPatternsApp", return_value=scanner), redirect_stdout(StringIO()):
            self.app.run_anti_patterns()

        scanner.run.assert_called_once_with(["--verbose", "--private-config"])


if __name__ == "__main__":
    unittest.main()
