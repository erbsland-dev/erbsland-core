# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import argparse
import hashlib
import re
import shlex
import stat
import subprocess
import threading
from dataclasses import dataclass
from pathlib import Path

from lib.config import validate_source_relative_path
from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.path_safety import read_safe_text, require_directory, require_safe_existing_file
from lib.safe_tool import safe_subprocess_environment
from lib.utility import UtilityApp

MAX_DEMO_SOURCE_SIZE = 1024 * 1024
MAX_DEMO_OUTPUT_SIZE = 1024 * 1024
MAX_DEMO_OUTPUT_LINES = 100
DEMO_TIMEOUT_SECONDS = 10
EXECUTABLE_NAME_RE = re.compile(r"^[A-Za-z0-9_-]+$")
ARGUMENT_RE = re.compile(r"^[A-Za-z0-9_-]+$")


class DemoDocError(Exception):
    """A recoverable synchronization error for a single demo block."""


@dataclass(frozen=True)
class DemoBlock:
    """One parsed `erbsland-demo` block."""

    start: int
    end: int
    indent: str
    options: dict[str, str]


@dataclass(frozen=True)
class GeneratedBlock:
    """Generated text and recoverable issues for one demo block."""

    text: str
    issues: tuple[str, ...]


def sha256_bytes(data: bytes) -> str:
    """Create a SHA-256 hash for bytes."""
    return hashlib.sha256(data).hexdigest()


