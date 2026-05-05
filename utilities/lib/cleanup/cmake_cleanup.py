# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import re
from collections.abc import Callable
from datetime import date
from pathlib import Path

from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.path_safety import read_safe_text

from .config import CleanupConfig


class TargetSourcesBlock:
    """A parsed `target_sources(...)` block."""

    VISIBILITY_NAMES = {"PRIVATE", "PUBLIC", "INTERFACE"}

    def __init__(self, target: str, files: list[str], indent: str = "") -> None:
        self.target = target
        self.files = sorted(files, key=str.casefold)
        self.indent = indent

    @classmethod
    def from_match(cls, match: re.Match[str]) -> "TargetSourcesBlock":
        """Create a target source block from a regex match."""
        content = match.group("content")
        tokens = cls.file_tokens_from_content(content)
        return cls(match.group("target"), tokens, match.group("indent"))

    @classmethod
    def file_tokens_from_content(cls, content: str) -> list[str]:
        """Extract file tokens from a `target_sources(...)` body."""
        tokens: list[str] = []
        for line in content.splitlines():
            line = line.split("#", 1)[0].strip()
            if not line:
                continue
            for token in line.split():
                if token.upper() in cls.VISIBILITY_NAMES:
                    continue
                tokens.append(token)
        return tokens

    def add_files(self, files: list[str]) -> None:
        """Add files to this block and keep the list sorted and unique."""
        self.files = sorted(set(self.files) | set(files), key=str.casefold)

    def render(self, indent: str | None = None) -> str:
        """Render this block using the configured CMake style."""
        if indent is None:
            indent = self.indent
        lines = [f"{indent}target_sources({self.target} PRIVATE"]
        for file in self.files:
            lines.append(f"{indent}        {file}")
        lines.append(f"{indent})")
        return "\n".join(lines)


class CMakeListsFile:
    """A CMakeLists.txt file split into manageable sections."""

    def __init__(self, path: Path, text: str) -> None:
        self.path = path
        self.text = text
        self.prefix, self.conditional_suffix = self.split_at_first_if(text)

    @staticmethod
    def split_at_first_if(text: str) -> tuple[str, str]:
        """Split the file at the first line that starts an `if(...)` block."""
        match = re.search(r"(?m)^if\s*\(", text)
        if not match:
            return text, ""
        return text[: match.start()], text[match.start() :]


