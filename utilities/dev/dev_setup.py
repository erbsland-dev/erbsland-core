# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import os
import subprocess
import sys
from collections.abc import Callable, Sequence
from pathlib import Path
from typing import TextIO

from lib.error import UtilityError
from lib.safe_tool import safe_subprocess_environment
from lib.utility import UtilityApp

CommandRunner = Callable[[Sequence[str], Path], None]


class DevSetupRunner:
    """Set up the local development environment."""

    def __init__(
        self,
        project_root: Path,
        *,
        command_runner: CommandRunner | None = None,
        output_stream: TextIO | None = None,
    ) -> None:
        self.project_root = project_root
        self.command_runner = command_runner or self.run_subprocess
        self.output_stream = output_stream or sys.stdout

    @property
    def venv_directory(self) -> Path:
        """The project virtual environment directory."""
        return self.project_root / ".venv"

    @property
    def venv_python(self) -> Path:
        """The virtual environment Python executable."""
        if os.name == "nt":
            return self.venv_directory / "Scripts" / "python.exe"
        return self.venv_directory / "bin" / "python"

    def print_progress(self, message: str) -> None:
        """Print a progress message."""
        print(message, file=self.output_stream, flush=True)

    @staticmethod
    def run_subprocess(command: Sequence[str], cwd: Path) -> None:
        """Run one setup command."""
        subprocess.run(command, check=True, cwd=cwd, env=safe_subprocess_environment())

    def run_command(self, command: Sequence[str], *, cwd: Path | None = None, label: str) -> None:
        """Run one setup command and turn failures into utility errors."""
        try:
            self.command_runner(command, cwd or self.project_root)
        except FileNotFoundError as error:
            executable = Path(command[0]).name
            raise UtilityError(f"{label} failed: could not find `{executable}` in PATH.") from error
        except subprocess.CalledProcessError as error:
            raise UtilityError(f"{label} failed with exit code {error.returncode}.") from error

    def create_virtual_environment(self) -> None:
        """Create the virtual environment when it is missing."""
        if self.venv_directory.exists():
            if not self.venv_directory.is_dir():
                raise UtilityError(f"Virtual environment path exists but is not a directory: {self.venv_directory}")
            self.print_progress("Reusing existing .venv virtual environment.")
            return
        self.print_progress("Creating .venv virtual environment...")
        self.run_command(
            [sys.executable, "-m", "venv", str(self.venv_directory)],
            label="Creating .venv virtual environment",
        )

    def install_requirements(self) -> None:
        """Install pinned Python development and documentation requirements."""
        if not self.venv_python.is_file():
            raise UtilityError(f"Virtual environment Python is missing: {self.venv_python}")
        self.print_progress("Installing development and documentation requirements...")
        self.run_command(
            [
                str(self.venv_python),
                "-m",
                "pip",
                "install",
                "-r",
                "utilities/requirements-dev.txt",
                "-r",
                "doc/requirements-doc.txt",
            ],
            label="Installing Python requirements",
        )

    def initialize_submodules(self) -> None:
        """Initialize the required git submodules."""
        self.print_progress("Initializing git submodules...")
        self.run_command(
            [
                "git",
                "-C",
                str(self.project_root),
                "submodule",
                "update",
                "--init",
                "--recursive",
                "test/erbsland-unittest",
            ],
            label="Initializing git submodules",
        )

    def configure_cmake(self) -> None:
        """Configure the debug CMake build tree."""
        self.print_progress("Configuring cmake-build-debug...")
        self.run_command(
            [
                "cmake",
                "-S",
                ".",
                "-B",
                "cmake-build-debug",
                "-G",
                "Ninja",
                "-DCMAKE_BUILD_TYPE=Debug",
            ],
            label="Configuring CMake",
        )

    def run(self) -> None:
        """Run the full development setup."""
        self.create_virtual_environment()
        self.install_requirements()
        self.initialize_submodules()
        self.configure_cmake()
        self.print_progress("Development environment setup completed.")


class DevSetupApp(UtilityApp):
    """Set up a fresh development worktree."""

    description = "Set up a fresh development worktree."

    def run(self, argv=None) -> None:
        """Run the development environment setup."""
        super().run(argv)
        DevSetupRunner(self.project_directory).run()


def main() -> None:
    """Main entry point for setting up a development worktree."""
    raise SystemExit(DevSetupApp().main())


if __name__ == "__main__":
    main()
