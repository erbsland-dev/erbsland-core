# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

from lib.error import UtilityError
from lib.path_safety import read_safe_text


@dataclass(frozen=True)
class CodePointRange:
    """A closed Unicode code-point range."""

    begin: int
    end: int

    def __post_init__(self) -> None:
        if self.begin < 0 or self.end < self.begin:
            raise UtilityError(f"Invalid Unicode code-point range: {self.begin:04X}..{self.end:04X}")


VERSION_PATTERN = re.compile(r"Version (\d+)\.(\d+)\.(\d+)")


def parse_code_point_range(text: str) -> CodePointRange:
    """Parse a UCD code-point field."""
    field = text.strip()
    if ".." in field:
        begin_text, end_text = field.split("..", maxsplit=1)
        return CodePointRange(int(begin_text, 16), int(end_text, 16))
    value = int(field, 16)
    return CodePointRange(value, value)


def iter_ucd_data_lines(path: Path, label: str) -> list[str]:
    """Read a UCD file and return non-empty data lines without comments."""
    text = read_safe_text(path, label)
    result = []
    for raw_line in text.splitlines():
        line = raw_line.split("#", maxsplit=1)[0].strip()
        if line:
            result.append(line)
    return result


def parse_ucd_version(readme_path: Path) -> tuple[int, int, int]:
    """Parse the Unicode version from a UCD ReadMe.txt file."""
    text = read_safe_text(readme_path, "Unicode ReadMe.txt")
    match = VERSION_PATTERN.search(text)
    if match is None:
        raise UtilityError(f"Failed to parse the Unicode version from {readme_path}.")
    return tuple(int(value) for value in match.groups())