class CMakeCleanup:
    """Cleanup pass for simple CMakeLists.txt files."""

    ALLOWED_COMMANDS = {"add_subdirectory", "target_sources", "if", "else", "endif", "cmake_minimum_required"}
    RE_COMMAND = re.compile(r"(?P<cmd_name>\w+)[ ]*\(")
    RE_TOP_LEVEL_ADD_SUBDIRECTORY = re.compile(r"(?m)^add_subdirectory\(\s*(?P<directory>[-_a-zA-Z0-9.]+)\s*\)[ \t]*$")
    RE_TARGET_SOURCES = re.compile(
        r"(?ms)^(?P<indent>[ \t]*)target_sources\(\s*(?P<target>[-_a-zA-Z0-9]+)\s+(?P<content>.*?)\)"
    )

    def __init__(self, config: CleanupConfig, print_verbose: Callable[[str], None]) -> None:
        self.config = config
        self.print_verbose = print_verbose
        self.file_update = FileUpdate(print_verbose)

    @staticmethod
    def strip_comments(text: str) -> str:
        """Remove comments before scanning CMake commands."""
        return "\n".join(line.split("#", 1)[0] for line in text.splitlines())

    def command_names(self, text: str) -> set[str]:
        """Collect all CMake command names used by a file."""
        return {match.group("cmd_name").lower() for match in self.RE_COMMAND.finditer(self.strip_comments(text))}

    def is_simple_file(self, path: Path, text: str) -> bool:
        """Test if a CMakeLists.txt file is simple enough to cleanup."""
        commands = self.command_names(text)
        unsupported = commands - self.ALLOWED_COMMANDS
        if unsupported:
            self.print_verbose(f"Skipping complex CMake file {path}: {', '.join(sorted(unsupported))}")
            return False
        return True

    def find_cmake_files(self) -> list[Path]:
        """Find all CMakeLists.txt files below configured source roots."""
        paths = []
        for directory in self.config.cmake_source_dirs:
            paths.extend(directory.rglob("CMakeLists.txt"))
        return sorted(set(paths), key=lambda path: path.as_posix().casefold())

    def validate_local_path(self, cmake_dir: Path, entry: str, *, is_directory: bool) -> None:
        """Validate that a CMake entry is local and exists."""
        entry_path = Path(entry)
        if entry_path.is_absolute() or entry_path.name != entry or entry in {".", ".."}:
            raise UtilityError(f"CMake entry in {cmake_dir} is not directory-local: {entry}")
        full_path = cmake_dir / entry
        if is_directory:
            if not full_path.is_dir():
                raise UtilityError(f"CMake add_subdirectory entry does not exist: {full_path}")
        elif not full_path.is_file():
            raise UtilityError(f"CMake target_sources entry does not exist: {full_path}")

    def collect_add_subdirectories(self, cmake_dir: Path, prefix: str) -> list[str]:
        """Collect column-0 `add_subdirectory(...)` entries from a CMake prefix."""
        directories = []
        for match in self.RE_TOP_LEVEL_ADD_SUBDIRECTORY.finditer(prefix):
            directory = match.group("directory")
            self.validate_local_path(cmake_dir, directory, is_directory=True)
            directories.append(directory)
        return sorted(set(directories), key=str.casefold)

    def collect_target_source_blocks(self, cmake_dir: Path, text: str) -> list[TargetSourcesBlock]:
        """Collect and validate target source blocks from text."""
        blocks = []
        for match in self.RE_TARGET_SOURCES.finditer(text):
            block = TargetSourcesBlock.from_match(match)
            self.validate_target_sources_block(cmake_dir, block)
            blocks.append(block)
        return blocks

    def validate_target_sources_block(self, cmake_dir: Path, block: TargetSourcesBlock) -> None:
        """Validate all file entries in a target source block."""
        for file in block.files:
            self.validate_local_path(cmake_dir, file, is_directory=False)

    def merge_blocks(self, blocks: list[TargetSourcesBlock]) -> TargetSourcesBlock | None:
        """Merge all pre-conditional target source blocks into one main block."""
        if not blocks:
            return None
        target_names = {block.target for block in blocks}
        if len(target_names) > 1:
            raise UtilityError(f"Cannot merge target_sources for multiple targets: {', '.join(sorted(target_names))}")
        result = TargetSourcesBlock(blocks[0].target, [])
        for block in blocks:
            result.add_files(block.files)
        return result

    def files_in_blocks(self, blocks: list[TargetSourcesBlock]) -> set[str]:
        """Collect all files referenced by a list of target source blocks."""
        return {file for block in blocks for file in block.files}

    def normalize_suffix_target_sources(self, cmake_dir: Path, suffix: str) -> tuple[str, list[TargetSourcesBlock]]:
        """Normalize target source blocks in the conditional suffix while preserving surrounding text."""
        blocks: list[TargetSourcesBlock] = []

        def replace(match: re.Match[str]) -> str:
            block = TargetSourcesBlock.from_match(match)
            self.validate_target_sources_block(cmake_dir, block)
            blocks.append(block)
            return block.render()

        return self.RE_TARGET_SOURCES.sub(replace, suffix), blocks

    def main_target_block(
        self, cmake_dir: Path, prefix_blocks: list[TargetSourcesBlock], suffix_blocks: list[TargetSourcesBlock]
    ) -> TargetSourcesBlock | None:
        """Create the main target_sources block for the normalized prefix."""
        local_files = self.config.local_source_files(cmake_dir)
        if not local_files and not prefix_blocks:
            return None
        block = self.merge_blocks(prefix_blocks)
        if block is None:
            block = TargetSourcesBlock(self.config.target_for_directory(cmake_dir), [])
        covered_files = self.files_in_blocks(prefix_blocks) | self.files_in_blocks(suffix_blocks)
        missing_files = [file for file in local_files if file not in covered_files]
        block.add_files(missing_files)
        return block

    def render_prefix(self, add_subdirectories: list[str], target_sources: TargetSourcesBlock | None) -> str:
        """Render the normalized prefix before any conditional block."""
        blocks = [
            self.config.cmake_header_text(date.today().year),
            f"cmake_minimum_required(VERSION {self.config.cmake_minimum_version})",
        ]
        if add_subdirectories:
            blocks.append("\n".join(f"add_subdirectory({directory})" for directory in add_subdirectories))
        if target_sources is not None:
            blocks.append(target_sources.render(""))
        return "\n\n".join(blocks) + "\n"

    @staticmethod
    def normalize_conditional_suffix(suffix: str) -> str:
        """Trim a preserved conditional suffix to the project block-spacing style."""
        return suffix.strip() + "\n" if suffix.strip() else ""

    def cleanup_file(self, path: Path) -> None:
        """Cleanup one CMakeLists.txt file if it is simple enough."""
        original_text = read_safe_text(path, "CMake file", FileUpdate.MAX_COMPARE_FILE_SIZE)
        if not self.is_simple_file(path, original_text):
            return
        self.print_verbose(f"Processing CMake file: {path.relative_to(self.config.project_dir)}")
        cmake_file = CMakeListsFile(path, original_text)
        cmake_dir = path.parent
        add_subdirectories = self.collect_add_subdirectories(cmake_dir, cmake_file.prefix)
        prefix_blocks = self.collect_target_source_blocks(cmake_dir, cmake_file.prefix)
        suffix, suffix_blocks = self.normalize_suffix_target_sources(cmake_dir, cmake_file.conditional_suffix)
        target_sources = self.main_target_block(cmake_dir, prefix_blocks, suffix_blocks)
        normalized_suffix = self.normalize_conditional_suffix(suffix)
        new_text = self.render_prefix(add_subdirectories, target_sources)
        if normalized_suffix:
            new_text = f"{new_text}\n{normalized_suffix.strip()}\n"
        self.file_update.write_if_changed(path, new_text)

    def run(self) -> None:
        """Run the CMake cleanup pass."""
        for path in self.find_cmake_files():
            self.cleanup_file(path)
