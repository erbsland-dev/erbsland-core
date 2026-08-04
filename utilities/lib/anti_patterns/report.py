# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from collections.abc import Iterable
from dataclasses import dataclass
from pathlib import Path

from .model import Finding, Suppression


@dataclass(frozen=True)
class _ReportRow:
    """One rendered report row, possibly representing multiple findings."""

    finding: Finding
    finding_count: int


def create_report(findings: Iterable[Finding], *, limit: int, show_suppressed: bool) -> str:
    """Render a concise, stable, agent-focused report."""
    all_findings = tuple(findings)
    active_count = sum(not finding.suppressed for finding in all_findings)
    suppressed_count = len(all_findings) - active_count
    visible = tuple(finding for finding in all_findings if show_suppressed or not finding.suppressed)
    displayed = _report_rows(visible)[:limit]
    lines = [finding_line(row.finding, additional_count=row.finding_count - 1) for row in displayed]
    rule_infos = {row.finding.candidate.rule.identifier: row.finding.candidate.rule for row in displayed}
    if rule_infos:
        lines.append("Legend:")
        for identifier in sorted(rule_infos):
            lines.append(f"{identifier}: {rule_infos[identifier].documentation_path.as_posix()}")
    lines.append(
        f"Summary: {active_count} active, {suppressed_count} suppressed, " f"{len(displayed)} shown (limit {limit})."
    )
    return "\n".join(lines) + "\n"


def _report_rows(findings: Iterable[Finding]) -> tuple[_ReportRow, ...]:
    """Group special findings and return report rows in display order."""
    grouped: dict[tuple[Path, Suppression | None], list[Finding]] = {}
    rows: list[_ReportRow] = []
    for finding in findings:
        if finding.candidate.rule.identifier in {"missing_api_documentation", "missing_default_group_comment"}:
            grouped.setdefault((finding.path, finding.suppression), []).append(finding)
        else:
            rows.append(_ReportRow(finding, 1))
    for grouped_findings in grouped.values():
        first = min(
            grouped_findings,
            key=lambda finding: (
                finding.line_number,
                finding.end_line_number,
                finding.candidate.start,
                finding.candidate.end,
                finding.snippet,
            ),
        )
        rows.append(_ReportRow(first, len(grouped_findings)))
    return tuple(sorted(rows, key=_report_row_sort_key))


def _report_row_sort_key(row: _ReportRow) -> tuple[int, int, str, str, int]:
    """Return the stable priority key for a rendered report row."""
    finding = row.finding
    return (
        int(finding.candidate.rule.severity),
        -row.finding_count,
        finding.candidate.rule.identifier,
        finding.path.as_posix().casefold(),
        finding.line_number,
    )


def finding_line(finding: Finding, *, additional_count: int = 0) -> str:
    """Render one finding as a single physical line."""
    prefix = "SUPPRESSED " if finding.suppressed else ""
    rule = finding.candidate.rule
    line = (
        f"{prefix}{rule.severity.display_name} {rule.identifier} "
        f"{finding.path.as_posix()}:{finding.line_number} | {finding.snippet}"
    )
    if additional_count:
        line += f" | {additional_count} more found"
    if finding.suppression is not None:
        reason = compact_text(finding.suppression.reason, 80)
        line += f" | accepted: {reason}"
    return line


def compact_text(text: str, maximum_length: int) -> str:
    """Collapse and bound user-controlled report text."""
    result = " ".join(text.replace("\x00", "\\0").split())
    if len(result) <= maximum_length:
        return result
    return result[: maximum_length - 1].rstrip() + "…"
