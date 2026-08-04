#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0
"""Generate the deterministic ELCL profiling corpus and its embedded C++ source."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

PROFILE_DIR = Path(__file__).resolve().parent.parent
DATA_DIR = PROFILE_DIR / "data"
OUTPUT_PATH = PROFILE_DIR / "src" / "EmbeddedDocuments.cpp"


def title_name(value: str) -> str:
    """Convert regular ELCL names to their human-readable equivalent."""
    if '"' in value:
        return value
    return " . ".join(part.strip().replace("_", " ").title() for part in value.split("."))


def pretty_format(text: str) -> str:
    """Apply the profiling-corpus ELCL layout."""
    source = [line for line in text.splitlines() if not line.startswith("@features:")]
    assignment_names = []
    for line in source:
        match = re.match(r"^([A-Za-z][A-Za-z0-9_ ]*)\s*:", line)
        if match:
            assignment_names.append(title_name(match.group(1)))
    width = min(32, max((len(name) for name in assignment_names), default=4))
    result = []
    for line in source:
        match = re.match(r"^\*\[\s*(.*?)\s*\]$", line)
        if match:
            prefix = f"--*[ {title_name(match.group(1))} ]*"
            result.append(prefix + "-" * max(3, 79 - len(prefix)))
            continue
        match = re.match(r"^\[\s*(.*?)\s*\]$", line)
        if match:
            prefix = f"---[ {title_name(match.group(1))} ]"
            result.append(prefix + "-" * max(3, 79 - len(prefix)))
            continue
        match = re.match(r"^([A-Za-z][A-Za-z0-9_ ]*)\s*:(.*)$", line)
        if match:
            name = title_name(match.group(1))
            result.append(f"{name:<{width}} :{match.group(2)}")
            continue
        result.append(line)
    return "\n".join(result)


def scalar_document() -> str:
    lines = [
        '@version: "1.0"',
        "# Dense scalar values and value lists.",
        "",
    ]
    for index in range(90):
        lines.extend(
            [
                f"[scalar batch {index:03d}]",
                f"decimal value: {index * 7919 - 200000}",
                f"hexadecimal value: 0x{index * 65537 + 0x1234:x}",
                f"binary value: 0b{index * 257 + 3:b}",
                f"float value: {index + 1}.{index:03d}e-{index % 8}",
                f"boolean value: {'enabled' if index % 2 else 'disabled'}",
                f'text value: "Telemetry sample {index:03d} – Καλημέρα – おはようございます – 😀"',
                f"date value: 2026-{index % 12 + 1:02d}-{index % 27 + 1:02d}",
                f"time value: {index % 24:02d}:{index * 7 % 60:02d}:{index * 13 % 60:02d}",
                f"date time value: 2026-{index % 12 + 1:02d}-{index % 27 + 1:02d} "
                f"{index % 24:02d}:{index * 7 % 60:02d}:{index * 13 % 60:02d}z",
                f"byte count value: {index + 1} KiB",
                f"time delta value: {index + 1} weeks",
                f"bytes value: <01 23 45 67 89 ab cd ef {index:02x}>",
                f"regex value: /^sensor-{index:03d}-[a-z]+$/",
                f'mixed list: {index}, {index + 1}.25, enabled, "sample {index:03d}", '
                f"2026-06-{index % 27 + 1:02d}, <de ad be ef>",
                "",
            ]
        )
    return "\n".join(lines)


def hierarchy_document() -> str:
    lines = ['@version: "1.0"', "# Nested sections, text names and section lists.", ""]
    for index in range(100):
        lines.extend(
            [
                f"[environment_{index:03d}.service_{index:03d}.localized_labels]",
                f'name: "Service {index:03d}"',
                f"port: {20000 + index}",
                f'hosts: "node-{index:03d}-a", "node-{index:03d}-b", "node-{index:03d}-c"',
                f'[text."Localized Labels {index:03d}"]',
                f'"English": "Processing station {index:03d}"',
                f'"日本語": "処理ステーション {index:03d}"',
                f'"Emoji 😀": "status {index:03d} ✅"',
                "",
            ]
        )
    for index in range(120):
        lines.extend(
            [
                "*[worker]",
                f"identifier: {index}",
                f'label: "Worker {index:03d}"',
                f'capabilities: "parse", "index", "report-{index % 9}"',
                f"limits: {index + 1}, {index + 2}, {index + 3}, {index + 4}",
                "",
            ]
        )
    return "\n".join(lines)


def multiline_document() -> str:
    lines = [
        '@version: "1.0"',
        "# Multiline text, code, regular expressions and byte data.",
        "",
    ]
    for index in range(55):
        lines.extend(
            [
                f"[multiline batch {index:03d}]",
                'description: """',
                f"    Observation batch {index:03d} contains a deliberately substantial text payload.",
                "    It covers indentation, Unicode symbols Δ λ 中, punctuation, and repeated parser lines.",
                "    The quick brown fox jumps over the lazy dog while the telemetry clock advances.",
                '    """',
                "program: ```python",
                f"    batch = {index}",
                "    for sensor in range(8):",
                '        print(f"{batch}:{sensor}:ready")',
                "    ```",
                "pattern: ///",
                f"    ^batch-{index:03d}-\\w+\\.[Ee][Ll][Cc][Ll]$",
                "    ///",
                "payload: <<<hex",
                "    00 11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff",
                f"    {index:02x} {index + 1:02x} {index + 2:02x} {index + 3:02x} "
                f"{index + 4:02x} {index + 5:02x} {index + 6:02x} {index + 7:02x}",
                "    >>>",
                "",
            ]
        )
    return "\n".join(lines)


