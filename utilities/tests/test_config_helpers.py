# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import sys
import unittest
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from dev.cleanup import CleanupApp
from dev.pre_commit import PreCommitApp
from lib.config import (
    project_relative_posix,
    validate_file_suffixes,
    validate_local_names,
    validate_source_relative_path,
)
from lib.error import UtilityError
from lib.utility import UtilityApp


class RecordingApp(UtilityApp):
    """Record arguments passed to a child utility."""

    def __init__(self) -> None:
        super().__init__()
        self.arguments: list[str] | None = None

    def run(self, argv=None) -> None:
        self.arguments = list(argv or [])


class ConfigHelpersTest(unittest.TestCase):
    """Tests for shared utility configuration helpers."""

    def test_utility_app_uses_standard_config_file_path(self) -> None:
        self.assertEqual(
            Path(__file__).resolve().parents[1] / "conf" / "cleanup.elcl",
            CleanupApp().config_file_path(),
        )

    def test_utility_app_uses_private_config_file_path_when_requested(self) -> None:
        app = CleanupApp()
        app.parse_command_line(["--private-config"])

        self.assertEqual(
            Path(__file__).resolve().parents[1] / "_conf" / "cleanup.elcl",
            app.config_file_path(),
        )

    def test_pre_commit_propagates_private_config_to_child_apps(self) -> None:
        pre_commit = PreCommitApp()
        pre_commit.verbose = True
        pre_commit.private_config = True
        child = RecordingApp()

        with redirect_stdout(StringIO()):
            pre_commit.run_app(child, "recording")

        self.assertEqual(["--verbose", "--private-config"], child.arguments)

    def test_validates_local_names(self) -> None:
        validate_local_names(["file.txt", "directory"], "Names")
        with self.assertRaises(UtilityError):
            validate_local_names(["../file.txt"], "Names")
        with self.assertRaises(UtilityError):
            validate_local_names(["."], "Names")

    def test_validates_file_suffixes(self) -> None:
        validate_file_suffixes([".hpp", ".cpp"], "Suffixes")
        with self.assertRaises(UtilityError):
            validate_file_suffixes(["hpp"], "Suffixes")
        with self.assertRaises(UtilityError):
            validate_file_suffixes(["."], "Suffixes")

    def test_validates_source_relative_paths(self) -> None:
        validate_source_relative_path("src/erbsland", "Path")
        validate_source_relative_path("", "Path")
        with self.assertRaises(UtilityError):
            validate_source_relative_path(".", "Path")
        with self.assertRaises(UtilityError):
            validate_source_relative_path("../src", "Path")
        with self.assertRaises(UtilityError):
            validate_source_relative_path("/tmp/src", "Path")

    def test_project_relative_posix_path(self) -> None:
        project_dir = Path("/tmp/project")

        self.assertEqual("doc/reference", project_relative_posix(project_dir, project_dir / "doc" / "reference"))
        self.assertEqual(
            "doc/reference/",
            project_relative_posix(project_dir, project_dir / "doc" / "reference", is_directory=True),
        )


if __name__ == "__main__":
    unittest.main()
