# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import os
from pathlib import Path

from lib.error import UtilityError
from lib.path_safety import read_safe_text, require_directory, require_safe_existing_file, resolve_project_path

from .config import AntiPatternConfig
from .model import Finding, Suppression
from .rules import RULES
from .source import SourceFile


class AntiPatternScanner:
    """Scan configured first-party C++ sources for anti-patterns."""

    def __init__(self, config: AntiPatternConfig) -> None:
        self.config = config

    def discover_paths(self, requested_paths: list[Path] | None = None) -> tuple[Path, ...]:
        """Resolve explicit targets or discover files below configured source roots."""
        roots = self._requested_roots(requested_paths) if requested_paths else self.config.source_directories
        result: set[Path] = set()
        for root in roots:
            if root.is_file():
                result.add(root)
                continue
            for directory, directory_names, file_names in os.walk(root, followlinks=False):
                directory_path = Path(directory)
                directory_names[:] = sorted(
                    name for name in directory_names if not (directory_path / name).is_symlink()
                )
                for file_name in sorted(file_names):
                    path = directory_path / file_name
                    if path.is_symlink() or path.suffix not in self.config.file_suffixes:
                        continue
                    result.add(path)
        return tuple(
            sorted(result, key=lambda path: path.relative_to(self.config.project_directory).as_posix().casefold())
        )

    def _requested_roots(self, requested_paths: list[Path]) -> tuple[Path, ...]:
        roots = []
        for requested_path in requested_paths:
            path = resolve_project_path(self.config.project_directory, requested_path, "Requested Scan Path")
            if path.is_dir():
                require_directory(path, "Requested Scan Path")
            else:
                require_safe_existing_file(path, "Requested Scan Path")
                if not path.is_file():
                    raise UtilityError(f"Requested scan path does not exist: {requested_path}")
                if path.suffix not in self.config.file_suffixes:
                    raise UtilityError(f"Requested scan file has an unsupported suffix: {requested_path}")
            roots.append(path)
        return tuple(roots)

    def scan(self, requested_paths: list[Path] | None = None) -> tuple[Finding, ...]:
        """Scan all selected sources and return deterministically ordered findings."""
        findings: list[Finding] = []
        for path in self.discover_paths(requested_paths):
            relative_path = path.relative_to(self.config.project_directory)
            text = read_safe_text(path, "Anti-pattern source file")
            source = SourceFile(relative_path, text)
            for rule in RULES:
                for candidate in rule.scan(source):
                    if source.is_in_os_conditional(candidate.start):
                        continue
                    end = max(candidate.end, candidate.start + 1)
                    inline_reason = source.inline_suppression(rule.info.identifier, candidate.start, end)
                    suppression = Suppression("inline", inline_reason) if inline_reason is not None else None
                    if suppression is None:
                        configured = self.config.suppression_for(rule.info.identifier, relative_path)
                        if configured is not None:
                            suppression = Suppression("configuration", configured.reason)
                    findings.append(
                        Finding(
                            candidate=candidate,
                            path=relative_path,
                            line_number=source.line_number(candidate.start),
                            end_line_number=source.line_number(end - 1),
                            snippet=source.snippet(candidate.start, end),
                            suppression=suppression,
                        )
                    )
        return tuple(sorted(findings, key=self._sort_key))

    @staticmethod
    def _sort_key(finding: Finding) -> tuple[int, str, str, int]:
        return (
            int(finding.candidate.rule.severity),
            finding.candidate.rule.identifier,
            finding.path.as_posix().casefold(),
            finding.line_number,
        )