def application_document() -> str:
    lines = ['@version: "1.0"', "# Realistic mixed service configuration.", ""]
    for index in range(100):
        lines.extend(
            [
                f"[service_{index:03d}.database.observability]",
                f'enabled: {"yes" if index % 3 else "no"}',
                f'display name: "Ingestion Service {index:03d} – Zürich"',
                f'endpoint: "https://service-{index:03d}.example.invalid/api/v1/events"',
                f"retry delays: 1, 2, 5, 10, 30, 60",
                f"started: 2026-{index % 12 + 1:02d}-{index % 27 + 1:02d} "
                f"{index % 24:02d}:{index * 11 % 60:02d}:00z",
                f"storage limit: {index + 64} MiB",
                f'host: "db-{index % 12:02d}.internal.example"',
                f"port: {5400 + index % 20}",
                f"pool size: {8 + index % 32}",
                f"statement timeout: {index % 20 + 1} seconds",
                f"sample rate: 0.{index % 90 + 10}",
                f'labels: "region-{index % 7}", "tier-{index % 4}", "owner-{index % 13}"',
                f"health pattern: /^service-{index:03d}:(ready|degraded)$/",
                "",
            ]
        )
    return "\n".join(lines)


def documents() -> list[tuple[str, str]]:
    return [
        ("scalar-values.elcl", pretty_format(scalar_document())),
        ("hierarchy.elcl", pretty_format(hierarchy_document())),
        ("multiline.elcl", pretty_format(multiline_document())),
        ("application.elcl", pretty_format(application_document())),
    ]


def embedded_text(text: str) -> str:
    """Add the final empty line used by the original embedded corpus."""
    return f"{text}\n"


def cpp_literal_lines(text: str) -> list[str]:
    """Encode one C++ string-literal token per source line."""
    return [f"    {json.dumps(line, ensure_ascii=False)}" for line in embedded_text(text).splitlines(keepends=True)]


def generated_cpp(corpus: list[tuple[str, str]]) -> str:
    lines = [
        "// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev",
        "// SPDX-License-Identifier: Apache-2.0",
        "//",
        "// This file is generated by test/profiling/conf/tools/generate_corpus.py",
        '#include "EmbeddedDocuments.hpp"',
        "",
        "#include <erbsland/text/Literals.hpp>",
        "",
        "namespace app::conf {",
        "",
        "namespace el = erbsland;",
        "",
        "using namespace el::text::literals;",
        "",
        "// clang-format off",
    ]
    for index, (_, text) in enumerate(corpus):
        lines.append(f"static constexpr auto cDocument{index} =")
        lines.extend(cpp_literal_lines(text))
        lines[-1] += "_el;"
        lines.append("")
    lines.extend(
        [
            "auto embeddedDocuments() -> const std::array<EmbeddedDocument, 4> & {",
            "    static const auto documents = std::array<EmbeddedDocument, 4>{{",
        ]
    )
    for index, (name, _) in enumerate(corpus):
        lines.append(f'        {{"{name}"_el, cDocument{index}}},')
    lines.extend(["    }};", "    return documents;", "}", "// clang-format on", "", "}", ""])
    return "\n".join(lines)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Verify that all generated files are up to date.")
    return parser.parse_args()


def update_file(path: Path, content: str, check: bool) -> None:
    expected = content if content.endswith("\n") else f"{content}\n"
    if check:
        if not path.exists() or path.read_text(encoding="utf-8") != expected:
            raise SystemExit(f"Generated file is out of date: {path}")
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(expected, encoding="utf-8")


def main() -> None:
    args = parse_arguments()
    corpus = documents()
    for name, text in corpus:
        update_file(DATA_DIR / name, text, args.check)
    update_file(OUTPUT_PATH, generated_cpp(corpus), args.check)


if __name__ == "__main__":
    main()
