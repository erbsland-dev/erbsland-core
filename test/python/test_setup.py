# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0
"""Bootstrap sequencing and installation failure tests."""

import importlib.util
import os
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

PROJECT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("repository_setup", PROJECT / "setup.py")
bootstrap = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bootstrap)


class SetupTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        (self.root / "CMakeLists.txt").write_text("project(erbsland-core LANGUAGES CXX)\n")
        self.build = self.root / "release"
        self.name = "erbsland-core-tool.exe" if os.name == "nt" else "erbsland-core-tool"
        self.source = self.build / "tool" / self.name
        self.source.parent.mkdir(parents=True)
        self.source.write_bytes(b"binary")

    def test_command_sequence_and_install_after_success(self):
        def complete(command, root, label):
            if "venv" in command:
                python = root / ".venv" / ("Scripts/python.exe" if os.name == "nt" else "bin/python")
                python.parent.mkdir(parents=True)
                python.touch()

        with patch.object(bootstrap, "run", side_effect=complete) as run, patch.object(
            bootstrap, "install_binary"
        ) as install:
            bootstrap.setup(self.root)
        commands = [call.args[0] for call in run.call_args_list]
        self.assertEqual(len(commands), 6)
        self.assertEqual(commands[0][1:3], ["-m", "venv"])
        self.assertEqual(commands[1][1:4], ["-m", "pip", "install"])
        self.assertEqual(commands[1][4:], ["-r", "requirements-dev.txt", "-r", "doc/requirements-doc.txt"])
        self.assertEqual(commands[2][:4], ["git", "submodule", "update", "--init"])
        self.assertIn("-DCMAKE_BUILD_TYPE=Debug", commands[3])
        self.assertIn("-DCMAKE_BUILD_TYPE=Release", commands[4])
        self.assertIn("-DERBSLAND_CORE_ENABLE_TESTS=OFF", commands[4])
        self.assertEqual(commands[5][-3:], ["--target", "erbsland-core-tool", "--parallel"])
        install.assert_called_once_with(self.root, self.root / "cmake-build-erbsland-core-tool-release")

    def test_failure_stops_before_install(self):
        with patch.object(bootstrap, "run", side_effect=bootstrap.SetupError("failure")) as run, patch.object(
            bootstrap, "install_binary"
        ) as install:
            with self.assertRaises(bootstrap.SetupError):
                bootstrap.setup(self.root)
        self.assertEqual(run.call_count, 1)
        install.assert_not_called()

    def test_incomplete_environment_and_invalid_root(self):
        (self.root / ".venv").mkdir()
        with patch.object(bootstrap, "run") as run:
            with self.assertRaises(bootstrap.SetupError):
                bootstrap.setup(self.root)
            run.assert_not_called()
        (self.root / "CMakeLists.txt").unlink()
        with self.assertRaises(bootstrap.SetupError):
            bootstrap.setup(self.root)

    def test_command_failure_has_diagnostic(self):
        with patch.object(bootstrap.subprocess, "run", side_effect=subprocess.CalledProcessError(9, ["cmake"])):
            with self.assertRaisesRegex(bootstrap.SetupError, "status 9"):
                bootstrap.run(["cmake"], self.root, "Build")

    def test_install_and_update(self):
        bootstrap.install_binary(self.root, self.build)
        destination = self.root / "tool/bin" / self.name
        self.assertEqual(destination.read_bytes(), b"binary")
        self.source.write_bytes(b"updated")
        bootstrap.install_binary(self.root, self.build)
        self.assertEqual(destination.read_bytes(), b"updated")
        self.assertFalse(destination.with_name(f".{self.name}.new").exists())
        if os.name != "nt":
            self.assertEqual(os.readlink(self.root / "erbsland-core-tool"), "tool/bin/erbsland-core-tool")

    @unittest.skipIf(os.name == "nt", "POSIX shortcut")
    def test_unrelated_shortcut_is_preserved(self):
        shortcut = self.root / "erbsland-core-tool"
        shortcut.write_text("user data")
        with self.assertRaises(bootstrap.SetupError):
            bootstrap.install_binary(self.root, self.build)
        self.assertEqual(shortcut.read_text(), "user data")
        self.assertFalse((self.root / "tool/bin" / self.name).exists())
        shortcut.unlink()
        shortcut.symlink_to("elsewhere")
        with self.assertRaises(bootstrap.SetupError):
            bootstrap.install_binary(self.root, self.build)
        self.assertEqual(os.readlink(shortcut), "elsewhere")

    def test_copy_failure_preserves_installed_binary(self):
        bootstrap.install_binary(self.root, self.build)
        with patch.object(bootstrap.shutil, "copy2", side_effect=OSError("failed copy")):
            with self.assertRaises(OSError):
                bootstrap.install_binary(self.root, self.build)
        destination = self.root / "tool/bin" / self.name
        self.assertEqual(destination.read_bytes(), b"binary")
        self.assertFalse(destination.with_name(f".{self.name}.new").exists())
