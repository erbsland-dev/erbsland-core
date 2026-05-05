# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

from lib.error import UtilityError
from lib.path_safety import require_safe_existing_file
from lib.safe_tool import safe_subprocess_environment
from lib.utility import UtilityApp


class DocumentationOutputFilter:
    """Filter Sphinx and Doxygen output to project-relevant diagnostics."""

    _sphinx_diagnostic = re.compile(r"^[^:\s][^:]*:\d+(?::\d+)?:\s+(?:WARNING|ERROR):\s+")
    _unknown_command = re.compile(r"\b(?:unexpanded alias|unknown command)\b", re.IGNORECASE)
    _constructor_warning = re.compile(
        r"Member\s+(?P<name>[A-Za-z_][A-Za-z_0-9]*)\s*\((?P<arguments>[^)]*)\)"
        r"\s*(?:noexcept(?:\([^)]*\))?\s*)?(?:=\s*(?:default|delete))?\s*\(function\)"
    )

    def __init__(self, project_root: Path) -> None:
        self.project_root = project_root.resolve()

    def filter_line(self, line: str) -> str | None:
        """Return a normalized output line, or None if it should be hidden."""
        if not self.should_show(line):
            return None
        return self.shorten_paths(line)

    def should_show(self, line: str) -> bool:
        """Decide if a Sphinx or Doxygen output line is relevant."""
        if self._sphinx_diagnostic.search(self.shorten_sphinx_paths(line)):
            return True
        if self.is_doxygen_error(line):
            return True
        if self.is_doxygen_unexpanded_alias_warning(line):
            return True
        return self.is_relevant_doxygen_undocumented_warning(line)

    def is_doxygen_error(self, line: str) -> bool:
        """Detect Doxygen error lines."""
        return "error:" in line

    def is_doxygen_unexpanded_alias_warning(self, line: str) -> bool:
        """Detect warnings caused by aliases that Doxygen did not expand."""
        return "warning:" in line and bool(self._unknown_command.search(line))

    def is_relevant_doxygen_undocumented_warning(self, line: str) -> bool:
        """Detect relevant Doxygen warnings about undocumented API entries."""
        if "warning:" not in line:
            return False
        lower_line = line.lower()
        if "is not documented" not in lower_line and "no documented entries" not in lower_line:
            return False
        if "::impl::" in line or "/impl/" in line:
            return False
        if "std::hash" in line or "std::format" in line:
            return False
        if self.is_operator_warning(line):
            return False
        if self.is_copy_or_move_constructor_warning(line):
            return False
        return True

    def is_operator_warning(self, line: str) -> bool:
        """Detect undocumented operator overload warnings."""
        return "operator" in line

    def is_copy_or_move_constructor_warning(self, line: str) -> bool:
        """Detect undocumented copy/move constructor warnings."""
        match = self._constructor_warning.search(line)
        if match is None:
            return False
        class_name = match.group("name")
        arguments = " ".join(match.group("arguments").split())
        return arguments in {f"const {class_name} &", f"{class_name} &&"}

    def shorten_paths(self, line: str) -> str:
        """Shorten generated Doxygen and Sphinx documentation paths."""
        line = self.shorten_doxygen_paths(line)
        return self.shorten_sphinx_paths(line)

    def shorten_doxygen_paths(self, line: str) -> str:
        """Shorten absolute Doxygen input paths."""
        doxygen_input = self.project_root / "_doxygen_input"
        line = line.replace(f"{doxygen_input}/", "")
        return re.sub(r"(?P<prefix>^|\s)(?:[A-Za-z]:)?/.*?/_doxygen_input/", r"\g<prefix>", line)

    def shorten_sphinx_paths(self, line: str) -> str:
        """Shorten absolute Sphinx documentation paths."""
        doc_dir = self.project_root / "doc"
        line = line.replace(f"{doc_dir}/", "")
        return re.sub(r"(?P<prefix>^|\s)(?:[A-Za-z]:)?/.*?/erbsland-core/doc/", r"\g<prefix>", line)


