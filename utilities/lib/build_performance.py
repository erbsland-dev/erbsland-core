# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Build-performance audit and compiler-report helpers."""

from __future__ import annotations

import json
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable

from lib.include_dependencies import (
    DirectInclude,
    IncludeGraph,
    direct_includes,
    discover_source_paths,
    infer_role,
    resolve_local_include,
)


@dataclass(frozen=True)
class AuditFile:
    """Computed audit metadata for one library source file."""

    path: str
    role: str
    platform: str
    owner: str | None
    direct_includes: tuple[DirectInclude, ...]


def infer_platform(path: Path) -> str:
    """Infer a platform restriction from a source file name."""
    path_text = path.as_posix().casefold()
    if "windows" in path_text:
        return "windows"
    if "posix" in path_text:
        return "posix"
    if "linux" in path_text:
        return "linux"
    if "macos" in path_text or "darwin" in path_text or "apple" in path_text:
        return "macos"
    return "all"


def find_template_owners(paths: Iterable[Path]) -> dict[Path, Path]:
    """Map every template implementation file to its directly including header."""
    path_list = list(paths)
    template_paths = {path.resolve() for path in path_list if path.suffix == ".tpp"}
    owners: dict[Path, list[Path]] = defaultdict(list)
    for source in path_list:
        if source.suffix not in {".hpp", ".tpp"}:
            continue
        for include in direct_includes(source.read_text(encoding="utf-8")):
            target = resolve_local_include(source, include)
            if target in template_paths:
                owners[target].append(source.resolve())
    result: dict[Path, Path] = {}
    for template_path in template_paths:
        candidates = owners.get(template_path, [])
        header_candidates = [candidate for candidate in candidates if candidate.suffix == ".hpp"]
        if len(header_candidates) == 1:
            result[template_path] = header_candidates[0]
        elif len(candidates) == 1:
            result[template_path] = candidates[0]
    return result


def collect_audit_files(project_directory: Path) -> list[AuditFile]:
    """Collect computed metadata for every audited library source file."""
    source_directory = project_directory / "src" / "erbsland"
    paths = discover_source_paths(source_directory)
    owners = find_template_owners(paths)
    result = []
    for path in paths:
        owner = owners.get(path.resolve())
        result.append(
            AuditFile(
                path=path.relative_to(project_directory).as_posix(),
                role=infer_role(path),
                platform=infer_platform(path),
                owner=owner.relative_to(project_directory.resolve()).as_posix() if owner else None,
                direct_includes=direct_includes(path.read_text(encoding="utf-8")),
            )
        )
    return result


def validate_source_tree(project_directory: Path) -> list[str]:
    """Validate structural source invariants from the current tree without persistent audit state."""
    issues: list[str] = []
    files = collect_audit_files(project_directory)
    for file in files:
        if file.role == "template" and not file.owner:
            issues.append(f"Template implementation has no unique owning header: {file.path}")
    source_directory = project_directory / "src" / "erbsland"
    for forward_path in source_directory.rglob("*_fwd.hpp"):
        implementation_path = forward_path.with_name(forward_path.name.removesuffix("_fwd.hpp") + ".hpp")
        if not implementation_path.is_file():
            continue
        includes = direct_includes(implementation_path.read_text(encoding="utf-8"))
        if not any(
            resolve_local_include(implementation_path, include) == forward_path.resolve() for include in includes
        ):
            issues.append(
                f"Implementation header must include its forward header: "
                f"{implementation_path.relative_to(project_directory).as_posix()}"
            )
    return issues


def add_forward_include_text(text: str, forward_name: str) -> str:
    """Add a matching forward-header include directly after `#pragma once`."""
    include_line = f'#include "{forward_name}"'
    if include_line in text:
        return text
    marker = "#pragma once\n"
    if marker not in text:
        raise ValueError("Header has no #pragma once marker.")
    return text.replace(marker, f"{marker}\n{include_line}\n", 1)


def parse_ninja_dependencies(text: str, project_directory: Path) -> dict[str, int]:
    """Count translation units depending on each project source from `ninja -t deps` output."""
    root = (project_directory / "src" / "erbsland").resolve()
    counts: dict[str, int] = defaultdict(int)
    seen: set[str] = set()
    for line in text.splitlines():
        if line and not line.startswith(" "):
            seen.clear()
            continue
        if not line.startswith("    "):
            continue
        candidate = Path(line.strip())
        try:
            relative = candidate.resolve().relative_to(root).as_posix()
        except (OSError, ValueError):
            continue
        if relative not in seen:
            counts[relative] += 1
            seen.add(relative)
    return dict(counts)


def transitive_dependency_counts(files: Iterable[AuditFile], project_directory: Path) -> dict[str, int]:
    """Count reachable project sources for each audited file's direct-include graph."""
    file_list = list(files)
    graph = IncludeGraph.from_project(project_directory)
    return {
        file.path.removeprefix("src/erbsland/"): len(graph.reachable(file.path.removeprefix("src/erbsland/")))
        for file in file_list
    }


