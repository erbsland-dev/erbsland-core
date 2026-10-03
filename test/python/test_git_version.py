# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


@unittest.skipUnless(shutil.which("git") and shutil.which("cmake"), "Git and CMake are required")
class GitVersionTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = self.root / "source"
        self.source.mkdir()
        self.helper = Path(__file__).resolve().parents[2] / "cmake" / "git-version.cmake"
        self.script = self.root / "version.cmake"
        self.template = self.root / "version.in"
        self.template.write_text("@PROBE_MAJOR@.@PROBE_MINOR@.@PROBE_REVISION@.@PROBE_BUILD@\n@PROBE_TEXT@\n")
        self.output = self.root / "version.txt"
        self.script.write_text(
            f'include("{self.helper.as_posix()}")\n'
            f'set(ERBSLAND_CORE_SOURCE_DIR "{self.source.as_posix()}")\n'
            f'set(ERBSLAND_CORE_TEMPLATE "{self.template.as_posix()}")\n'
            f'set(ERBSLAND_CORE_OUTPUT "{self.output.as_posix()}")\n'
            'set(ERBSLAND_GIT_VERSION_VARIABLE_PREFIX "PROBE")\n'
            'erbsland_core_read_git_version("${ERBSLAND_CORE_SOURCE_DIR}" PACKAGE)\n'
            "erbsland_core_generate_version_source()\n"
            f'file(WRITE "{(self.root / "package.txt").as_posix()}" '
            '"${PACKAGE_MAJOR}.${PACKAGE_MINOR}.${PACKAGE_REVISION}.${PACKAGE_BUILD}")\n'
        )

    def git(self, *arguments):
        subprocess.run(["git", *arguments], cwd=self.source, check=True, capture_output=True, text=True)

    def read_version(self):
        result = subprocess.run(["cmake", "-P", str(self.script)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        lines = self.output.read_text().splitlines()
        self.assertEqual(lines[0], (self.root / "package.txt").read_text())
        return lines

    def initialize_repository(self):
        self.git("init")
        self.git("config", "user.name", "Version Test")
        self.git("config", "user.email", "test@example.invalid")
        self.git("config", "commit.gpgsign", "false")
        self.git("commit", "--allow-empty", "-m", "Initial")

    def test_missing_metadata_and_untagged_checkout(self):
        self.assertEqual(self.read_version(), ["0.0.0.0", "0.0.0.0"])
        self.initialize_repository()
        self.assertEqual(self.read_version(), ["0.0.0.0", "0.0.0.0"])

    def test_release_tag_and_subsequent_commit(self):
        self.initialize_repository()
        self.git("tag", "v0.90.0")
        self.assertEqual(self.read_version()[0], "0.90.0.0")
        self.git("commit", "--allow-empty", "-m", "After release")
        version, description = self.read_version()
        self.assertEqual(version, "0.90.0.1")
        self.assertTrue(description.startswith("v0.90.0-1-g"))

    def test_numeric_normalization_and_overflow(self):
        self.initialize_repository()
        self.git("tag", "00.090.000")
        self.assertEqual(self.read_version()[0], "0.90.0.0")
        self.git("tag", "-d", "00.090.000")
        self.git("tag", "v0.65536.0")
        self.assertEqual(self.read_version(), ["0.0.0.0", "0.0.0.0"])

    @unittest.skipUnless(shutil.which("ninja"), "Ninja is required")
    def test_new_tag_refreshes_configured_package_version(self):
        self.initialize_repository()
        self.git("tag", "v0.89.0")
        self.git("commit", "--allow-empty", "-m", "Prepare beta")
        (self.source / "CMakeLists.txt").write_text(
            "cmake_minimum_required(VERSION 3.28)\nproject(VersionProbe NONE)\n"
            f'include("{self.helper.as_posix()}")\n'
            'erbsland_core_read_git_version("${CMAKE_CURRENT_SOURCE_DIR}" PACKAGE)\n'
            'erbsland_core_collect_git_dependencies(dependencies "${CMAKE_CURRENT_SOURCE_DIR}" TRACK_REFS)\n'
            "set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${dependencies})\n"
            "include(CMakePackageConfigHelpers)\n"
            'write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/Version.cmake" '
            'VERSION "${PACKAGE_MAJOR}.${PACKAGE_MINOR}.${PACKAGE_REVISION}" '
            "COMPATIBILITY SameMinorVersion ARCH_INDEPENDENT)\n"
        )
        build = self.root / "build"
        subprocess.run(
            ["cmake", "-S", str(self.source), "-B", str(build), "-G", "Ninja"], check=True, capture_output=True
        )
        package = build / "Version.cmake"
        self.assertIn('set(PACKAGE_VERSION "0.89.0")', package.read_text())
        self.git("tag", "v0.90.0")
        subprocess.run(["cmake", "--build", str(build)], check=True, capture_output=True)
        self.assertIn('set(PACKAGE_VERSION "0.90.0")', package.read_text())
        for requested, compatible in (("0.90.0", True), ("0.90.1", False), ("0.89.0", False), ("0.91.0", False)):
            major, minor, patch = requested.split(".")
            probe = self.root / "compatibility.cmake"
            probe.write_text(
                f'set(PACKAGE_FIND_VERSION "{requested}")\n'
                f'set(PACKAGE_FIND_VERSION_MAJOR "{major}")\n'
                f'set(PACKAGE_FIND_VERSION_MINOR "{minor}")\n'
                f'set(PACKAGE_FIND_VERSION_PATCH "{patch}")\n'
                f'include("{package.as_posix()}")\n'
                f'if(NOT PACKAGE_VERSION_COMPATIBLE STREQUAL "{str(compatible).upper()}")\n'
                'message(FATAL_ERROR "Unexpected compatibility")\nendif()\n'
            )
            result = subprocess.run(["cmake", "-P", str(probe)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_ref_tracking_is_opt_in(self):
        self.initialize_repository()
        self.git("tag", "v0.90.0")
        dependency_file = self.root / "dependencies.txt"
        self.script.write_text(
            f'include("{self.helper.as_posix()}")\n'
            f'erbsland_core_collect_git_dependencies(default "{self.source.as_posix()}")\n'
            f'erbsland_core_collect_git_dependencies(expanded "{self.source.as_posix()}" TRACK_REFS)\n'
            f'file(WRITE "{dependency_file.as_posix()}" "${{default}}\\n${{expanded}}")\n'
        )
        subprocess.run(["cmake", "-P", str(self.script)], check=True, capture_output=True)
        default, expanded = (line.split(";") for line in dependency_file.read_text().splitlines())
        head = (self.source / ".git" / "HEAD").as_posix()
        tag = (self.source / ".git" / "refs" / "tags" / "v0.90.0").as_posix()
        self.assertIn(head, default)
        self.assertNotIn(tag, default)
        self.assertFalse(any("/refs" in path for path in default))
        self.assertLessEqual(len(default), 4)
        self.assertIn(tag, expanded)
