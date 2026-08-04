# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from dataclasses import dataclass
from functools import cached_property
from pathlib import Path, PurePosixPath

from lib.config import read_elcl_file, validate_file_suffixes, validate_source_relative_path
from lib.error import UtilityError
from lib.path_safety import require_directory, require_safe_existing_file, resolve_project_path

from .rules import RULES_BY_IDENTIFIER


@dataclass(frozen=True)
class ConfiguredSuppression:
    """One configured rule suppression for a project-relative path."""

    rule_identifier: str
    excluded_path: str
    reason: str
    directory: bool = False

    def matches(self, relative_path: Path) -> bool:
        """Test whether this suppression covers a relative source path."""
        path = PurePosixPath(relative_path.as_posix())
        if "*" in self.excluded_path or "?" in self.excluded_path:
            return path.full_match(self.excluded_path, case_sensitive=True)
        configured_path = PurePosixPath(self.excluded_path)
        if self.directory:
            return path == configured_path or configured_path in path.parents
        return path == configured_path


@dataclass(frozen=True)
class AntiPatternConfig:
    """Validated anti-pattern scanner configuration."""

    project_directory: Path
    source_directories: tuple[Path, ...]
    file_suffixes: frozenset[str]
    suppressions: tuple[ConfiguredSuppression, ...]

    @classmethod
    def read(cls, project_directory: Path, config_path: Path) -> "AntiPatternConfig":
        """Read and validate the scanner configuration."""
        project_directory = project_directory.resolve()
        document = read_elcl_file(config_path)
        main = document["main"]
        source_directories = tuple(
            resolve_project_path(project_directory, path, "Anti-Pattern Source Directory")
            for path in main.get_list("source_directories", str)
        )
        file_suffixes = frozenset(main.get_list("file_suffixes", str))
        for path in source_directories:
            require_directory(path, "Anti-Pattern Source Directory")
        if not source_directories:
            raise UtilityError("No anti-pattern source directories configured.")
        if not file_suffixes:
            raise UtilityError("No anti-pattern file suffixes configured.")
        validate_file_suffixes(file_suffixes, "Anti-Pattern File Suffixes")
        suppressions = cls._read_suppressions(project_directory, document)
        return cls(project_directory, source_directories, file_suffixes, suppressions)

    @staticmethod
    def _read_suppressions(project_directory: Path, document) -> tuple[ConfiguredSuppression, ...]:
        rule_section = document.get("rule")
        if rule_section is None:
            return ()
        result: list[ConfiguredSuppression] = []
        seen: set[tuple[str, str]] = set()
        for configured_rule in rule_section:
            identifier = configured_rule.name.as_text()
            if identifier not in RULES_BY_IDENTIFIER:
                raise UtilityError(f"Unknown anti-pattern rule in configuration: {identifier}")
            for entry in configured_rule:
                reason = entry.get_text("reason").strip()
                if not reason:
                    raise UtilityError(f"Configured suppression reason must not be empty for rule {identifier}.")
                path_text = entry.get_text("excluded_path").strip()
                label = f"Excluded path for {identifier}"
                if not path_text:
                    raise UtilityError(f"{label} must not be empty.")
                validate_source_relative_path(path_text, label)
                directory = False
                if "*" not in path_text and "?" not in path_text:
                    absolute_path = resolve_project_path(project_directory, path_text, label)
                    if absolute_path.is_dir():
                        require_directory(absolute_path, label)
                        directory = True
                    else:
                        require_safe_existing_file(absolute_path, label)
                        if not absolute_path.is_file():
                            raise UtilityError(f"{label} does not exist: {absolute_path}")
                    path_text = absolute_path.relative_to(project_directory).as_posix()
                key = (identifier, path_text)
                if key in seen:
                    raise UtilityError(f"Duplicate configured suppression for {identifier}: {path_text}")
                seen.add(key)
                result.append(ConfiguredSuppression(identifier, path_text, reason, directory))
        return tuple(result)

    def suppression_for(self, rule_identifier: str, relative_path: Path) -> ConfiguredSuppression | None:
        """Find the first configured suppression matching a finding."""
        return next(
            (
                suppression
                for suppression in self._suppressions_by_rule.get(rule_identifier, ())
                if suppression.matches(relative_path)
            ),
            None,
        )

    @cached_property
    def _suppressions_by_rule(self) -> dict[str, tuple[ConfiguredSuppression, ...]]:
        """Group configured suppressions by rule for repeated finding lookups."""
        return {
            identifier: tuple(item for item in self.suppressions if item.rule_identifier == identifier)
            for identifier in RULES_BY_IDENTIFIER
        }
