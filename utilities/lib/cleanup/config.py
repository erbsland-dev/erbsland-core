# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

from lib.config import read_elcl_file, validate_file_suffixes, validate_local_names
from lib.error import UtilityError
from lib.path_safety import require_directory, resolve_project_path


@dataclass(frozen=True)
class CleanupConfig:
    """Configuration shared by all cleanup passes."""

    RE_CMAKE_VERSION = re.compile(r"\d+(?:\.\d+){1,3}")

    project_dir: Path
    library_source_dir: Path
    unit_test_source_dir: Path
    cmake_source_dirs: tuple[Path, ...]
    excluded_names: frozenset[str]
    scanned_file_suffixes: frozenset[str]
    header_file_suffixes: frozenset[str]
    cmake_minimum_version: str
    cmake_header: str

    @classmethod
    def read(cls, project_dir: Path, config_file: Path) -> "CleanupConfig":
        """Read the cleanup configuration from an ELCL file."""
        config = read_elcl_file(config_file)
        main_config = config["main"]
        cmake_config = config["cmake"]
        project_dir = project_dir.resolve()
        result = cls(
            project_dir=project_dir,
            library_source_dir=resolve_project_path(
                project_dir, main_config.get_text("library_source_directory"), "Library Source Directory"
            ),
            unit_test_source_dir=resolve_project_path(
                project_dir, main_config.get_text("unit_test_source_directory"), "Unit Test Source Directory"
            ),
            cmake_source_dirs=tuple(
                resolve_project_path(project_dir, directory, "CMake Source Directories")
                for directory in main_config.get_list("cmake_source_directories", str)
            ),
            excluded_names=frozenset(main_config.get_list("excluded_names", str, default=[])),
            scanned_file_suffixes=frozenset(main_config.get_list("scanned_file_suffixes", str, default=[])),
            header_file_suffixes=frozenset(main_config.get_list("header_file_suffixes", str, default=[])),
            cmake_minimum_version=cmake_config.get_text("minimum_version", default=""),
            cmake_header=cmake_config.get_text("header", default=""),
        )
        result.validate()
        return result

    def validate(self) -> None:
        """Validate the cleanup configuration."""
        require_directory(self.library_source_dir, "Library Source Directory")
        require_directory(self.unit_test_source_dir, "Unit Test Source Directory")
        if not self.cmake_source_dirs:
            raise UtilityError("No CMake source directories configured.")
        for directory in self.cmake_source_dirs:
            require_directory(directory, "CMake Source Directories")
        if not self.scanned_file_suffixes:
            raise UtilityError("No scanned file suffixes configured.")
        validate_file_suffixes(self.scanned_file_suffixes, "Scanned File Suffixes")
        if not self.header_file_suffixes:
            raise UtilityError("No header file suffixes configured.")
        validate_file_suffixes(self.header_file_suffixes, "Header File Suffixes")
        validate_local_names(self.excluded_names, "Excluded Names")
        if not self.cmake_minimum_version:
            raise UtilityError("No CMake minimum version configured.")
        if not self.RE_CMAKE_VERSION.fullmatch(self.cmake_minimum_version):
            raise UtilityError(f"Invalid CMake minimum version: {self.cmake_minimum_version}")
        if not self.cmake_header:
            raise UtilityError("No CMake header configured.")
        for line in self.cmake_header.splitlines():
            if line.strip() and not line.strip().startswith("#"):
                raise UtilityError("CMake header lines must be comments.")

    def target_for_directory(self, path: Path) -> str:
        """Infer the target name for a CMakeLists.txt directory."""
        if path == self.library_source_dir or path.is_relative_to(self.library_source_dir):
            return "erbsland-core"
        if path == self.unit_test_source_dir or path.is_relative_to(self.unit_test_source_dir):
            return "erbsland-core-unittest"
        raise UtilityError(f"Cannot infer CMake target for directory: {path}")

    def local_source_files(self, directory: Path) -> list[str]:
        """List local source files that must be present in CMake target_sources."""
        return sorted(
            path.name
            for path in directory.iterdir()
            if path.is_file()
            and not path.is_symlink()
            and path.suffix in self.scanned_file_suffixes
            and path.name not in self.excluded_names
        )

    def cmake_header_text(self, year: int) -> str:
        """Create the configured CMake header for the given year."""
        return self.cmake_header.replace("{year}", str(year)).strip()