class DemoExecutor:
    """Run demo executables through a narrow allowlist."""

    def __init__(self, project_dir: Path) -> None:
        self.project_dir = project_dir
        self.demo_output_dir = project_dir / "cmake-build-debug" / "demo-apps"

    def validate_command(self, command_text: str) -> list[str]:
        """Parse and validate one `:exec:` command."""
        try:
            parts = shlex.split(command_text, posix=True)
        except ValueError as error:
            raise DemoDocError(f"Could not parse demo command: {error}") from None
        if not parts:
            raise DemoDocError("Demo command is empty.")
        executable_name = parts[0]
        if EXECUTABLE_NAME_RE.fullmatch(executable_name) is None:
            raise DemoDocError(f"Demo executable name is not allowed: {executable_name!r}")
        for argument in parts[1:]:
            if ARGUMENT_RE.fullmatch(argument) is None:
                raise DemoDocError(f"Demo argument is not allowed: {argument!r}")
        return parts

    def executable_path(self, executable_name: str) -> Path:
        """Resolve and validate the executable path for a demo command."""
        executable_path = self.demo_output_dir / executable_name
        resolved_output_dir = self.demo_output_dir.resolve(strict=False)
        resolved_executable = executable_path.resolve(strict=False)
        if not resolved_executable.is_relative_to(resolved_output_dir):
            raise DemoDocError(f"Demo executable escapes the output directory: {executable_name}")
        if executable_path.is_symlink():
            raise DemoDocError(f"Demo executable must not be a symbolic link: {executable_path}")
        if not executable_path.exists():
            raise DemoDocError(f"Demo executable does not exist: {executable_path}")
        if not executable_path.is_file():
            raise DemoDocError(f"Demo executable is not a regular file: {executable_path}")
        mode = executable_path.stat().st_mode
        if not mode & (stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH):
            raise DemoDocError(f"Demo executable is not executable: {executable_path}")
        return executable_path

    def run(self, command_text: str) -> str:
        """Run a validated demo command and return normalized ANSI text."""
        parts = self.validate_command(command_text)
        executable_path = self.executable_path(parts[0])
        command = [str(executable_path), *parts[1:]]
        process = subprocess.Popen(
            command,
            cwd=self.project_dir,
            env=safe_subprocess_environment(),
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        if process.stdout is None:
            raise DemoDocError("Could not capture demo output.")
        output = bytearray()
        output_limit_error: list[str] = []

        def read_output() -> None:
            while True:
                chunk = process.stdout.read(4096)
                if not chunk:
                    return
                if len(output) + len(chunk) > MAX_DEMO_OUTPUT_SIZE:
                    output_limit_error.append("Demo output exceeds 1 MiB.")
                    process.kill()
                    return
                output.extend(chunk)

        output_thread = threading.Thread(target=read_output, daemon=True)
        output_thread.start()
        try:
            process.wait(timeout=DEMO_TIMEOUT_SECONDS)
        except subprocess.TimeoutExpired:
            process.kill()
            output_thread.join(timeout=1)
            process.stdout.close()
            raise DemoDocError(f"Demo execution timed out after {DEMO_TIMEOUT_SECONDS} seconds.") from None
        output_thread.join(timeout=1)
        process.stdout.close()
        if output_limit_error:
            raise DemoDocError(output_limit_error[0])
        raw_output = bytes(output)
        text = raw_output.decode("utf-8", errors="replace")
        if len(text.splitlines()) > MAX_DEMO_OUTPUT_LINES:
            raise DemoDocError(f"Demo output exceeds {MAX_DEMO_OUTPUT_LINES} lines.")
        if process.returncode != 0:
            excerpt = text.strip().splitlines()
            detail = f" Last output line: {excerpt[-1]}" if excerpt else ""
            raise DemoDocError(f"Demo command failed with exit code {process.returncode}.{detail}")
        return self.convert_output(raw_output)

    @staticmethod
    def convert_output(raw_output: bytes) -> str:
        """Flatten demo output to static ANSI text for the Sphinx ANSI directive."""
        text = raw_output.decode("utf-8", errors="replace")
        try:
            from erbsland.ansi_convert import Terminal
        except ImportError:
            if b"\x1b" in raw_output or b"\r" in raw_output:
                raise DemoDocError(
                    "erbsland-ansi-convert is required for ANSI or cursor-control demo output."
                ) from None
            return text.replace("\r\n", "\n").replace("\r", "\n")
        terminal = Terminal(width=120, height=40, back_buffer_height=2000)
        terminal.write(text, collapse_capture_updates=True)
        return terminal.to_ansi()


class DemoDocSynchronizer:
    """Synchronize managed demo blocks in one reStructuredText document."""

    RE_START = re.compile(r"^(?P<indent>\s*)\.\.\s+erbsland-demo::\s*$")
    RE_END = re.compile(r"^\s*\.\.\s+erbsland-demo-end::\s*$")
    RE_OPTION = re.compile(r"^\s+:(?P<name>source|exec|source-sha256):\s*(?P<value>.*)$")

    def __init__(self, project_dir: Path, *, force: bool = False) -> None:
        self.project_dir = project_dir
        self.force = force
        self.demos_dir = project_dir / "demos"
        self.executor = DemoExecutor(project_dir)

    def parse_blocks(self, path: Path, lines: list[str]) -> list[DemoBlock]:
        """Parse all demo blocks from one document."""
        blocks: list[DemoBlock] = []
        index = 0
        while index < len(lines):
            start_match = self.RE_START.match(lines[index])
            if start_match is None:
                if self.RE_END.match(lines[index]) is not None:
                    raise UtilityError(f"Unexpected erbsland-demo-end directive in {path}:{index + 1}.")
                index += 1
                continue
            end = self.find_block_end(path, lines, index + 1)
            blocks.append(
                DemoBlock(index, end, start_match.group("indent"), self.parse_options(lines[index + 1 : end]))
            )
            index = end + 1
        return blocks

    def find_block_end(self, path: Path, lines: list[str], start: int) -> int:
        """Find the end directive for a block."""
        for index in range(start, len(lines)):
            if self.RE_START.match(lines[index]) is not None:
                raise UtilityError(f"Nested erbsland-demo directive in {path}:{index + 1}.")
            if self.RE_END.match(lines[index]) is not None:
                return index
        raise UtilityError(f"Missing erbsland-demo-end directive in {path}:{start}.")

    def parse_options(self, lines: list[str]) -> dict[str, str]:
        """Parse directive options from a block body."""
        options: dict[str, str] = {}
        in_options = True
        for line in lines:
            if in_options and not line.strip():
                continue
            match = self.RE_OPTION.match(line)
            if in_options and match is not None:
                options[match.group("name")] = match.group("value").strip()
                continue
            in_options = False
        return options

    def process_text(self, path: Path, text: str) -> tuple[str, tuple[str, ...]]:
        """Synchronize all demo blocks in a document."""
        had_final_newline = text.endswith("\n")
        lines = text.splitlines()
        blocks = self.parse_blocks(path, lines)
        if not blocks:
            return text, ()
        result: list[str] = []
        issues: list[str] = []
        cursor = 0
        for block in blocks:
            result.extend(lines[cursor : block.start])
            if not self.block_needs_update(block):
                result.extend(lines[block.start : block.end + 1])
            else:
                generated = self.generate_block(block)
                result.extend(generated.text.splitlines())
                issues.extend(f"{path}:{block.start + 1}: {issue}" for issue in generated.issues)
            cursor = block.end + 1
        result.extend(lines[cursor:])
        updated_text = "\n".join(result)
        if had_final_newline or text == "":
            updated_text += "\n"
        return updated_text, tuple(issues)

    def block_needs_update(self, block: DemoBlock) -> bool:
        """Test if a block must be regenerated."""
        if self.force:
            return True
        source_option = block.options.get("source", "")
        if not source_option:
            return True
        try:
            source_hash = self.source_hash(source_option)
        except DemoDocError:
            return True
        return block.options.get("source-sha256") != source_hash

    def generate_block(self, block: DemoBlock) -> GeneratedBlock:
        """Generate one managed demo block."""
        issues: list[str] = []
        source_option = block.options.get("source", "")
        source_hash = ""
        display_source_lines: list[str] = []
        if not source_option:
            issues.append("Missing required :source: option.")
        else:
            try:
                source_hash, display_source_lines = self.load_display_source(source_option)
            except DemoDocError as error:
                issues.append(str(error))

        lines = [
            f"{block.indent}.. erbsland-demo::",
            f"{block.indent}    :source: {source_option}",
        ]
        exec_option = block.options.get("exec", "")
        if exec_option:
            lines.append(f"{block.indent}    :exec: {exec_option}")
        if source_hash:
            lines.append(f"{block.indent}    :source-sha256: {source_hash}")
        lines.append("")

        if display_source_lines:
            self.append_code_block(lines, block.indent, display_source_lines)
        if exec_option:
            try:
                output = self.executor.run(exec_option)
                if output:
                    self.append_ansi_block(lines, block.indent, output)
            except DemoDocError as error:
                issues.append(str(error))
        if issues:
            self.append_note(lines, block.indent, issues)
        lines.append(f"{block.indent}.. erbsland-demo-end::")
        return GeneratedBlock("\n".join(lines), tuple(issues))

    def load_display_source(self, source_option: str) -> tuple[str, list[str]]:
        """Read and trim the display source for one block."""
        data = self.source_bytes(source_option)
        source_hash = sha256_bytes(data)
        try:
            text = data.decode("utf-8")
        except UnicodeDecodeError:
            raise DemoDocError(f"Demo source is not valid UTF-8: {source_option}") from None
        source_lines = text.splitlines()
        start_index = next((index for index, line in enumerate(source_lines) if line.lstrip().startswith("///")), None)
        if start_index is None:
            raise DemoDocError(f"Demo source has no /// documentation comment: {source_option}")
        display_lines = source_lines[start_index:]
        while display_lines and not display_lines[-1].strip():
            display_lines.pop()
        return source_hash, display_lines

    def source_hash(self, source_option: str) -> str:
        """Read and hash one demo source file."""
        return sha256_bytes(self.source_bytes(source_option))

    def source_bytes(self, source_option: str) -> bytes:
        """Read bytes from one validated demo source file."""
        validate_source_relative_path(source_option, "Demo Source")
        source_path = (self.demos_dir / source_option).resolve(strict=False)
        demos_dir = self.demos_dir.resolve(strict=False)
        if source_path == demos_dir or not source_path.is_relative_to(demos_dir):
            raise DemoDocError(f"Demo source escapes the demos directory: {source_option}")
        if source_path.is_symlink():
            raise DemoDocError(f"Demo source must not be a symbolic link: {source_path}")
        require_safe_existing_file(source_path, "Demo source", MAX_DEMO_SOURCE_SIZE)
        if not source_path.exists():
            raise DemoDocError(f"Demo source does not exist: {source_path}")
        return source_path.read_bytes()

    @staticmethod
    def append_code_block(lines: list[str], indent: str, source_lines: list[str]) -> None:
        """Append a C++ code block."""
        lines.extend([f"{indent}.. code-block:: cpp", ""])
        lines.extend(f"{indent}    {line}" if line else "" for line in source_lines)
        lines.append("")

    @staticmethod
    def append_ansi_block(lines: list[str], indent: str, output: str) -> None:
        """Append an ANSI output block."""
        lines.extend([f"{indent}.. erbsland-ansi::", f"{indent}    :escape-char: ␛", ""])
        escaped_output = output.replace("\x1b", "␛")
        output_lines = escaped_output.splitlines()
        while output_lines and not output_lines[-1]:
            output_lines.pop()
        lines.extend(f"{indent}    {line}" if line else "" for line in output_lines)
        lines.append("")

    @staticmethod
    def append_note(lines: list[str], indent: str, issues: list[str]) -> None:
        """Append recoverable synchronization issues as a note."""
        lines.extend([f"{indent}.. note::", ""])
        lines.append(f"{indent}    Demo synchronization issue:")
        lines.append("")
        for issue in issues:
            for line in issue.splitlines() or [""]:
                lines.append(f"{indent}    {line}")
        lines.append("")


class DemoDocRunner:
    """Run demo-doc validation or synchronization over selected RST files."""

    def __init__(
        self,
        project_dir: Path,
        *,
        mode: str,
        force: bool,
        recursive: bool,
        print_verbose=None,
    ) -> None:
        self.project_dir = project_dir.resolve()
        self.mode = mode
        self.force = force
        self.recursive = recursive
        self.print_verbose = print_verbose
        self.file_update = FileUpdate(print_verbose)
        self.synchronizer = DemoDocSynchronizer(self.project_dir, force=force)
        self.changed_files: list[Path] = []
        self.issues: list[str] = []

    def display_path(self, path: Path) -> str:
        """Create a project-relative display path where possible."""
        try:
            return path.relative_to(self.project_dir).as_posix()
        except ValueError:
            return path.as_posix()

    def paths_for_target(self, target: Path) -> list[Path]:
        """Collect RST files for a target."""
        target = target.resolve(strict=False)
        if target != self.project_dir and not target.is_relative_to(self.project_dir):
            raise UtilityError(f"Target escapes the project root: {target}")
        if target.is_file():
            if target.suffix != ".rst":
                raise UtilityError(f"Target file is not an .rst file: {target}")
            require_safe_existing_file(target, "reStructuredText file", FileUpdate.MAX_COMPARE_FILE_SIZE)
            return [target]
        require_directory(target, "Target Directory")
        iterator = target.rglob("*.rst") if self.recursive else target.glob("*.rst")
        return sorted(
            (path for path in iterator if path.is_file() and not path.is_symlink()), key=lambda path: path.as_posix()
        )

    def process_file(self, path: Path) -> None:
        """Process one RST file."""
        if self.print_verbose is not None:
            self.print_verbose(f"Processing file: {self.display_path(path)}")
        original_text = read_safe_text(path, "reStructuredText file", FileUpdate.MAX_COMPARE_FILE_SIZE)
        updated_text, issues = self.synchronizer.process_text(path, original_text)
        self.issues.extend(issues)
        if updated_text == original_text:
            return
        self.changed_files.append(path)
        if self.mode == "validate":
            print(f"needs sync: {self.display_path(path)}")
            return
        self.file_update.write_if_changed(path, updated_text)

    def run(self, target: Path) -> None:
        """Run the selected mode."""
        for path in self.paths_for_target(target):
            self.process_file(path)
        for issue in self.issues:
            print(f"warning: {issue}")
        if self.mode == "validate" and self.changed_files:
            raise UtilityError("Demo documentation is not synchronized.")
        if self.issues:
            raise UtilityError("Demo documentation has synchronization issues.")
        if self.mode == "validate" and not self.changed_files:
            print("Demo documentation is synchronized.")


class DemoDocApp(UtilityApp):
    """Validate or synchronize managed documentation demo blocks."""

    description = "Validate or synchronize erbsland-demo documentation blocks."

    def __init__(self) -> None:
        super().__init__()
        self.project_dir = Path()
        self.mode = "validate"
        self.force = False
        self.recursive = False
        self.target = Path()

    def add_command_line_args(self, parser: argparse.ArgumentParser) -> None:
        parser.add_argument("mode", choices=("validate", "sync"), help="Validate or synchronize demo blocks.")
        parser.add_argument("-r", "--recursive", action="store_true", help="Process directories recursively.")
        parser.add_argument("--force", action="store_true", help="Regenerate blocks even when the source hash matches.")
        parser.add_argument("target", type=Path, help="An .rst file or directory to process.")

    def handle_command_line_args(self, args: argparse.Namespace) -> None:
        self.project_dir = self.project_directory
        self.mode = args.mode
        self.force = args.force
        self.recursive = args.recursive
        self.target = args.target

    def run(self, argv=None) -> None:
        """Run the app."""
        super().run(argv)
        DemoDocRunner(
            self.project_dir,
            mode=self.mode,
            force=self.force,
            recursive=self.recursive,
            print_verbose=self.print_verbose,
        ).run(self.target)


def main() -> None:
    """Main entry point."""
    raise SystemExit(DemoDocApp().main())


if __name__ == "__main__":
    main()
