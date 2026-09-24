# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import argparse
import errno
import hashlib
import os
import re
import shlex
import shutil
import stat
import struct
import subprocess
import tempfile
import threading
import textwrap
from dataclasses import dataclass
from pathlib import Path

from lib.config import validate_source_relative_path
from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.path_safety import read_safe_text, require_directory, require_safe_existing_file
from lib.safe_tool import safe_subprocess_environment
from lib.utility import UtilityApp

if os.name == "posix":
    import fcntl
    import pty
    import termios

MAX_DEMO_SOURCE_SIZE = 1024 * 1024
MAX_DEMO_OUTPUT_SIZE = 1024 * 1024
MAX_DEMO_OUTPUT_LINES = 100
DEMO_TIMEOUT_SECONDS = 10
DEMO_TERMINAL_WIDTH = 90
DEMO_TERMINAL_HEIGHT = 40
EXECUTABLE_NAME_RE = re.compile(r"^[A-Za-z0-9_-]+(?:/[A-Za-z0-9_-]+)*$")
ARGUMENT_RE = re.compile(r"^[A-Za-z0-9_-]+$")
BRANCH_ARGUMENT_RE = re.compile(r"^[A-Za-z0-9_-]+(?:\.[A-Za-z0-9_-]+)+$")
FIXTURE_ARGUMENT_RE = re.compile(r"^demos/[A-Za-z0-9_.-]+(?:/[A-Za-z0-9_.-]+)+$")
PLACEHOLDER_RE = re.compile(r"^\{(?P<kind>file|directory):(?P<value>[^{}]+)\}$")
DIRECTORY_PLACEHOLDER_NAME_RE = re.compile(r"^[-_a-zA-Z0-9]{1,64}$")
FILE_PLACEHOLDER_TYPES = frozenset(("empty", "text", "none"))
TEXT_PLACEHOLDER_CONTENT = (
    "This is the first line of demo text.\n"
    "This is the second line of demo text.\n"
    "This is the third line of demo text.\n"
    "This is the fourth line of demo text.\n"
    "This is the fifth line of demo text.\n"
)


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


@dataclass(frozen=True)
class DemoPlaceholder:
    """One safe temporary path placeholder in a demo command."""

    kind: str
    value: str
    index: int


@dataclass(frozen=True)
class DemoFile:
    """One document-relative file available to demo commands by basename."""

    name: str
    configured_path: str
    path: Path


@dataclass(frozen=True)
class DemoRun:
    """One command execution configured for a demo block."""

    index: int
    command_text: str
    expected_exit_code: int = 0

    @property
    def exec_option_name(self) -> str:
        """Get the directive option name for this run command."""
        return "exec" if self.index == 1 else f"exec-{self.index}"

    @property
    def exit_code_option_name(self) -> str:
        """Get the directive option name for this run's expected exit code."""
        return "exec-exit-code" if self.index == 1 else f"exec-{self.index}-exit-code"


def sha256_bytes(data: bytes) -> str:
    """Create a SHA-256 hash for bytes."""
    return hashlib.sha256(data).hexdigest()


