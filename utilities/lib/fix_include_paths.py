# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import os
from collections import defaultdict
from collections.abc import Callable, Iterable
from dataclasses import dataclass
from pathlib import Path, PurePosixPath

from lib.config import read_elcl_file, validate_file_suffixes, validate_local_names
from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.include_block import IncludeBlockScanner, IncludeLine
from lib.path_safety import read_safe_text, require_directory, require_safe_existing_file, resolve_project_path


@dataclass(frozen=True)
class FixIncludePathsConfig:
    """Configuration for the include path fixer."""

    project_dir: Path
    source_dir: Path
    unittest_dir: Path
    excluded_names: frozenset[str]
    scanned_file_suffixes: frozenset[str]
    header_file_suffixes: frozenset[str]

    @classmethod
    def read(cls, project_dir: Path, config_file: Path) -> "FixIncludePathsConfig":
        """Read the fix-include-paths configuration from an ELCL file."""
        config = read_elcl_file(config_file)
        main_config = config["main"]
        project_dir = project_dir.resolve()
        result = cls(
            project_dir=project_dir,
            source_dir=resolve_project_path(project_dir, main_config.get_text("sources"), "Sources"),
            unittest_dir=resolve_project_path(project_dir, main_config.get_text("unittests"), "Unittests"),
            excluded_names=frozenset(main_config.get_list("excluded_names", str, default=[])),
            scanned_file_suffixes=frozenset(main_config.get_list("scanned_file_suffixes", str, default=[])),
            header_file_suffixes=frozenset(main_config.get_list("header_file_suffixes", str, default=[])),
        )
        result.validate()
        return result

    def validate(self) -> None:
        """Validate the configuration."""
        require_directory(self.source_dir, "Sources")
        require_directory(self.unittest_dir, "Unittests")
        validate_local_names(self.excluded_names, "Excluded Names")
        if not self.scanned_file_suffixes:
            raise UtilityError("No scanned file suffixes configured.")
        validate_file_suffixes(self.scanned_file_suffixes, "Scanned File Suffixes")
        if not self.header_file_suffixes:
            raise UtilityError("No header file suffixes configured.")
        validate_file_suffixes(self.header_file_suffixes, "Header File Suffixes")

    def is_header_file(self, path: Path) -> bool:
        """Test if the path is treated as a header file."""
        return path.suffix in self.header_file_suffixes

    def source_files(self, root: Path) -> list[Path]:
        """Find source files below the given root directory."""
        paths = [
            path
            for path in root.rglob("*")
            if path.is_file()
            and not path.is_symlink()
            and path.suffix in self.scanned_file_suffixes
            and path.name not in self.excluded_names
        ]
        return sorted(paths, key=lambda path: str(path).casefold())


@dataclass(frozen=True)
class PendingIncludePathUpdate:
    """A file update that is only written after all source files were validated."""

    path: Path
    text: str


