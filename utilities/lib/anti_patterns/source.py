# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import bisect
import re
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Comment:
    """One C++ comment with preserved source coordinates."""

    start: int
    end: int
    start_line: int
    end_line: int
    text: str
    line_comment: bool


@dataclass(frozen=True)
class StringLiteral:
    """One C++ string literal outside preprocessing directives."""

    start: int
    end: int
    prefix: str
    suffix: str


@dataclass(frozen=True)
class Block:
    """One matched C++ brace block."""

    start: int
    end: int
    header_start: int
    kind: str
    name: str = ""
    parent_index: int | None = None


class SourceFile:
    """Lexical representation of one C++ source file."""

    _string_start = re.compile(r'(?P<prefix>u8|u|U|L)?(?P<raw>R)?"')
    _character_start = re.compile(r"(?:u8|u|U|L)?'")
    _suffix = re.compile(r"[A-Za-z_]\w*")
    _suppression = re.compile(
        r"^\s*anti-pattern:\s*allow\s+(?P<rule>[a-z][a-z0-9_]*)\s+--\s+(?P<reason>\S(?:.*\S)?)\s*$"
    )
    _conditional_directive = re.compile(r"^\s*#\s*(?P<directive>if|ifdef|ifndef|endif)\b(?P<condition>.*)$")
    _os_macro = re.compile(r"\bERBSLAND_OS(?:_[A-Z0-9_]+)?\b")

    def __init__(self, path: Path, text: str) -> None:
        self.path = path
        self.text = text
        self.lines = tuple(text.splitlines())
        self._line_starts = self._create_line_starts(text)
        self._preprocessor_lines = self._find_preprocessor_lines()
        self._os_conditional_ranges = self._find_os_conditional_ranges()
        self.masked_text, self.comments, self.string_literals = self._lex()
        self.blocks = self._find_blocks()

    @staticmethod
    def _create_line_starts(text: str) -> tuple[int, ...]:
        result = [0]
        result.extend(index + 1 for index, character in enumerate(text) if character == "\n")
        return tuple(result)

    def _find_preprocessor_lines(self) -> frozenset[int]:
        result: set[int] = set()
        continued = False
        for line_number, line in enumerate(self.lines, start=1):
            is_directive = continued or line.lstrip().startswith("#")
            if is_directive:
                result.add(line_number)
            continued = is_directive and line.rstrip().endswith("\\")
        return frozenset(result)

    def _find_os_conditional_ranges(self) -> tuple[tuple[int, int], ...]:
        """Find conditional compilation blocks selected with an Erbsland OS macro."""
        stack: list[tuple[int, bool]] = []
        result: list[tuple[int, int]] = []
        line_number = 1
        while line_number <= len(self.lines):
            directive_line = line_number
            logical_line = self.lines[line_number - 1]
            while logical_line.rstrip().endswith("\\") and line_number < len(self.lines):
                logical_line = logical_line.rstrip()[:-1] + self.lines[line_number]
                line_number += 1
            match = self._conditional_directive.match(logical_line)
            if match is None:
                line_number += 1
                continue
            directive = match.group("directive")
            if directive in {"if", "ifdef", "ifndef"}:
                condition = match.group("condition")
                stack.append((self.line_start(directive_line), self._os_macro.search(condition) is not None))
                line_number += 1
                continue
            if stack:
                start, is_os_conditional = stack.pop()
                if is_os_conditional:
                    result.append((start, self.line_start(line_number + 1)))
            line_number += 1
        for start, is_os_conditional in stack:
            if is_os_conditional:
                result.append((start, len(self.text)))
        return tuple(sorted(result))

    def _lex(self) -> tuple[str, tuple[Comment, ...], tuple[StringLiteral, ...]]:
        masked = list(self.text)
        comments: list[Comment] = []
        literals: list[StringLiteral] = []
        index = 0
        line_number = 1
        while index < len(self.text):
            character = self.text[index]
            if character == "\n":
                line_number += 1
                index += 1
                continue
            if line_number in self._preprocessor_lines:
                masked[index] = " "
                index += 1
                continue
            if character == "/" and self.text.startswith("//", index):
                end = self.text.find("\n", index)
                if end < 0:
                    end = len(self.text)
                comments.append(Comment(index, end, line_number, line_number, self.text[index + 2 : end], True))
                self._mask_range(masked, index, end)
                index = end
                continue
            if character == "/" and self.text.startswith("/*", index):
                closing = self.text.find("*/", index + 2)
                end = len(self.text) if closing < 0 else closing + 2
                end_line = line_number + self.text.count("\n", index, end)
                comments.append(Comment(index, end, line_number, end_line, self.text[index + 2 : end - 2], False))
                self._mask_range(masked, index, end)
                line_number = end_line
                index = end
                continue
            if character not in "\"'uULR":
                index += 1
                continue
            string_match = self._string_start.match(self.text, index)
            if string_match is not None and self._literal_prefix_boundary(index):
                prefix = string_match.group("prefix") or ""
                raw = string_match.group("raw") is not None
                literal_end = self._raw_string_end(string_match.end()) if raw else self._quoted_end(string_match.end())
                suffix_match = self._suffix.match(self.text, literal_end)
                end = suffix_match.end() if suffix_match is not None else literal_end
                suffix = suffix_match.group(0) if suffix_match is not None else ""
                literals.append(StringLiteral(index, end, prefix, suffix))
                self._mask_range(masked, index, end)
                line_number += self.text.count("\n", index, end)
                index = end
                continue
            character_match = self._character_start.match(self.text, index)
            if character_match is not None and self._literal_prefix_boundary(index):
                end = self._character_end(character_match.end())
                suffix_match = self._suffix.match(self.text, end)
                if suffix_match is not None:
                    end = suffix_match.end()
                self._mask_range(masked, index, end)
                line_number += self.text.count("\n", index, end)
                index = end
                continue
            index += 1
        return "".join(masked), tuple(comments), tuple(literals)

    @staticmethod
    def _mask_range(masked: list[str], start: int, end: int) -> None:
        for index in range(start, end):
            if masked[index] not in "\r\n":
                masked[index] = " "

    def _literal_prefix_boundary(self, index: int) -> bool:
        return index == 0 or not (self.text[index - 1].isalnum() or self.text[index - 1] == "_")

    def _quoted_end(self, content_start: int) -> int:
        index = content_start
        escaped = False
        while index < len(self.text):
            character = self.text[index]
            if character == '"' and not escaped:
                return index + 1
            if character == "\\" and not escaped:
                escaped = True
            else:
                escaped = False
            index += 1
        return len(self.text)

    def _character_end(self, content_start: int) -> int:
        index = content_start
        escaped = False
        while index < len(self.text):
            character = self.text[index]
            if character == "'" and not escaped:
                return index + 1
            if character == "\\" and not escaped:
                escaped = True
            else:
                escaped = False
            index += 1
        return len(self.text)

    def _raw_string_end(self, delimiter_start: int) -> int:
        opening = self.text.find("(", delimiter_start)
        if opening < 0:
            return len(self.text)
        delimiter = self.text[delimiter_start:opening]
        closing_text = f'){delimiter}"'
        closing = self.text.find(closing_text, opening + 1)
        return len(self.text) if closing < 0 else closing + len(closing_text)

    def _find_blocks(self) -> tuple[Block, ...]:
        temporary: list[dict[str, int | str | None]] = []
        stack: list[int] = []
        nearest_delimiter = -1
        for index, character in enumerate(self.masked_text):
            if character == "{":
                header_start = self._header_start(index, nearest_delimiter)
                kind, name = self._classify_header(self.masked_text[header_start:index])
                parent_index = stack[-1] if stack else None
                temporary.append(
                    {
                        "start": index,
                        "end": len(self.masked_text),
                        "header_start": header_start,
                        "kind": kind,
                        "name": name,
                        "parent_index": parent_index,
                    }
                )
                stack.append(len(temporary) - 1)
                nearest_delimiter = index
            elif character == "}":
                if stack:
                    block_index = stack.pop()
                    temporary[block_index]["end"] = index
                nearest_delimiter = index
            elif character == ";":
                nearest_delimiter = index
        return tuple(Block(**block) for block in temporary)

    def _header_start(self, brace_index: int, nearest_delimiter: int) -> int:
        suffix = self.masked_text[nearest_delimiter + 1 : brace_index]
        if ")" not in suffix and ">" not in suffix:
            return nearest_delimiter + 1
        index = brace_index - 1
        parenthesis_depth = 0
        angle_depth = 0
        while index >= 0:
            character = self.masked_text[index]
            if character == ")":
                parenthesis_depth += 1
            elif character == "(" and parenthesis_depth:
                parenthesis_depth -= 1
            elif character == ">" and parenthesis_depth == 0 and not (
                index > 0 and self.masked_text[index - 1] == "-"
            ):
                angle_depth += 1
            elif character == "<" and parenthesis_depth == 0 and angle_depth:
                angle_depth -= 1
            elif parenthesis_depth == 0 and angle_depth == 0 and character in ";{}":
                return index + 1
            index -= 1
        return 0

    @staticmethod
    def _classify_header(header: str) -> tuple[str, str]:
        if not any(keyword in header for keyword in ("enum", "class", "struct", "namespace")):
            return "other", ""
        compact = " ".join(header.split())
        enum_match = re.search(r"\benum\s+class\s+([A-Za-z_]\w*)[^;{}]*$", compact)
        if enum_match is not None:
            return "enum_class", enum_match.group(1)
        class_match = re.search(r"\b(class|struct)\s+([A-Za-z_]\w*)[^;{}]*$", compact)
        if class_match is not None:
            return class_match.group(1), class_match.group(2)
        namespace_match = re.search(r"\bnamespace(?:\s+(?P<name>[A-Za-z_]\w*(?:::\w+)*))?\s*$", compact)
        if namespace_match is not None:
            return "namespace", namespace_match.group("name") or ""
        return "other", ""

    def line_number(self, offset: int) -> int:
        """Return the one-based line number for a source offset."""
        return bisect.bisect_right(self._line_starts, offset)

    def line_start(self, line_number: int) -> int:
        """Return the source offset for a one-based line number."""
        if line_number <= 0:
            return 0
        if line_number > len(self._line_starts):
            return len(self.text)
        return self._line_starts[line_number - 1]

    def line_end(self, line_number: int) -> int:
        """Return the source offset immediately before a line break."""
        if line_number >= len(self._line_starts):
            return len(self.text)
        return self._line_starts[line_number] - 1

    def enclosing_blocks(self, offset: int) -> tuple[Block, ...]:
        """Return all blocks containing an offset, from outermost to innermost."""
        return tuple(block for block in self.blocks if block.start < offset < block.end)

    def is_namespace_scope(self, offset: int) -> bool:
        """Test whether a location is at global or namespace scope."""
        return all(block.kind == "namespace" for block in self.enclosing_blocks(offset))

    def is_in_os_conditional(self, offset: int) -> bool:
        """Test whether a location is inside a conditional block selected by an Erbsland OS macro."""
        return any(start <= offset < end for start, end in self._os_conditional_ranges)

    def immediate_parent(self, block: Block) -> Block | None:
        """Return the immediate parent of a block."""
        if block.parent_index is None:
            return None
        return self.blocks[block.parent_index]

    def statement_end(self, offset: int) -> int:
        """Find a useful statement/header endpoint for suppression matching."""
        line_end = self.line_end(self.line_number(offset))
        semicolon = self.masked_text.find(";", offset)
        opening = self.masked_text.find("{", offset)
        candidates = [value + 1 for value in (semicolon, opening) if value >= 0]
        if not candidates:
            return line_end
        end = min(candidates)
        if self.line_number(end) > self.line_number(offset) + 80:
            return line_end
        return end

    def has_unclosed_delimiter_before(self, offset: int, opening: str, closing: str) -> bool:
        """Test for an unclosed delimiter before a source location."""
        depth = 0
        for character in self.masked_text[:offset]:
            if character == opening:
                depth += 1
            elif character == closing and depth:
                depth -= 1
        return depth > 0

    def type_declaration_start(self, block: Block) -> int:
        """Return the class/struct/enum keyword offset for a type block."""
        header = self.masked_text[block.header_start : block.start]
        if block.kind == "enum_class":
            match = re.search(r"\benum\s+class\b", header)
        else:
            match = re.search(rf"\b{block.kind}\b", header)
        return block.header_start if match is None else block.header_start + match.start()

    def inline_suppression(self, rule_identifier: str, start: int, end: int) -> str | None:
        """Return a matching adjacent inline-suppression reason."""
        start_line = self.line_number(start)
        previous_line = start_line - 1
        while previous_line >= 1:
            comments = [
                comment for comment in self.comments if comment.line_comment and comment.start_line == previous_line
            ]
            if len(comments) != 1:
                break
            comment = comments[0]
            if self.text[self.line_start(previous_line) : comment.start].strip():
                break
            match = self._suppression.fullmatch(comment.text)
            if match is not None and match.group("rule") == rule_identifier:
                return match.group("reason")
            previous_line -= 1

        end_line = self.line_number(max(start, end - 1))
        for comment in self.comments:
            if not comment.line_comment or comment.start_line != end_line or comment.start < end:
                continue
            match = self._suppression.fullmatch(comment.text)
            if match is not None and match.group("rule") == rule_identifier:
                return match.group("reason")
        return None

    def snippet(self, start: int, end: int, maximum_length: int = 96) -> str:
        """Create a compact single-line source snippet."""
        text = self.text[start:end]
        text = " ".join(text.replace("\x00", "\\0").split())
        if len(text) <= maximum_length:
            return text
        return text[: maximum_length - 1].rstrip() + "…"
