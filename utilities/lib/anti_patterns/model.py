# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from dataclasses import dataclass
from enum import IntEnum
from pathlib import Path


class Severity(IntEnum):
    """Severity of an anti-pattern finding, ordered for report sorting."""

    High = 0
    Medium = 1
    Low = 2

    @property
    def display_name(self) -> str:
        """Return the stable uppercase name used in reports."""
        return self.name.upper()


@dataclass(frozen=True)
class RuleInfo:
    """Stable metadata for one anti-pattern rule."""

    identifier: str
    title: str
    severity: Severity

    @property
    def documentation_path(self) -> Path:
        """Return the project-relative guideline page for this rule."""
        return Path("doc/guidelines/anti_patterns") / f"{self.identifier}.rst"


@dataclass(frozen=True)
class Candidate:
    """One source location reported by a rule before suppression handling."""

    rule: RuleInfo
    start: int
    end: int


@dataclass(frozen=True)
class Suppression:
    """The accepted-exception metadata for one finding."""

    source: str
    reason: str


@dataclass(frozen=True)
class Finding:
    """One complete anti-pattern finding."""

    candidate: Candidate
    path: Path
    line_number: int
    end_line_number: int
    snippet: str
    suppression: Suppression | None = None

    @property
    def suppressed(self) -> bool:
        """Test whether this finding is explicitly accepted."""
        return self.suppression is not None