class LibrarySourceMap:
    """Lookup table for library source files."""

    def __init__(self, source_dir: Path) -> None:
        self.source_dir = source_dir
        self._paths: set[PurePosixPath] = set()
        self._paths_by_name: dict[str, list[PurePosixPath]] = defaultdict(list)

    @classmethod
    def collect(cls, config: FixIncludePathsConfig) -> "LibrarySourceMap":
        """Collect all configured library source files."""
        result = cls(config.source_dir)
        for path in config.source_dir.rglob("*"):
            if not path.is_file() or path.is_symlink() or path.suffix not in config.scanned_file_suffixes:
                continue
            require_safe_existing_file(path, "Library source file", FileUpdate.MAX_COMPARE_FILE_SIZE)
            relative_path = PurePosixPath(path.relative_to(config.source_dir).as_posix())
            result._paths.add(relative_path)
            result._paths_by_name[relative_path.name].append(relative_path)
        for paths in result._paths_by_name.values():
            paths.sort(key=lambda value: value.as_posix().casefold())
        return result

    def has_filename(self, filename: str) -> bool:
        """Test if a source file with the given base name exists."""
        return filename in self._paths_by_name

    def has_relative_path(self, include_path: str) -> bool:
        """Test if the include path exists below the source root."""
        path = self.clean_posix_path(include_path)
        return path is not None and path in self._paths

    def path_for_include(self, include_path: str, context: str) -> PurePosixPath:
        """Resolve an include path to exactly one library source file."""
        path = self.clean_posix_path(include_path)
        if path is not None:
            if path in self._paths:
                return path
            suffix_matches = [candidate for candidate in self._paths if self.ends_with(candidate, path)]
            if len(suffix_matches) == 1:
                return suffix_matches[0]
            if len(suffix_matches) > 1:
                raise self.ambiguous_include_error(include_path, context, suffix_matches)

        filename = PurePosixPath(include_path).name
        candidates = self._paths_by_name.get(filename, [])
        if not candidates:
            raise UtilityError(
                f"Cannot resolve include path in {context}: {include_path}\n"
                f'No library source file named "{filename}" exists below {self.source_dir}.'
            )
        if len(candidates) > 1:
            raise self.ambiguous_include_error(include_path, context, candidates)
        return candidates[0]

    @staticmethod
    def clean_posix_path(include_path: str) -> PurePosixPath | None:
        """Convert a clean include path into a POSIX path."""
        path = PurePosixPath(include_path)
        if not include_path or path.is_absolute() or ".." in path.parts or "." in path.parts:
            return None
        return path

    @staticmethod
    def ends_with(candidate: PurePosixPath, suffix: PurePosixPath) -> bool:
        """Test if a source-relative path ends with an include-path suffix."""
        candidate_parts = candidate.parts
        suffix_parts = suffix.parts
        if len(suffix_parts) > len(candidate_parts):
            return False
        return candidate_parts[-len(suffix_parts) :] == suffix_parts

    def ambiguous_include_error(
        self, include_path: str, context: str, candidates: Iterable[PurePosixPath]
    ) -> UtilityError:
        """Create a detailed ambiguity error for an include path."""
        lines = [
            f"Ambiguous include path in {context}: {include_path}",
            "The include can refer to more than one library source file:",
        ]
        lines.extend(f"  - {self.source_dir / candidate}" for candidate in candidates)
        lines.append("Use an include path that uniquely identifies the target file.")
        return UtilityError("\n".join(lines))


