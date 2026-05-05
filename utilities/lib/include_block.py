# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import re
from dataclasses import dataclass


@dataclass(frozen=True)
class IncludeBlock:
    """A contiguous include block found at the top of a C++ source file."""

    text: str
    start: int
    end: int


@dataclass(frozen=True)
class IncludeLine:
    """A parsed C++ include line."""

    prefix: str
    path: str
    is_global: bool
    suffix: str

    def render(self, *, path: str | None = None, is_global: bool | None = None) -> str:
        """Render the line with optional path and quote style replacements."""
        include_path = self.path if path is None else path
        use_global = self.is_global if is_global is None else is_global
        start_quote, end_quote = ("<", ">") if use_global else ('"', '"')
        return f"{self.prefix}{start_quote}{include_path}{end_quote}{self.suffix}"


class IncludeBlockScanner:
    """Scanner for the convention-managed leading C++ include block."""

    RE_LEADING_BLOCK_H = re.compile(
        r"""
        ^
        (?P<bom>\ufeff?)
        (?P<copyright>(?://.*?\n)+)
        (?:[ \t]*\n)*
        (?P<directive>\#pragma[ \t]+once[ \t]*\n)
    """,
        re.VERBOSE,
    )

    RE_LEADING_BLOCK_CPP = re.compile(
        r"""
        ^
        (?P<bom>\ufeff?)
        (?P<copyright>(?://.*?\n)+)
        (?:[ \t]*\n)*
        (?P<directive>\#include.+\n)
    """,
        re.VERBOSE,
    )

    RE_INCLUDE_BLOCK_H = re.compile(
        r"""
        \ufeff?
        (?://.*?\n)+(?:[ \t]*\n)*
        \#pragma[ \t]+once[ \t]*\n
        ((?:[ \t]*\n)*
        (?:\#include.+\n|[ \t]*\n)+)
    """,
        re.VERBOSE,
    )

    RE_INCLUDE_BLOCK_CPP = re.compile(
        r"""
        \ufeff?
        (?://.*?\n)+(?:[ \t]*\n)*
        \#include.+\n
        ((?:[ \t]*\n)*
        (?:\#include.+\n|[ \t]*\n)+)
    """,
        re.VERBOSE,
    )

    RE_FULL_INCLUDE_BLOCK_CPP = re.compile(
        r"""
        \ufeff?
        (?://.*?\n)+(?:[ \t]*\n)*
        ((?:\#include.+\n|[ \t]*\n)+)
    """,
        re.VERBOSE,
    )

    RE_INCLUDE_LINE = re.compile(
        r"""
        ^
        (?P<prefix>[ \t]*\#include[ \t]+)
        (?P<start_quote><|")
        (?P<path>[^>"]+)
        (?P<end_quote>>|")
        (?P<suffix>[^\n]*(?:\n)?)
        $
    """,
        re.VERBOSE,
    )

    @classmethod
    def normalize_leading_spacing(cls, text: str, is_header: bool) -> str:
        """Normalize the spacing between the copyright block and the first directive."""
        pattern = cls.RE_LEADING_BLOCK_H if is_header else cls.RE_LEADING_BLOCK_CPP
        match = pattern.match(text)
        if not match:
            return text
        return (
            f'{match.group("bom")}' f'{match.group("copyright")}' f'{match.group("directive")}' f"{text[match.end():]}"
        )

    @classmethod
    def find_include_block(cls, text: str, is_header: bool, *, include_first_cpp: bool = False) -> IncludeBlock | None:
        """Find the include block that is managed by the development utilities."""
        if is_header:
            if "#pragma once" not in text:
                raise ValueError('Missing "#pragma once" in header file!')
            match = cls.RE_INCLUDE_BLOCK_H.match(text)
        else:
            pattern = cls.RE_FULL_INCLUDE_BLOCK_CPP if include_first_cpp else cls.RE_INCLUDE_BLOCK_CPP
            match = pattern.match(text)
        if not match or not match.group(1).strip():
            return None
        return IncludeBlock(match.group(1), match.start(1), match.end(1))

    @classmethod
    def parse_include_line(cls, line: str) -> IncludeLine | None:
        """Parse one include line, preserving surrounding text."""
        match = cls.RE_INCLUDE_LINE.match(line)
        if not match:
            return None
        start_quote = match.group("start_quote")
        end_quote = match.group("end_quote")
        if (start_quote == "<" and end_quote != ">") or (start_quote == '"' and end_quote != '"'):
            return None
        return IncludeLine(
            prefix=match.group("prefix"),
            path=match.group("path"),
            is_global=start_quote == "<",
            suffix=match.group("suffix"),
        )