class RebuildDocApp(UtilityApp):
    """Safely rebuild the Sphinx documentation."""

    description = "Remove generated documentation output and rebuild the Sphinx documentation."

    def __init__(self) -> None:
        super().__init__()
        self.project_root = Path()
        self.force = False
        self.filter_output = True

    def add_command_line_args(self, parser: argparse.ArgumentParser) -> None:
        parser.add_argument("--force", action="store_true", help="Remove all generated documentation input/output.")
        parser.add_argument("--no-filter", action="store_true", help="Show the complete Sphinx and Doxygen output.")

    def handle_command_line_args(self, args) -> None:
        self.project_root = self.project_directory
        self.force = args.force
        self.filter_output = not args.no_filter

    def generated_directory(self, name: str) -> Path:
        """Get a generated documentation directory below the project root."""
        if name not in {"_doxygen_input", "_build"}:
            raise UtilityError(f"Unexpected generated documentation directory: {name}")
        project_root = self.project_root.resolve()
        path = (project_root / name).resolve(strict=False)
        if path.parent != project_root:
            raise UtilityError(f"Generated documentation directory is not below the project root: {path}")
        return path

    def remove_generated_directory(self, name: str) -> None:
        """Remove one generated documentation directory if it exists."""
        path = self.generated_directory(name)
        if path.is_symlink():
            raise UtilityError(f"Refusing to remove symbolic link: {path}")
        if not path.exists():
            self.print_verbose(f"Skipping missing {name}")
            return
        if not path.is_dir():
            raise UtilityError(f"Refusing to remove non-directory path: {path}")
        if self.verbose:
            print(f"Removing {name}", flush=True)
        for attempt in range(3):
            try:
                shutil.rmtree(path)
                return
            except OSError as error:
                if attempt == 2:
                    raise UtilityError(f"Could not remove generated documentation directory: {path}") from error
                time.sleep(0.2)

    def sphinx_build_executable(self) -> Path:
        """Get the sphinx-build executable from the project virtual environment."""
        executable = self.project_root / ".venv" / "bin" / "sphinx-build"
        require_safe_existing_file(executable, "sphinx-build executable", 1024 * 1024)
        if not executable.exists():
            raise UtilityError(f"Could not find sphinx-build in the project virtual environment: {executable}")
        if not os.access(executable, os.X_OK):
            raise UtilityError(f"sphinx-build is not executable: {executable}")
        return executable

    def sphinx_environment(self) -> dict[str, str]:
        """Create an environment equivalent to activating the project virtual environment."""
        venv_dir = self.project_root / ".venv"
        venv_bin_dir = venv_dir / "bin"
        environment = safe_subprocess_environment()
        path = environment.get("PATH", "")
        environment["PATH"] = f"{venv_bin_dir}{os.pathsep}{path}" if path else str(venv_bin_dir)
        environment["VIRTUAL_ENV"] = str(venv_dir)
        return environment

    def rebuild_documentation(self) -> None:
        """Run sphinx-build to rebuild the documentation."""
        executable = self.sphinx_build_executable()
        command = [str(executable), "doc", "_build"]
        if self.verbose:
            print("Running sphinx-build doc _build", flush=True)
        if self.filter_output:
            return_code = self.run_sphinx_build_filtered(command)
            status = self.sphinx_status_line(return_code)
            if return_code != 0:
                raise UtilityError(status)
            print(status, flush=True)
            return
        try:
            subprocess.run(command, check=True, cwd=self.project_root, env=self.sphinx_environment())
        except subprocess.CalledProcessError as error:
            raise UtilityError("sphinx-build failed.") from error

    def run_sphinx_build_filtered(self, command: list[str]) -> int:
        """Run sphinx-build and display only relevant diagnostics."""
        output_filter = DocumentationOutputFilter(self.project_root)
        process = subprocess.Popen(
            command,
            cwd=self.project_root,
            env=self.sphinx_environment(),
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            errors="replace",
            bufsize=1,
        )
        if process.stdout is not None:
            for line in process.stdout:
                filtered_line = output_filter.filter_line(line.rstrip("\n\r"))
                if filtered_line is not None:
                    print(filtered_line, flush=True)
        return process.wait()

    def sphinx_status_line(self, return_code: int) -> str:
        """Create the final Sphinx status line."""
        if return_code == 0:
            return "sphinx-build completed successfully."
        return f"sphinx-build failed with exit code {return_code}."

    def preprocess_sources(self) -> None:
        """Run the standalone Doxygen source preprocessing tool."""
        script = self.project_root / "doc" / "_tools" / "preprocess_sources.py"
        require_safe_existing_file(script, "preprocess_sources.py", 1024 * 1024)
        command = [sys.executable, str(script)]
        if self.force:
            command.append("--force")
        if self.verbose:
            print("Running doc/_tools/preprocess_sources.py", flush=True)
        try:
            if self.filter_output:
                subprocess.run(
                    command,
                    check=True,
                    cwd=self.project_root,
                    env=self.sphinx_environment(),
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    text=True,
                    errors="replace",
                )
                return
            subprocess.run(command, check=True, cwd=self.project_root, env=self.sphinx_environment())
        except subprocess.CalledProcessError as error:
            if self.filter_output and error.stdout:
                print(error.stdout, end="" if error.stdout.endswith("\n") else "\n")
            raise UtilityError("preprocess_sources.py failed.") from error

    def run(self, argv=None) -> None:
        """Remove generated output and rebuild the documentation."""
        super().run(argv)
        if self.filter_output:
            print("Filtered output (use --no-filter to show full tool output).", flush=True)
        if self.force:
            self.remove_generated_directory("_doxygen_input")
        self.remove_generated_directory("_build")
        self.preprocess_sources()
        self.rebuild_documentation()
        print("Documentation rebuild completed.")


def main() -> None:
    """Main entry point for rebuilding documentation."""
    raise SystemExit(RebuildDocApp().main())


if __name__ == "__main__":
    main()