class FixIncludePaths:
    """Fix wrong include paths in C++ source files."""

    def __init__(self, config: FixIncludePathsConfig, print_verbose: Callable[[str], None]) -> None:
        self.config = config
        self.print_verbose = print_verbose
        self.file_update = FileUpdate(print_verbose)
        self.source_map = LibrarySourceMap.collect(config)

    def run(self) -> None:
        """Run the fixer for source and unit test files."""
        updates: list[PendingIncludePathUpdate] = []
        self.print_verbose("Fixing include paths in library sources.")
        for path in self.config.source_files(self.config.source_dir):
            update = self.process_file(path, is_unittest=False)
            if update is not None:
                updates.append(update)
        self.print_verbose("Fixing include paths in unit tests.")
        for path in self.config.source_files(self.config.unittest_dir):
            update = self.process_file(path, is_unittest=True)
            if update is not None:
                updates.append(update)
        for update in updates:
            self.file_update.write_if_changed(update.path, update.text)

    def process_file(self, path: Path, *, is_unittest: bool) -> PendingIncludePathUpdate | None:
        """Fix include paths in one file."""
        display_path = self.display_path(path)
        self.print_verbose(f"Processing file: {display_path}")
        text = read_safe_text(path, "C++ source file", FileUpdate.MAX_COMPARE_FILE_SIZE)
        try:
            include_block = IncludeBlockScanner.find_include_block(
                text, self.config.is_header_file(path), include_first_cpp=True
            )
        except ValueError as error:
            raise UtilityError(f"{display_path}: {error}") from None
        if include_block is None:
            self.print_verbose("  no include block found.")
            return None

        replacement = self.fix_include_block(path, include_block.text, is_unittest=is_unittest)
        new_text = text[: include_block.start] + replacement + text[include_block.end :]
        if new_text == text:
            return None
        return PendingIncludePathUpdate(path, new_text)

    def fix_include_block(self, path: Path, include_block: str, *, is_unittest: bool) -> str:
        """Fix all relevant include lines in one include block."""
        result = []
        for line in include_block.splitlines(keepends=True):
            include_line = IncludeBlockScanner.parse_include_line(line)
            if include_line is None:
                result.append(line)
                continue
            if is_unittest:
                result.append(self.fix_unittest_include(path, include_line))
            else:
                result.append(self.fix_source_include(path, include_line))
        return "".join(result)

    def fix_source_include(self, path: Path, include_line: IncludeLine) -> str:
        """Fix one include from a library source file."""
        if include_line.is_global:
            return include_line.render()
        if self.existing_relative_library_include(path, include_line.path):
            return include_line.render()
        target = self.resolve_library_target(path, include_line.path, self.context(path, include_line))
        fixed_path = self.relative_path_from_file(path, target)
        return include_line.render(path=fixed_path, is_global=False)

    def fix_unittest_include(self, path: Path, include_line: IncludeLine) -> str:
        """Fix one include from a unit test source file."""
        if self.existing_relative_unittest_include(path, include_line.path):
            return include_line.render()
        target = self.resolve_optional_library_target(path, include_line.path, self.context(path, include_line))
        if target is not None:
            return include_line.render(path=target.as_posix(), is_global=True)
        if include_line.path.startswith("erbsland/"):
            return include_line.render(is_global=True)
        if not include_line.is_global and self.looks_like_project_include(include_line.path):
            self.source_map.path_for_include(include_line.path, self.context(path, include_line))
        return include_line.render()

    def resolve_optional_library_target(self, path: Path, include_path: str, context: str) -> PurePosixPath | None:
        """Resolve an include path to a library source file, if it is one."""
        relative_target = self.relative_existing_library_target(path, include_path)
        if relative_target is not None:
            return relative_target
        if self.source_map.has_relative_path(include_path):
            return PurePosixPath(include_path)
        filename = PurePosixPath(include_path).name
        if self.source_map.has_filename(filename):
            return self.source_map.path_for_include(include_path, context)
        return None

    def resolve_library_target(self, path: Path, include_path: str, context: str) -> PurePosixPath:
        """Resolve an include path to a library source file, or stop with a detailed error."""
        relative_target = self.relative_existing_library_target(path, include_path)
        if relative_target is not None:
            return relative_target
        return self.source_map.path_for_include(include_path, context)

    def relative_existing_library_target(self, path: Path, include_path: str) -> PurePosixPath | None:
        """Resolve an include path relative to the current file if it points into the library sources."""
        target = self.resolve_relative_file(path, include_path)
        if target is None:
            return None
        if not target.is_relative_to(self.config.source_dir):
            return None
        return PurePosixPath(target.relative_to(self.config.source_dir).as_posix())

    def existing_relative_library_include(self, path: Path, include_path: str) -> bool:
        """Test if an include already points to an existing library file relative to the current file."""
        return self.relative_existing_library_target(path, include_path) is not None

    def existing_relative_unittest_include(self, path: Path, include_path: str) -> bool:
        """Test if an include points to a local unit test source file."""
        relative_target = self.resolve_relative_file(path, include_path)
        if relative_target is not None and relative_target.is_relative_to(self.config.unittest_dir):
            return True
        clean_path = LibrarySourceMap.clean_posix_path(include_path)
        if clean_path is None:
            return False
        for root in self.unittest_include_roots():
            candidate = (root / clean_path).resolve(strict=False)
            if self.is_regular_file(candidate) and candidate.is_relative_to(self.config.unittest_dir):
                return True
        return False

    def unittest_include_roots(self) -> tuple[Path, ...]:
        """Return local include roots used by the unit test target."""
        roots = [self.config.unittest_dir]
        src_root = self.config.unittest_dir / "src"
        if src_root.is_dir():
            roots.append(src_root)
        return tuple(roots)

    def resolve_relative_file(self, path: Path, include_path: str) -> Path | None:
        """Resolve an include path relative to the current file without accepting directories or symlinks."""
        target = (path.parent / include_path).resolve(strict=False)
        if not self.is_regular_file(target):
            return None
        return target

    @staticmethod
    def is_regular_file(path: Path) -> bool:
        """Test if a path is a regular non-symlink file."""
        return path.exists() and path.is_file() and not path.is_symlink()

    @staticmethod
    def looks_like_project_include(include_path: str) -> bool:
        """Test if an unresolved quoted include should have been a project header."""
        suffix = PurePosixPath(include_path).suffix
        return suffix in {".h", ".hpp", ".hxx", ".tpp", ".cpp"}

    def relative_path_from_file(self, path: Path, target: PurePosixPath) -> str:
        """Create the shortest relative include path from a source file to a target."""
        absolute_target = self.config.source_dir / target
        return os.path.relpath(absolute_target, path.parent).replace(os.sep, "/")

    def context(self, path: Path, include_line: IncludeLine) -> str:
        """Create a detailed context for error messages."""
        start_quote, end_quote = ("<", ">") if include_line.is_global else ('"', '"')
        return f'{self.display_path(path)}: #include {start_quote}{include_line.path}{end_quote}'

    def display_path(self, path: Path) -> Path:
        """Create a user-friendly display path relative to the project."""
        try:
            return path.relative_to(self.config.project_dir)
        except ValueError:
            return path