class DemoExecutor:
    """Run demo executables through a narrow allowlist."""

    def __init__(self, project_dir: Path) -> None:
        self.project_dir = project_dir
        self.demo_output_dir = project_dir / "cmake-build-debug" / "demo-apps"

    def validate_command(self, command_text: str, demo_files: dict[str, DemoFile] | None = None) -> list[str]:
        """Parse and validate one `:exec:` command."""
        demo_files = demo_files or {}
        try:
            parts = shlex.split(command_text, posix=True)
        except ValueError as error:
            raise DemoDocError(f"Could not parse demo command: {error}") from None
        if not parts:
            raise DemoDocError("Demo command is empty.")
        executable_name = parts[0]
        if EXECUTABLE_NAME_RE.fullmatch(executable_name) is None:
            raise DemoDocError(f"Demo executable name is not allowed: {executable_name!r}")
        previous_argument = ""
        for argument in parts[1:]:
            if ARGUMENT_RE.fullmatch(argument) is not None:
                previous_argument = argument
                continue
            if previous_argument == "--branch" and BRANCH_ARGUMENT_RE.fullmatch(argument) is not None:
                previous_argument = argument
                continue
            if self.fixture_path(argument) is not None:
                previous_argument = argument
                continue
            if argument in demo_files:
                previous_argument = argument
                continue
            if self.parse_placeholder(argument, 0) is None:
                raise DemoDocError(f"Demo argument is not allowed: {argument!r}")
            previous_argument = argument
        return parts

    def fixture_path(self, argument: str) -> Path | None:
        """Resolve an existing read-only demo fixture argument."""
        if FIXTURE_ARGUMENT_RE.fullmatch(argument) is None:
            return None
        try:
            validate_source_relative_path(argument, "Demo fixture")
        except UtilityError as error:
            raise DemoDocError(str(error)) from None
        path = self.project_dir / argument
        try:
            require_safe_existing_file(path, "Demo fixture")
        except UtilityError as error:
            raise DemoDocError(str(error)) from None
        if not path.exists():
            raise DemoDocError(f"Demo fixture does not exist: {path}")
        return path

    @staticmethod
    def parse_placeholder(argument: str, index: int) -> DemoPlaceholder | None:
        """Parse and validate one whole-argument placeholder."""
        match = PLACEHOLDER_RE.fullmatch(argument)
        if match is None:
            return None
        kind = match.group("kind")
        value = match.group("value")
        if kind == "file":
            if value not in FILE_PLACEHOLDER_TYPES:
                raise DemoDocError(f"Demo file placeholder type is not allowed: {value!r}")
        elif kind == "directory":
            if DIRECTORY_PLACEHOLDER_NAME_RE.fullmatch(value) is None:
                raise DemoDocError(f"Demo directory placeholder name is not allowed: {value!r}")
        return DemoPlaceholder(kind, value, index)

    def expand_placeholders(self, arguments: list[str], temporary_dir: Path) -> list[str]:
        """Replace placeholder arguments with safe temporary paths."""
        expanded_arguments: list[str] = []
        directory_paths: dict[str, Path] = {}
        file_index = 0
        for argument in arguments:
            placeholder = self.parse_placeholder(argument, file_index)
            if placeholder is None:
                expanded_arguments.append(argument)
                continue
            if placeholder.kind == "file":
                file_index += 1
                expanded_arguments.append(str(self.create_placeholder_file(temporary_dir, placeholder)))
            elif placeholder.kind == "directory":
                directory_path = directory_paths.get(placeholder.value)
                if directory_path is None:
                    directory_path = temporary_dir / placeholder.value
                    directory_path.mkdir()
                    directory_paths[placeholder.value] = directory_path
                expanded_arguments.append(str(directory_path))
        return expanded_arguments

    def copy_fixture_arguments(self, arguments: list[str], temporary_dir: Path) -> list[str]:
        """Replace demo fixture arguments with temporary read-only copies."""
        expanded_arguments: list[str] = []
        fixture_index = 0
        for argument in arguments:
            fixture_path = self.fixture_path(argument)
            if fixture_path is None:
                expanded_arguments.append(argument)
                continue
            copied_path = temporary_dir / f"fixture-{fixture_index}{fixture_path.suffix}"
            shutil.copyfile(fixture_path, copied_path)
            copied_path.chmod(0o400)
            expanded_arguments.append(str(copied_path))
            fixture_index += 1
        return expanded_arguments

    @staticmethod
    def create_placeholder_file(temporary_dir: Path, placeholder: DemoPlaceholder) -> Path:
        """Create the temporary file for a file placeholder."""
        file_path = temporary_dir / f"file-{placeholder.index}-{placeholder.value}"
        if placeholder.value == "empty":
            file_path.touch()
        elif placeholder.value == "text":
            file_path.write_text(TEXT_PLACEHOLDER_CONTENT, encoding="utf-8")
        elif placeholder.value == "none":
            pass
        else:
            raise DemoDocError(f"Demo file placeholder type is not allowed: {placeholder.value!r}")
        return file_path

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

    def run(
        self,
        command_text: str,
        *,
        expected_exit_code: int = 0,
        demo_files: dict[str, DemoFile] | None = None,
    ) -> str:
        """Run a validated demo command and return normalized ANSI text."""
        if os.name != "posix":
            raise DemoDocError("ANSI demo capture requires a POSIX pseudo-terminal.")
        demo_files = demo_files or {}
        parts = self.validate_command(command_text, demo_files)
        executable_path = self.executable_path(parts[0])
        executable_argument = str(executable_path.relative_to(self.project_dir))
        arguments = [str(demo_files[argument].path) if argument in demo_files else argument for argument in parts[1:]]
        command = [executable_argument, *arguments]
        temporary_dir_context = None
        if any(
            self.parse_placeholder(argument, 0) is not None or self.fixture_path(argument) is not None
            for argument in parts[1:]
        ):
            temporary_dir_context = tempfile.TemporaryDirectory(prefix="erbsland-demo-doc-")
            temporary_dir = Path(temporary_dir_context.name)
            arguments = self.expand_placeholders(arguments, temporary_dir)
            command = [executable_argument, *self.copy_fixture_arguments(arguments, temporary_dir)]
        process = None
        terminal_master_fd = None
        terminal_slave_fd = None
        try:
            terminal_master_fd, terminal_slave_fd = pty.openpty()
            capture_width = max(
                [DEMO_TERMINAL_WIDTH, *(len(os.fsencode(demo_file.path)) + 32 for demo_file in demo_files.values())]
            )
            fcntl.ioctl(
                terminal_slave_fd,
                termios.TIOCSWINSZ,
                struct.pack("HHHH", DEMO_TERMINAL_HEIGHT, capture_width, 0, 0),
            )
            process = subprocess.Popen(
                command,
                cwd=self.project_dir,
                env=safe_subprocess_environment(),
                stdin=subprocess.DEVNULL,
                stdout=terminal_slave_fd,
                stderr=terminal_slave_fd,
            )
            os.close(terminal_slave_fd)
            terminal_slave_fd = None
            output = bytearray()
            output_limit_error: list[str] = []

            def read_output() -> None:
                while True:
                    try:
                        chunk = os.read(terminal_master_fd, 4096)
                    except OSError as error:
                        if error.errno == errno.EIO:
                            return
                        raise
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
                raise DemoDocError(f"Demo execution timed out after {DEMO_TIMEOUT_SECONDS} seconds.") from None
            output_thread.join(timeout=1)
            if output_limit_error:
                raise DemoDocError(output_limit_error[0])
            raw_output = self.redact_demo_file_paths(bytes(output), demo_files)
            text = raw_output.decode("utf-8", errors="replace")
            if len(text.splitlines()) > MAX_DEMO_OUTPUT_LINES:
                raise DemoDocError(f"Demo output exceeds {MAX_DEMO_OUTPUT_LINES} lines.")
            if process.returncode != expected_exit_code:
                excerpt = text.strip().splitlines()
                detail = f" Last output line: {excerpt[-1]}" if excerpt else ""
                if expected_exit_code == 0:
                    raise DemoDocError(f"Demo command failed with exit code {process.returncode}.{detail}")
                raise DemoDocError(
                    f"Demo command exited with {process.returncode}, expected {expected_exit_code}.{detail}"
                )
            return self.convert_output(raw_output)
        finally:
            if terminal_slave_fd is not None:
                os.close(terminal_slave_fd)
            if terminal_master_fd is not None:
                os.close(terminal_master_fd)
            if temporary_dir_context is not None:
                temporary_dir_context.cleanup()

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
        terminal = Terminal(width=DEMO_TERMINAL_WIDTH, height=DEMO_TERMINAL_HEIGHT, back_buffer_height=2000)
        terminal.write(text)
        return f"{terminal.to_ansi()}\n" if raw_output else ""

    @staticmethod
    def redact_demo_file_paths(raw_output: bytes, demo_files: dict[str, DemoFile]) -> bytes:
        """Replace resolved demo file paths in captured output with their declared basenames."""
        for demo_file in demo_files.values():
            raw_output = raw_output.replace(os.fsencode(demo_file.path), os.fsencode(demo_file.name))
        return raw_output


