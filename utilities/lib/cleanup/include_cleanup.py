# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import re
from collections.abc import Callable
from pathlib import Path
from re import Match

from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.include_block import IncludeBlock, IncludeBlockScanner
from lib.path_safety import read_safe_text, require_safe_existing_file

from .config import CleanupConfig


class Include:
    """A single include statement."""

    INCLUDE_CATEGORY_PATTERNS = [
        (re.compile(r'^"[^.][^/]+"'), 1),
        (re.compile(r'^"[^./]+/.+"'), 2),
        (re.compile(r'^"\.\./[^.].+"'), 3),
        (re.compile(r'^"\.\./\.\./.+"'), 4),
        (re.compile(r"^<.*\.h(pp)?>"), 5),
        (re.compile(r"^<[^.]+>"), 6),
        (re.compile(r".*"), 7),
    ]

    def __init__(self, path: str, is_global: bool):
        self.path = path
        self.is_global = is_global
        self.check_for_problems()
        self.group_id = self.get_group_id()
        self.sort_path = self.get_sort_path()

    def __eq__(self, other):
        if not isinstance(other, Include):
            return False
        return self.sort_path == other.sort_path and self.is_global == other.is_global

    def check_for_problems(self):
        if self.path.startswith("src/") or self.path.startswith("erbsland/"):
            raise UtilityError(f"Include statement with invalid path: {self.path}")

    def get_group_id(self) -> int:
        include_name = self.get_include_name()
        for pattern, priority in self.INCLUDE_CATEGORY_PATTERNS:
            if pattern.match(include_name):
                return priority
        return 7

    def get_include_name(self) -> str:
        if self.is_global:
            return f"<{self.path}>"
        return f'"{self.path}"'

    def get_sort_path(self) -> str:
        return self.get_include_name().casefold()


class IncludeCleanup:
    """Cleanup pass for C++ include statements."""

    RE_INCLUDE_LINE = re.compile(
        r"""
        \#include\s+(<|")([^>"]+)(>|")
    """,
        re.VERBOSE,
    )

    def __init__(self, config: CleanupConfig, print_verbose: Callable[[str], None]) -> None:
        self.config = config
        self.print_verbose = print_verbose
        self.file_update = FileUpdate(print_verbose)

    @classmethod
    def normalize_leading_spacing(cls, text: str, is_header: bool) -> str:
        """Normalize the spacing between the copyright block and the first directive."""
        return IncludeBlockScanner.normalize_leading_spacing(text, is_header)

    def create_include_from_match(self, path: Path, line_match: Match[str]) -> Include:
        """Create an include model from one parsed include line."""
        include_path = line_match.group(2)
        if include_path.startswith("src/erbsland/"):
            include_path = self.fix_clion_refactoring_include_path(path, include_path)
        return Include(include_path, line_match.group(1) == "<")

    def fix_clion_refactoring_include_path(self, path: Path, include_path: str) -> str:
        """Convert CLion's occasional source-root include into a local relative include."""
        target_path = self.config.library_source_dir / include_path.replace("src/erbsland/", "")
        rel_to_path_parts = path.relative_to(self.config.library_source_dir).parts
        rel_to_target_parts = target_path.relative_to(self.config.library_source_dir).parts
        if not rel_to_path_parts or not rel_to_target_parts:
            raise UtilityError("Unexpected empty paths")
        while rel_to_path_parts and rel_to_target_parts and rel_to_path_parts[0] == rel_to_target_parts[0]:
            rel_to_path_parts = rel_to_path_parts[1:]
            rel_to_target_parts = rel_to_target_parts[1:]
        up_count = max(0, len(rel_to_path_parts) - 1)
        return "../" * up_count + "/".join(rel_to_target_parts)

    def find_include_block(self, text: str, is_header: bool) -> IncludeBlock | None:
        """Find the include block that is managed by this utility."""
        try:
            return IncludeBlockScanner.find_include_block(text, is_header)
        except ValueError as error:
            raise UtilityError(str(error)) from None

    def collect_includes(self, path: Path, include_block: str) -> list[Include]:
        """Parse, deduplicate and sort all include lines from one include block."""
        include_list: list[Include] = []
        for line in include_block.splitlines():
            line_match = self.RE_INCLUDE_LINE.match(line)
            if not line_match:
                continue
            include = self.create_include_from_match(path, line_match)
            if include in include_list:
                self.print_verbose(f"  ignored duplicate include statement: {line}")
                continue
            include_list.append(include)
        include_list.sort(key=lambda include: (include.group_id, include.sort_path))
        return include_list

    @staticmethod
    def render_include_block(include_list: list[Include]) -> str:
        """Render the normalized include block."""
        if not include_list:
            return ""
        last_group_id = 0
        lines = [""]
        for entry in include_list:
            if last_group_id > 0 and last_group_id != entry.group_id:
                lines.append("")
            start_quote, end_quote = ("<", ">") if entry.is_global else ('"', '"')
            lines.append(f"#include {start_quote}{entry.path}{end_quote}")
            last_group_id = entry.group_id
        lines.append("")
        return "\n".join(lines) + "\n"

    def process_file(self, path: Path, is_header: bool) -> None:
        """Cleanup a single C++ file."""
        self.print_verbose(f"Processing file: {path.relative_to(self.config.library_source_dir)}")
        original_text = read_safe_text(path, "C++ source file", FileUpdate.MAX_COMPARE_FILE_SIZE)
        text = self.normalize_leading_spacing(original_text, is_header)
        include_block = self.find_include_block(text, is_header)
        if include_block is None:
            if original_text != text:
                self.file_update.write_if_changed(path, text)
                return
            self.print_verbose("  no include block found.")
            return
        include_list = self.collect_includes(path, include_block.text)
        if not include_list:
            self.print_verbose("Ignoring file without relevant includes.")
            return
        new_text = text[0 : include_block.start] + self.render_include_block(include_list) + text[include_block.end :]
        self.file_update.write_if_changed(path, new_text)

    def run(self) -> None:
        """Run the include cleanup pass."""
        paths = [
            path
            for path in self.config.library_source_dir.rglob("*")
            if not path.is_symlink() and path.suffix in self.config.scanned_file_suffixes
        ]
        paths.sort(key=lambda p: str(p).casefold())
        for path in paths:
            if path.name in self.config.excluded_names:
                continue
            self.process_file(path, path.suffix in self.config.header_file_suffixes)
