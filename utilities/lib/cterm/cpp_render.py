# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Helpers for rendering generated cterm C++ data files."""

from __future__ import annotations

from collections.abc import Iterable

from lib.copyright import HeaderConfig, HeaderKind


def escape_cpp_string(text: str) -> str:
    """Escape a string for a C++ string literal."""
    return text.replace("\\", "\\\\").replace('"', '\\"')


def cpp_u32_string_literal(text: str, *, chunk_size: int = 96) -> list[str]:
    """Render a UTF-32 C++ string literal as one or more source lines."""
    if not text:
        return ['U""']
    return [f'U"{escape_cpp_string(text[start:start + chunk_size])}"' for start in range(0, len(text), chunk_size)]


def cpp_byte_string_literal(byte_values: Iterable[int], *, chunk_size: int = 64) -> list[str]:
    """Render byte values as one or more C++ string literal source lines."""
    values = list(byte_values)
    if not values:
        return ['""']
    return [
        '"' + "".join(f"\\x{value:02X}" for value in values[start : start + chunk_size]) + '"'
        for start in range(0, len(values), chunk_size)
    ]


def render_integer_array(
    values: Iterable[int],
    *,
    indent: str = "        ",
    entries_per_line: int = 12,
    suffix: str = "U",
    width: int = 2,
) -> list[str]:
    """Render an integer list for a C++ array initializer."""
    value_list = list(values)
    return [
        indent
        + ", ".join(f"0x{value:0{width}X}{suffix}" for value in value_list[start : start + entries_per_line])
        + ","
        for start in range(0, len(value_list), entries_per_line)
    ]


def render_data_function(
    name: str,
    result_type: str,
    values: Iterable[int],
    *,
    entries_per_line: int = 12,
    width: int = 2,
) -> list[str]:
    """Render one generated data accessor function."""
    value_list = list(values)
    storage_type = result_type.split("::")[-1]
    lines = [
        f"auto CommonBoxFrameCombinationStyle::{name}() noexcept -> std::span<const {result_type}> {{",
        f"    static constexpr std::array<{storage_type}, {len(value_list)}U> data{{{{",
    ]
    lines.extend(render_integer_array(value_list, entries_per_line=entries_per_line, width=width))
    lines.extend(
        [
            "    }};",
            "    return data;",
            "}",
            "",
        ]
    )
    return lines


def generated_cpp_file(
    header_config: HeaderConfig,
    kind: HeaderKind,
    tool: str,
    summary_lines: Iterable[str],
    body_lines: Iterable[str],
) -> str:
    """Create a complete generated C++ file with shared project headers."""
    lines = [
        header_config.source_header(kind, tool=tool, pragma_once=kind == "hpp"),
        "// clang-format off",
    ]
    summary_list = list(summary_lines)
    if summary_list:
        lines.append("//")
        lines.extend(f"// {line}" for line in summary_list)
    lines.extend(body_lines)
    lines.extend(["// clang-format on", ""])
    return "\n".join(lines)
