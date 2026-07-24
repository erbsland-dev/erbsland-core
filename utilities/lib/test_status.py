# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Validation and normalization for API test-status documentation markers."""

from __future__ import annotations

import re
from dataclasses import dataclass


MARKER_NAMES = ("tested", "notest", "needtest")
MARKER_START = re.compile(r"^(?P<indent>\s*)///\s*@(?P<name>tested|notest|needtest)(?P<value>.*)$")
COMMENT_LINE = re.compile(r"^\s*///\s?(?P<text>.*)$")
TESTED_VALUE = re.compile(r"^[A-Za-z_]\w*Test(?: [A-Za-z_]\w*Test)*$")
TYPE_DECLARATION = re.compile(r"^(?:template\b|requires\b|class\b|struct\b|using\b)")
FREE_FUNCTION_DECLARATION = re.compile(
    r"^(?:(?:\[\[[^\]]+\]\]|inline|constexpr)\s+)*(?:auto|void)\b"
)


@dataclass(frozen=True)
class TestStatusIssue:
    """One invalid test-status marker."""

    line_number: int
    message: str


def normalize_test_status_text(text: str) -> str:
    """Normalize marker values and remove markers on member declarations."""
    lines = text.splitlines()
    result: list[str] = []
    index = 0
    while index < len(lines):
        match = MARKER_START.match(lines[index])
        if match is None:
            result.append(lines[index])
            index += 1
            continue

        name = match.group("name")
        value, end_index = marker_value(lines, index, match.group("value"))
        if not marker_is_allowed(lines, index, match.group("indent")):
            index = end_index + 1
            continue
        if name == "tested":
            value = normalize_tested_value(value)
            result.append(f"{match.group('indent')}/// @tested{{{value}}}")
        else:
            result.append(f"{match.group('indent')}/// @{name}{{{value.strip()}}}")
        index = end_index + 1
    return "\n".join(result) + ("\n" if text.endswith("\n") else "")


def validate_test_status_text(text: str) -> list[TestStatusIssue]:
    """Return all test-status marker issues in one C++ header."""
    lines = text.splitlines()
    result = []
    index = 0
    while index < len(lines):
        match = MARKER_START.match(lines[index])
        if match is None:
            index += 1
            continue
        name = match.group("name")
        value, end_index = marker_value(lines, index, match.group("value"))
        line_number = index + 1
        if end_index != index:
            result.append(TestStatusIssue(line_number, "test-status markers must be single-line."))
        if not marker_is_allowed(lines, index, match.group("indent")):
            result.append(TestStatusIssue(line_number, "test-status markers are only allowed on types and free functions."))
        if name == "tested":
            if not TESTED_VALUE.fullmatch(value):
                result.append(TestStatusIssue(line_number, "@tested must contain space-separated test suite names only."))
        elif not value.strip() or not match.group("value").strip().startswith("{"):
            result.append(TestStatusIssue(line_number, f"@{name} requires a non-empty braced reason."))
        index = end_index + 1
    return result


def marker_value(lines: list[str], index: int, value: str) -> tuple[str, int]:
    """Read the braced marker value, including malformed multiline values."""
    value = value.strip()
    end_index = index
    while "}" not in value and end_index + 1 < len(lines):
        next_match = COMMENT_LINE.match(lines[end_index + 1])
        if next_match is None:
            break
        end_index += 1
        value += " " + next_match.group("text").strip()
    if value.startswith("{") and "}" in value:
        return value[1 : value.index("}")], end_index
    return value, end_index


def normalize_tested_value(value: str) -> str:
    """Convert legacy separators and method pointers into suite names."""
    suites = []
    for part in re.split(r"[\s,]+", value.replace("`", "").strip()):
        if not part:
            continue
        suite = part.split("::", maxsplit=1)[0]
        if suite not in suites:
            suites.append(suite)
    return " ".join(suites)


def marker_is_allowed(lines: list[str], index: int, indent: str) -> bool:
    """Test whether the following declaration is a type or a free function."""
    declaration = next_declaration(lines, index + 1)
    if declaration is None:
        return False
    if TYPE_DECLARATION.match(declaration):
        return True
    return not indent and bool(FREE_FUNCTION_DECLARATION.match(declaration))


def next_declaration(lines: list[str], index: int) -> str | None:
    """Get the first declaration line after a documentation block."""
    for line in lines[index:]:
        stripped = line.strip()
        if not stripped or stripped.startswith("///"):
            continue
        return stripped
    return None
