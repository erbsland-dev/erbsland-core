# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import os
import shutil
import subprocess
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from hashlib import sha256
from pathlib import Path

from dev.cleanup import CleanupApp
from dev.fix_include_paths import FixIncludePathsApp
from dev.reference_doc import ReferenceDocApp
from dev.security_hashes import SecurityHashesApp
from dev.update_includes import UpdateIncludesApp
from lib.config import read_elcl_file, validate_file_suffixes
from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.path_safety import (
    require_directory,
    require_safe_existing_file,
    require_safe_parent_directory,
    resolve_project_path,
)
from lib.safe_tool import safe_subprocess_environment
from lib.utility import UtilityApp


@dataclass(frozen=True)
class GitLocalState:
    """A snapshot of local git state relevant for pre-commit checks."""

    status: bytes
    unstaged_diff: bytes
    staged_diff: bytes
    untracked_files: tuple[tuple[str, str, str], ...]


class PreCommitApp(UtilityApp):
    """Run pre-commit normalization and checks."""

    description = "Run pre-commit normalization and checks on the codebase."

    def __init__(self) -> None:
        super().__init__()
        self.project_root = Path()
        self.clang_format_marker_file = ""
        self.clang_format_full_run_interval_seconds = 0
        self.clang_format_extensions: set[str] = set()
        self.clang_format_source_dirs: list[Path] = []
        self.clang_format_executable_path = ""

    def handle_command_line_args(self, args) -> None:
        self.project_root = self.project_directory

    def read_config(self) -> None:
        """
        Read the pre-commit configuration.
        """
        self.print_verbose("Reading the configuration")
        config = read_elcl_file(self.config_file_path())
        clang_format_config = config["clang_format"]
        self.clang_format_marker_file = clang_format_config.get_text("marker_file", default="")
        self.clang_format_full_run_interval_seconds = self.time_delta_to_seconds(
            clang_format_config.get_time_delta("full_run_interval_seconds")
        )
        self.clang_format_extensions = set(clang_format_config.get_list("extensions", str, default=[]))
        self.clang_format_source_dirs = [
            resolve_project_path(self.project_root, directory, "Clang Format Directories")
            for directory in clang_format_config.get_list("directories", str, default=[])
        ]
        self.clang_format_executable_path = self.find_clang_format_executable()
        self.validate_config()

    def find_clang_format_executable(self) -> str:
        """Find a clang-format executable that is not supplied by this project tree."""
        executable = shutil.which("clang-format")
        if executable is None:
            raise UtilityError("Could not find clang-format in PATH.")
        executable_path = Path(executable).resolve()
        if executable_path == self.project_root or executable_path.is_relative_to(self.project_root):
            raise UtilityError(f"Refusing to run clang-format from inside the project tree: {executable_path}")
        return str(executable_path)

    @staticmethod
    def time_delta_to_seconds(time_delta) -> float:
        """
        Convert an ELCL time delta into seconds.

        :param time_delta: The time-delta value from `erbsland-conf`.
        :return: The number of seconds.
        """
        unit_seconds = {
            "nanosecond": 0.000000001,
            "microsecond": 0.000001,
            "millisecond": 0.001,
            "second": 1,
            "minute": 60,
            "hour": 60 * 60,
            "day": 24 * 60 * 60,
            "week": 7 * 24 * 60 * 60,
            "month": 30 * 24 * 60 * 60,
            "year": 365 * 24 * 60 * 60,
        }
        unit = time_delta.unit.value
        if unit not in unit_seconds:
            raise UtilityError(f"Unsupported time unit for clang-format interval: {unit}")
        return time_delta.count * unit_seconds[unit]

    def validate_config(self) -> None:
        """
        Validate the pre-commit configuration.
        """
        if not self.clang_format_marker_file:
            raise UtilityError("No clang-format marker file configured.")
        if self.clang_format_full_run_interval_seconds <= 0:
            raise UtilityError("The clang-format full run interval must be greater than zero.")
        if not self.clang_format_extensions:
            raise UtilityError("No clang-format extensions configured.")
        validate_file_suffixes(self.clang_format_extensions, "Clang Format Extensions")
        if not self.clang_format_source_dirs:
            raise UtilityError("No clang-format source directories configured.")
        for directory in self.clang_format_source_dirs:
            if directory.exists():
                require_directory(directory, "Clang Format Directories")

    def clang_format_directories(self) -> list[Path]:
        """Get the directories that contain C++ files for clang-format."""
        return self.clang_format_source_dirs

    def find_cpp_files(self) -> list[Path]:
        """Find C++ source files in the configured source directories."""
        cpp_files = []
        for directory in self.clang_format_directories():
            if not directory.exists():
                continue
            for file in directory.rglob("*"):
                if file.suffix not in self.clang_format_extensions:
                    continue
                require_safe_existing_file(file, "clang-format source file", FileUpdate.MAX_COMPARE_FILE_SIZE)
                cpp_files.append(file)
        return sorted(cpp_files)

    def clang_format_marker_path(self) -> Path:
        """Get the timestamp marker file used to skip unchanged files."""
        return resolve_project_path(self.project_root, self.clang_format_marker_file, "Clang Format Marker File")

    def should_run_full_clang_format(self, marker_path: Path) -> bool:
        """Test if clang-format should run over all source files."""
        if not marker_path.exists():
            return True
        return (time.time() - marker_path.stat().st_mtime) > self.clang_format_full_run_interval_seconds

    @staticmethod
    def touch_clang_format_marker(marker_path: Path) -> None:
        """Update the clang-format run marker file."""
        require_safe_existing_file(marker_path, "clang-format marker file", 1024 * 1024)
        require_safe_parent_directory(marker_path, "clang-format marker file")
        timestamp = datetime.now(timezone.utc).isoformat(timespec="seconds")
        marker_path.write_text(f"{timestamp}\n", encoding="utf-8")

    def select_files_to_format(self, cpp_files: list[Path], marker_path: Path, full_run: bool) -> list[Path]:
        """Select all files for a full run, or only files newer than the marker."""
        if full_run:
            return cpp_files
        marker_mtime = marker_path.stat().st_mtime if marker_path.exists() else 0
        return [file for file in cpp_files if file.stat().st_mtime > marker_mtime]

    def print_clang_format_scope(self, full_run: bool) -> None:
        """Print which clang-format mode is being used."""
        if full_run:
            print("Running clang-format for all source files...")
        else:
            print("Running clang-format for source files changed since the last run...")

    def display_path(self, path: Path) -> Path:
        """Create a user-friendly display path relative to the project."""
        try:
            return path.relative_to(self.project_root)
        except ValueError:
            return path

    def format_file(self, path: Path) -> None:
        """Run clang-format for one file."""
        display_path = self.display_path(path)
        self.print_verbose(f"  Formatting {display_path}")
        try:
            subprocess.run(
                [self.clang_format_executable_path, "-i", str(path)],
                check=True,
                env=safe_subprocess_environment(),
            )
        except subprocess.CalledProcessError as error:
            raise UtilityError(f"clang-format failed for {display_path}.") from error

    def git_output(self, *args: str) -> bytes:
        """Run a git command and return its standard output."""
        try:
            result = subprocess.run(
                ["git", "-C", str(self.project_root), *args],
                check=True,
                capture_output=True,
                env=safe_subprocess_environment(),
            )
        except subprocess.CalledProcessError as error:
            message = error.stderr.decode("utf-8", errors="replace").strip()
            if not message:
                message = str(error)
            raise UtilityError(f"git {' '.join(args)} failed: {message}") from error
        return result.stdout

    def untracked_file_fingerprints(self) -> tuple[tuple[str, str, str], ...]:
        """Create content fingerprints for untracked files."""
        output = self.git_output("ls-files", "--others", "--exclude-standard", "-z")
        result = []
        for path_bytes in output.split(b"\0"):
            if not path_bytes:
                continue
            relative_path = os.fsdecode(path_bytes)
            path = self.project_root / relative_path
            if path.is_symlink():
                result.append((relative_path, "symlink", os.readlink(path)))
                continue
            if not path.exists():
                result.append((relative_path, "missing", ""))
                continue
            if not path.is_file():
                result.append((relative_path, "other", ""))
                continue
            digest = sha256(path.read_bytes()).hexdigest()
            result.append((relative_path, "file", digest))
        return tuple(sorted(result, key=lambda item: item[0].casefold()))

    def git_local_state(self) -> GitLocalState:
        """Capture the local git state before or after pre-commit tasks."""
        return GitLocalState(
            status=self.git_output("status", "--porcelain=v1", "-z", "--untracked-files=all"),
            unstaged_diff=self.git_output("diff", "--binary", "--no-ext-diff"),
            staged_diff=self.git_output("diff", "--cached", "--binary", "--no-ext-diff"),
            untracked_files=self.untracked_file_fingerprints(),
        )

    def require_unchanged_git_state(self, initial_state: GitLocalState) -> None:
        """Stop when pre-commit tasks changed the local git state."""
        final_state = self.git_local_state()
        if final_state == initial_state:
            return
        status = self.git_output("status", "--short", "--untracked-files=all").decode("utf-8", errors="replace")
        message = "Pre-commit made changes. Review the changes, then run pre_commit again."
        if status.strip():
            message += f"\n\nCurrent git status:\n{status.rstrip()}"
        raise UtilityError(message)

    def run_clang_format(self) -> None:
        """Run clang-format on configured source directories."""
        marker_path = self.clang_format_marker_path()
        full_run = self.should_run_full_clang_format(marker_path)
        cpp_files = self.find_cpp_files()
        files_to_format = self.select_files_to_format(cpp_files, marker_path, full_run)

        self.print_clang_format_scope(full_run)
        for file in files_to_format:
            self.format_file(file)
        self.touch_clang_format_marker(marker_path)
        print(f"Formatted {len(files_to_format)} of {len(cpp_files)} source files.")
        print("clang-format completed.")

    def run_app(self, app: UtilityApp, name: str) -> None:
        """Run a child utility application."""
        print(f"Running {name}...")
        arguments = []
        if self.verbose:
            arguments.append("--verbose")
        if self.private_config:
            arguments.append("--private-config")
        app.run(arguments)
        print(f"{name} completed.")

    def run(self, argv=None) -> None:
        """Run all pre-commit tasks."""
        super().run(argv)
        self.read_config()
        os.chdir(self.project_root)
        initial_git_state = self.git_local_state()
        try:
            self.run_clang_format()
            self.run_app(UpdateIncludesApp(), "update_includes")
            self.run_app(FixIncludePathsApp(), "fix_include_paths")
            self.run_app(CleanupApp(), "cleanup")
            self.run_app(ReferenceDocApp(), "reference_doc")
            self.run_app(SecurityHashesApp(), "security_hashes")
        finally:
            self.require_unchanged_git_state(initial_git_state)

        print("All pre-commit tasks completed successfully.")


def main():
    """Main entry point for pre-commit tasks."""
    raise SystemExit(PreCommitApp().main())


if __name__ == "__main__":
    main()
