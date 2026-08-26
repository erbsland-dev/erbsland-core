# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import os
from dataclasses import replace
from pathlib import Path

from lib.error import UtilityError
from lib.path_safety import read_safe_text, require_directory, require_safe_existing_file, resolve_project_path

from .cache import AntiPatternCache
from .config import AntiPatternConfig
from .model import Finding, Suppression
from .rules import RULES
from .source import SourceFile


class AntiPatternScanner:
    """Scan configured first-party C++ sources for anti-patterns."""

    def __init__(self, config: AntiPatternConfig, *, use_cache: bool = True) -> None:
        self.config = config
        self.cache = AntiPatternCache(config.project_directory) if use_cache else None
        self.cache_hits = 0
        self.cache_misses = 0

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
        paths = self.discover_paths(requested_paths)
        relative_paths: set[Path] = set()
        for path in paths:
            relative_path = path.relative_to(self.config.project_directory)
            relative_paths.add(relative_path)
            source_findings = self._cached_findings(path, relative_path)
            findings.extend(self._apply_configured_suppressions(relative_path, source_findings))
        if self.cache is not None:
            if requested_paths is None:
                self.cache.retain(relative_paths)
            self.cache.save()
        return tuple(sorted(findings, key=self._sort_key))

    def _cached_findings(self, path: Path, relative_path: Path) -> tuple[Finding, ...]:
        """Load valid cached findings or scan one source and update its cache entry."""
        signature = AntiPatternCache.file_signature(path)
        if self.cache is not None:
            cached = self.cache.get(relative_path, signature)
            if cached is not None:
                self.cache_hits += 1
                return cached
        self.cache_misses += 1
        findings = self._scan_source(path, relative_path)
        if self.cache is not None and signature == AntiPatternCache.file_signature(path):
            self.cache.put(relative_path, signature, findings)
        return findings

    @staticmethod
    def _scan_source(path: Path, relative_path: Path) -> tuple[Finding, ...]:
        """Read, parse, and scan one source file exactly once."""
        findings: list[Finding] = []
        text = read_safe_text(path, "Anti-pattern source file")
        source = SourceFile(relative_path, text)
        for rule in RULES:
            for candidate in rule.scan(source):
                if source.is_in_os_conditional(candidate.start):
                    continue
                end = max(candidate.end, candidate.start + 1)
                inline_reason = source.inline_suppression(rule.info.identifier, candidate.start, end)
                suppression = Suppression("inline", inline_reason) if inline_reason is not None else None
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
        return tuple(findings)

    def _apply_configured_suppressions(self, relative_path: Path, findings: tuple[Finding, ...]) -> tuple[Finding, ...]:
        """Apply current configuration suppressions to fresh or cached source findings."""
        result: list[Finding] = []
        for finding in findings:
            if finding.suppression is not None:
                result.append(finding)
                continue
            configured = self.config.suppression_for(finding.candidate.rule.identifier, relative_path)
            if configured is None:
                result.append(finding)
                continue
            result.append(replace(finding, suppression=Suppression("configuration", configured.reason)))
        return tuple(result)

    @staticmethod
    def _sort_key(finding: Finding) -> tuple[int, str, str, int]:
        return (
            int(finding.candidate.rule.severity),
            finding.candidate.rule.identifier,
            finding.path.as_posix().casefold(),
            finding.line_number,
        )
