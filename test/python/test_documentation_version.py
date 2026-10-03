# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

import os
import runpy
import subprocess
import sys
import unittest
from pathlib import Path
from unittest.mock import patch


class DocumentationVersionTest(unittest.TestCase):
    configuration = Path(__file__).resolve().parents[2] / "doc" / "conf.py"

    def read_configuration(self):
        with patch.object(sys, "path", list(sys.path)):
            return runpy.run_path(str(self.configuration))

    def test_local_build_does_not_require_git(self):
        with patch.dict(os.environ, {"GITHUB_ACTIONS": "false"}), patch("subprocess.check_output") as git:
            configuration = self.read_configuration()
        self.assertEqual(configuration["release"], "development")
        git.assert_not_called()

    def test_github_build_uses_release_tag(self):
        for tag in ("v0.90.0", "0.90.0"):
            with self.subTest(tag=tag), patch.dict(os.environ, {"GITHUB_ACTIONS": "true"}), patch(
                "subprocess.check_output", return_value=tag + "\n"
            ) as git:
                configuration = self.read_configuration()
                self.assertEqual(configuration["version"], "0.90")
                self.assertEqual(configuration["release"], "0.90.0")
                git.assert_called_once_with(
                    ["git", "describe", "--tags", "--abbrev=0"], cwd=self.configuration.parent.parent, text=True
                )

    def test_github_build_rejects_missing_or_invalid_tags(self):
        with patch.dict(os.environ, {"GITHUB_ACTIONS": "true"}):
            with patch("subprocess.check_output", return_value="not-a-release\n"), self.assertRaises(RuntimeError):
                self.read_configuration()
            with patch("subprocess.check_output", side_effect=subprocess.CalledProcessError(128, "git")):
                with self.assertRaises(subprocess.CalledProcessError):
                    self.read_configuration()