class DemoDocSynchronizer:
    """Synchronize managed demo blocks in one reStructuredText document."""

    RE_START = re.compile(r"^(?P<indent>\s*)\.\.\s+erbsland-demo::\s*$")
    RE_END = re.compile(r"^\s*\.\.\s+erbsland-demo-end::\s*$")
    RE_OPTION = re.compile(
        r"^\s+:(?P<name>files(?:-sha256)?|function-blocks(?:-sha256)?|source|source-sha256|show-cmd-line|exec(?:-\d+)?(?:-exit-code)?):\s*(?P<value>.*)$"
    )
    RE_EXEC_OPTION = re.compile(r"^exec(?:-(?P<index>\d+))?$")
    RE_FUNCTION_NAME = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")

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
            if not self.block_needs_update(path, block):
                result.extend(lines[block.start : block.end + 1])
            else:
                generated = self.generate_block(path, block)
                result.extend(generated.text.splitlines())
                issues.extend(f"{path}:{block.start + 1}: {issue}" for issue in generated.issues)
            cursor = block.end + 1
        result.extend(lines[cursor:])
        updated_text = "\n".join(result)
        if had_final_newline or text == "":
            updated_text += "\n"
        return updated_text, tuple(issues)

    def block_needs_update(self, path: Path, block: DemoBlock) -> bool:
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
        if block.options.get("source-sha256") != source_hash:
            return True
        function_blocks = block.options.get("function-blocks", "")
        if function_blocks:
            if block.options.get("function-blocks-sha256") != sha256_bytes(function_blocks.encode()):
                return True
        files_option = block.options.get("files", "")
        if files_option:
            try:
                files_hash = self.demo_files_hash(self.resolve_demo_files(path, files_option))
            except DemoDocError:
                return True
            if block.options.get("files-sha256") != files_hash:
                return True
        return False

    def generate_block(self, path: Path, block: DemoBlock) -> GeneratedBlock:
        """Generate one managed demo block."""
        issues: list[str] = []
        source_option = block.options.get("source", "")
        source_hash = ""
        display_source_lines: list[str] = []
        if not source_option:
            issues.append("Missing required :source: option.")
        else:
            try:
                source_hash, display_source_lines = self.load_display_source(
                    source_option, block.options.get("function-blocks", "")
                )
            except DemoDocError as error:
                issues.append(str(error))

        lines = [
            f"{block.indent}.. erbsland-demo::",
            f"{block.indent}    :source: {source_option}",
        ]
        function_blocks = block.options.get("function-blocks", "")
        if function_blocks:
            lines.append(f"{block.indent}    :function-blocks: {function_blocks}")
            lines.append(f"{block.indent}    :function-blocks-sha256: {sha256_bytes(function_blocks.encode())}")
        demo_files: dict[str, DemoFile] = {}
        files_option = block.options.get("files", "")
        if files_option:
            lines.append(f"{block.indent}    :files: {files_option}")
            try:
                demo_files = self.resolve_demo_files(path, files_option)
                lines.append(f"{block.indent}    :files-sha256: {self.demo_files_hash(demo_files)}")
            except DemoDocError as error:
                issues.append(str(error))
        runs = self.parse_runs(block.options, issues)
        show_cmd_line = "show-cmd-line" in block.options
        for run in runs:
            lines.append(f"{block.indent}    :{run.exec_option_name}: {run.command_text}")
            if run.expected_exit_code != 0:
                lines.append(f"{block.indent}    :{run.exit_code_option_name}: {run.expected_exit_code}")
        if show_cmd_line:
            lines.append(f"{block.indent}    :show-cmd-line:")
        if source_hash:
            lines.append(f"{block.indent}    :source-sha256: {source_hash}")
        lines.append("")

        if display_source_lines:
            self.append_code_block(lines, block.indent, display_source_lines)
        for run in runs:
            try:
                output = self.executor.run(
                    run.command_text,
                    expected_exit_code=run.expected_exit_code,
                    demo_files=demo_files,
                )
                if output:
                    if show_cmd_line or len(runs) > 1:
                        self.append_command_rubric(lines, block.indent, run.command_text)
                    self.append_ansi_block(lines, block.indent, output)
            except DemoDocError as error:
                issues.append(str(error))
        if issues:
            self.append_note(lines, block.indent, issues)
        lines.append(f"{block.indent}.. erbsland-demo-end::")
        return GeneratedBlock("\n".join(lines), tuple(issues))

    def resolve_demo_files(self, document_path: Path, files_option: str) -> dict[str, DemoFile]:
        """Resolve validated document-relative demo files and index them by basename."""
        try:
            configured_paths = shlex.split(files_option, posix=True)
        except ValueError as error:
            raise DemoDocError(f"Could not parse :files: option: {error}") from None
        if not configured_paths:
            raise DemoDocError("Invalid :files: value: expected at least one relative file path.")

        result: dict[str, DemoFile] = {}
        project_dir = self.project_dir.resolve(strict=False)
        for configured_path in configured_paths:
            relative_path = Path(configured_path)
            if relative_path == Path(".") or relative_path.is_absolute():
                raise DemoDocError(f"Demo file must be a relative path: {configured_path}")
            resolved_path = (document_path.parent / relative_path).resolve(strict=False)
            if resolved_path == project_dir or not resolved_path.is_relative_to(project_dir):
                raise DemoDocError(f"Demo file escapes the project directory: {configured_path}")
            try:
                require_safe_existing_file(resolved_path, "Demo file", MAX_DEMO_SOURCE_SIZE)
            except UtilityError as error:
                raise DemoDocError(str(error)) from None
            if not resolved_path.exists():
                raise DemoDocError(f"Demo file does not exist: {configured_path}")
            name = Path(configured_path).name
            if name in result:
                raise DemoDocError(f"Demo files must have unique basenames: {name}")
            result[name] = DemoFile(name=name, configured_path=configured_path, path=resolved_path)
        return result

    @staticmethod
    def demo_files_hash(demo_files: dict[str, DemoFile]) -> str:
        """Create a stable combined hash for configured demo file paths and contents."""
        digest = hashlib.sha256()
        for demo_file in demo_files.values():
            digest.update(demo_file.configured_path.encode("utf-8"))
            digest.update(b"\0")
            digest.update(demo_file.path.read_bytes())
            digest.update(b"\0")
        return digest.hexdigest()

    def parse_runs(self, options: dict[str, str], issues: list[str]) -> list[DemoRun]:
        """Parse all configured demo command executions."""
        commands: dict[int, str] = {}
        expected_exit_codes: dict[int, int] = {}
        for name, value in options.items():
            match = self.RE_EXEC_OPTION.fullmatch(name)
            if match is not None:
                index = int(match.group("index") or "1")
                commands[index] = value
                continue
            if name == "exec-exit-code":
                self.parse_expected_exit_code(name, value, 1, expected_exit_codes, issues)
                continue
            if name.startswith("exec-") and name.endswith("-exit-code"):
                index_text = name[len("exec-") : -len("-exit-code")]
                if index_text.isdigit():
                    self.parse_expected_exit_code(name, value, int(index_text), expected_exit_codes, issues)
        return [
            DemoRun(index=index, command_text=command_text, expected_exit_code=expected_exit_codes.get(index, 0))
            for index, command_text in sorted(commands.items())
        ]

    @staticmethod
    def parse_expected_exit_code(
        option_name: str,
        value: str,
        index: int,
        expected_exit_codes: dict[int, int],
        issues: list[str],
    ) -> None:
        """Parse one expected exit code option."""
        try:
            exit_code = int(value)
        except ValueError:
            issues.append(f"Invalid :{option_name}: value: expected an integer.")
            return
        if exit_code < 0:
            issues.append(f"Invalid :{option_name}: value: expected a non-negative integer.")
            return
        expected_exit_codes[index] = exit_code

    def load_display_source(self, source_option: str, function_blocks: str = "") -> tuple[str, list[str]]:
        """Read and trim the display source for one block."""
        data = self.source_bytes(source_option)
        source_hash = sha256_bytes(data)
        try:
            text = data.decode("utf-8")
        except UnicodeDecodeError:
            raise DemoDocError(f"Demo source is not valid UTF-8: {source_option}") from None
        source_lines = text.splitlines()
        if function_blocks:
            return source_hash, self.load_function_blocks(source_lines, function_blocks)
        start_index = next((index for index, line in enumerate(source_lines) if line.lstrip().startswith("///")), None)
        if start_index is None:
            raise DemoDocError(f"Demo source has no /// documentation comment: {source_option}")
        display_lines = source_lines[start_index:]
        while display_lines and not display_lines[-1].strip():
            display_lines.pop()
        display_lines = self.remove_display_namespace_tail(source_lines, start_index, display_lines)
        return source_hash, display_lines

    def load_function_blocks(self, source_lines: list[str], function_blocks: str) -> list[str]:
        """Load and dedent named ``auto`` or ``void`` function bodies from a demo source."""
        function_names = function_blocks.split()
        if not function_names:
            raise DemoDocError("Invalid :function-blocks: value: expected at least one function name.")
        invalid_name = next((name for name in function_names if self.RE_FUNCTION_NAME.fullmatch(name) is None), None)
        if invalid_name is not None:
            raise DemoDocError(f"Invalid :function-blocks: function name: {invalid_name!r}.")

        selected_blocks = [self.find_function_block(source_lines, name) for name in function_names]
        return textwrap.dedent("\n\n".join("\n".join(block) for block in selected_blocks)).splitlines()

    @classmethod
    def find_function_block(cls, source_lines: list[str], function_name: str) -> list[str]:
        """Find one complete named ``auto`` or ``void`` function definition."""
        function_start = re.compile(
            rf"^(?P<indent>[ \t]*)(?:auto|void)\s+{re.escape(function_name)}\s*\(", re.MULTILINE
        )
        source_text = "\n".join(source_lines)
        matches = tuple(function_start.finditer(source_text))
        if not matches:
            raise DemoDocError(f"Function block not found: {function_name}.")
        if len(matches) > 1:
            raise DemoDocError(f"Function block is ambiguous: {function_name}.")

        start_index = source_text[: matches[0].start()].count("\n")
        depth = 0
        has_opening_brace = False
        for end_index in range(start_index, len(source_lines)):
            line = source_lines[end_index]
            if "{" in line:
                has_opening_brace = True
            depth += cls.brace_delta(line)
            if has_opening_brace and depth == 0:
                return source_lines[start_index : end_index + 1]
        raise DemoDocError(f"Function block has no closing brace: {function_name}.")

    @staticmethod
    def remove_display_namespace_tail(source_lines: list[str], start_index: int, display_lines: list[str]) -> list[str]:
        """Remove the demo namespace close and any following wrapper code."""
        namespace_index = next(
            (
                index
                for index, line in reversed(tuple(enumerate(source_lines[:start_index])))
                if line.strip() == "namespace demo {"
            ),
            None,
        )
        if namespace_index is None or not display_lines:
            return display_lines
        namespace_end = DemoDocSynchronizer.find_namespace_end(source_lines, namespace_index)
        if namespace_end is None or namespace_end <= start_index:
            return display_lines
        result = source_lines[start_index:namespace_end]
        while result and not result[-1].strip():
            result.pop()
        return result

    @staticmethod
    def find_namespace_end(source_lines: list[str], namespace_index: int) -> int | None:
        """Find the matching closing brace for a namespace line."""
        depth = 0
        for index, line in enumerate(source_lines[namespace_index:], start=namespace_index):
            depth += DemoDocSynchronizer.brace_delta(line)
            if index > namespace_index and depth == 0:
                return index
        return None

    @staticmethod
    def brace_delta(line: str) -> int:
        """Count curly braces outside simple strings and line comments."""
        result = 0
        quote = ""
        escaped = False
        index = 0
        while index < len(line):
            character = line[index]
            if quote:
                if escaped:
                    escaped = False
                elif character == "\\":
                    escaped = True
                elif character == quote:
                    quote = ""
            else:
                if character == "/" and index + 1 < len(line) and line[index + 1] == "/":
                    break
                if character in {"'", '"'}:
                    quote = character
                elif character == "{":
                    result += 1
                elif character == "}":
                    result -= 1
            index += 1
        return result

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
    def append_command_rubric(lines: list[str], indent: str, command_text: str) -> None:
        """Append a command line label before generated output."""
        lines.extend([f"{indent}.. rubric:: ``$ {command_text}``", ""])

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
    """Validate, synchronize, or refresh managed documentation demo blocks."""

    description = "Validate, synchronize, or refresh erbsland-demo documentation blocks."

    def __init__(self) -> None:
        super().__init__()
        self.project_dir = Path()
        self.mode = "validate"
        self.force = False
        self.recursive = False
        self.target = Path()

    def add_command_line_args(self, parser: argparse.ArgumentParser) -> None:
        parser.add_argument(
            "mode",
            choices=("validate", "sync", "refresh"),
            help="Validate, synchronize, or recursively refresh demo blocks.",
        )
        parser.add_argument("-r", "--recursive", action="store_true", help="Process directories recursively.")
        parser.add_argument("--force", action="store_true", help="Regenerate blocks even when the source hash matches.")
        parser.add_argument("target", type=Path, help="An .rst file or directory to process.")

    def handle_command_line_args(self, args: argparse.Namespace) -> None:
        self.project_dir = self.project_directory
        self.mode = "sync" if args.mode == "refresh" else args.mode
        self.force = args.force or args.mode == "refresh"
        self.recursive = args.recursive or args.mode == "refresh"
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