def trace_source_events(document: dict[str, Any]) -> Iterable[tuple[str, int]]:
    """Yield inclusive source-event durations from Clang time-trace JSON."""
    stacks: dict[tuple[int, int], list[tuple[int, str]]] = defaultdict(list)
    for event in document.get("traceEvents", []):
        if event.get("name") != "Source":
            continue
        phase = event.get("ph")
        if phase == "X" and isinstance(event.get("dur"), int):
            detail = event.get("args", {}).get("detail")
            if isinstance(detail, str):
                yield detail, event["dur"]
            continue
        key = (int(event.get("pid", 0)), int(event.get("tid", 0)))
        if phase in {"b", "B"}:
            detail = event.get("args", {}).get("detail")
            if isinstance(detail, str):
                stacks[key].append((int(event.get("ts", 0)), detail))
        elif phase in {"e", "E"} and stacks[key]:
            start, detail = stacks[key].pop()
            yield detail, max(0, int(event.get("ts", start)) - start)


def collect_trace_metrics(build_directory: Path, project_directory: Path) -> dict[str, dict[str, int]]:
    """Aggregate project-header costs from Clang time-trace files."""
    root = (project_directory / "src" / "erbsland").resolve()
    metrics: dict[str, dict[str, int]] = defaultdict(lambda: {"duration_us": 0, "count": 0})
    for path in build_directory.rglob("*.json"):
        if path.name == "compile_commands.json":
            continue
        try:
            document = json.loads(path.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError):
            continue
        if not isinstance(document, dict) or "traceEvents" not in document:
            continue
        for source, duration in trace_source_events(document):
            try:
                relative = Path(source).resolve().relative_to(root).as_posix()
            except (OSError, ValueError):
                continue
            metrics[relative]["duration_us"] += duration
            metrics[relative]["count"] += 1
    return dict(metrics)


def collect_compiler_metrics(build_directory: Path) -> tuple[int, int]:
    """Collect Clang's cumulative compiler duration and translation-unit count."""
    total = 0
    count = 0
    for path in build_directory.rglob("*.json"):
        if path.name == "compile_commands.json":
            continue
        try:
            document = json.loads(path.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError):
            continue
        for event in document.get("traceEvents", []):
            if event.get("name") == "Total ExecuteCompiler" and isinstance(event.get("dur"), int):
                total += event["dur"]
                count += 1
    return total, count


def collect_compiler_total_us(build_directory: Path) -> int:
    """Sum Clang's per-translation-unit `Total ExecuteCompiler` durations."""
    return collect_compiler_metrics(build_directory)[0]


def scan_report(
    files: Iterable[AuditFile],
    fanout: dict[str, int],
    trace_metrics: dict[str, dict[str, int]],
    transitive_counts: dict[str, int] | None = None,
    compiler_duration_us: int = 0,
    translation_units: int = 0,
) -> dict[str, Any]:
    """Combine source inventory and compiler measurements into a ranked report."""
    entries = []
    for file in files:
        relative = file.path.removeprefix("src/erbsland/")
        trace = trace_metrics.get(relative, {})
        duration = int(trace.get("duration_us", 0))
        entry = {
            "path": file.path,
            "role": file.role,
            "direct_includes": len(file.direct_includes),
            "transitive_dependencies": int((transitive_counts or {}).get(relative, 0)),
            "fanout": int(fanout.get(relative, 0)),
            "parse_count": int(trace.get("count", 0)),
            "parse_duration_us": duration,
        }
        entries.append(entry)
    entries.sort(
        key=lambda entry: (
            -entry["parse_duration_us"],
            -entry["fanout"],
            -entry["direct_includes"],
            entry["path"].casefold(),
        )
    )
    return {
        "schema": 1,
        "summary": {
            "audited_files": len(entries),
            "translation_units": translation_units,
            "compiler_duration_us": compiler_duration_us,
        },
        "files": entries,
    }


def scan_report_text(document: dict[str, Any], limit: int = 100) -> str:
    """Render a concise human-readable scan report."""
    summary = document.get("summary", {})
    lines = [
        f"Audited {summary.get('audited_files', 0)} files across "
        f"{summary.get('translation_units', 0)} translation units; cumulative compiler time "
        f"{summary.get('compiler_duration_us', 0) / 1_000_000:.3f} s.",
        "parse_s  fanout  transit  direct  path",
    ]
    for entry in document.get("files", [])[:limit]:
        lines.append(
            f"{entry['parse_duration_us'] / 1_000_000:7.3f}  {entry['fanout']:6d}  "
            f"{entry.get('transitive_dependencies', 0):7d}  {entry['direct_includes']:6d}  {entry['path']}"
        )
    return "\n".join(lines) + "\n"
