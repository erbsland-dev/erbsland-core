# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Structural validation for domain-specific API guideline pages."""

from __future__ import annotations

import re
from dataclasses import dataclass

MAX_CORE_SEMANTICS_LINES = 60
MAX_CORE_SEMANTICS_SUBSECTIONS = 4
MAX_CODE_LINE_LENGTH = 120
MAX_PAGE_LINES = 500

PLACEHOLDER = r"❮[A-Za-z_]\w*❯"
PATTERN_IDENTIFIER = rf"(?:[A-Za-z_]|{PLACEHOLDER})(?:[A-Za-z0-9_]|{PLACEHOLDER})*"
FREE_IDENTIFIER = rf"(?:[a-z_]|{PLACEHOLDER})(?:[A-Za-z0-9_]|{PLACEHOLDER})*"
TYPE_COMPONENT = rf"(?:[A-Z]|{PLACEHOLDER})(?:[A-Za-z0-9_]|{PLACEHOLDER})*"
TYPE_NAME = rf"(?:{PATTERN_IDENTIFIER}::)*{TYPE_COMPONENT}"
SHORTCUT = r"[A-Z][A-Za-z]?"
SHORTCUT_LIST = re.compile(rf"{SHORTCUT}(?:, {SHORTCUT})*")
RETURN_BASE = rf"(?:{PATTERN_IDENTIFIER}::)*{PATTERN_IDENTIFIER}"


@dataclass(frozen=True)
class ApiGuidelineIssue:
    """One invalid API guideline construct."""

    line_number: int
    message: str


@dataclass(frozen=True)
class Section:
    """One top-level section in a guideline page."""

    title: str
    title_line: int
    content_start: int
    content_end: int


def validate_api_guideline_text(text: str) -> list[ApiGuidelineIssue]:
    """Validate one domain-specific API guideline page."""
    lines = text.splitlines()
    if text.endswith("\n"):
        lines.append("")
    issues: list[ApiGuidelineIssue] = []
    if len(lines) - int(bool(text.endswith("\n"))) > MAX_PAGE_LINES:
        issues.append(ApiGuidelineIssue(1, f"page must not exceed {MAX_PAGE_LINES} lines."))
    validate_page_title(lines, issues)
    sections = find_top_level_sections(lines)
    validate_section_structure(lines, sections, issues)
    for section in sections:
        if section.title == "Core Semantics":
            validate_core_semantics(lines, section, issues)
        elif section.title.endswith(" Types"):
            validate_code_section(lines, section, "type", issues)
        elif section.title == "Pattern Definitions":
            validate_code_section(lines, section, "definition", issues)
        elif section.title.endswith(" Patterns"):
            validate_code_section(lines, section, "pattern", issues)
    return sorted(issues, key=lambda issue: (issue.line_number, issue.message))


def validate_page_title(lines: list[str], issues: list[ApiGuidelineIssue]) -> None:
    """Validate the overlined page title."""
    if len(lines) < 3:
        issues.append(ApiGuidelineIssue(1, "page must start with an overlined title."))
        return
    title = lines[1]
    expected = "*" * len(title)
    if not title or lines[0] != expected or lines[2] != expected:
        issues.append(ApiGuidelineIssue(1, "page title must use matching '*' lines above and below the title."))
    if len(lines) >= 4 and lines[3]:
        issues.append(ApiGuidelineIssue(4, "page title must be followed by one blank line."))


def find_top_level_sections(lines: list[str]) -> list[Section]:
    """Find all sections using the equals-sign adornment."""
    headings: list[tuple[int, str]] = []
    for index in range(len(lines) - 1):
        title = lines[index]
        underline = lines[index + 1]
        if title and underline == "=" * len(title):
            headings.append((index, title))
    result = []
    for heading_index, (line_index, title) in enumerate(headings):
        content_end = headings[heading_index + 1][0] if heading_index + 1 < len(headings) else len(lines)
        result.append(Section(title, line_index + 1, line_index + 2, content_end))
    return result


def validate_section_structure(lines: list[str], sections: list[Section], issues: list[ApiGuidelineIssue]) -> None:
    """Validate allowed top-level sections and their order."""
    if not sections:
        issues.append(ApiGuidelineIssue(1, "page must contain at least one top-level section."))
        return

    first_section_index = sections[0].title_line - 1
    for index in range(3, first_section_index):
        if lines[index]:
            issues.append(
                ApiGuidelineIssue(index + 1, "content outside Core Semantics and structural sections is not allowed.")
            )
            break

    phase = "start"
    core_count = 0
    definition_count = 0
    type_sections: list[Section] = []
    for section in sections:
        title = section.title
        if title == "Core Semantics":
            core_count += 1
            if phase != "start":
                issues.append(ApiGuidelineIssue(section.title_line, "Core Semantics must precede all type sections."))
            phase = "core"
        elif title.endswith(" Types"):
            if phase in {"definitions", "patterns"}:
                issues.append(ApiGuidelineIssue(section.title_line, "type sections must precede pattern sections."))
            phase = "types"
            type_sections.append(section)
        elif title == "Pattern Definitions":
            definition_count += 1
            if phase not in {"types"}:
                issues.append(
                    ApiGuidelineIssue(section.title_line, "Pattern Definitions must follow all type sections.")
                )
            phase = "definitions"
        elif title.endswith(" Patterns"):
            if phase not in {"types", "definitions", "patterns"}:
                issues.append(ApiGuidelineIssue(section.title_line, "pattern sections must follow all type sections."))
            phase = "patterns"
        else:
            issues.append(
                ApiGuidelineIssue(
                    section.title_line,
                    "invalid section; expected Core Semantics, '... Types', Pattern Definitions, or '... Patterns'.",
                )
            )

    if core_count > 1:
        issues.append(ApiGuidelineIssue(1, "at most one Core Semantics section is allowed."))
    if definition_count > 1:
        issues.append(ApiGuidelineIssue(1, "at most one Pattern Definitions section is allowed."))
    if not type_sections:
        issues.append(ApiGuidelineIssue(1, "at least one type section is required."))
        return
    if type_sections[0].title != "Primary Types":
        issues.append(ApiGuidelineIssue(type_sections[0].title_line, "the first type section must be Primary Types."))
    if len(type_sections) == 2 and type_sections[1].title != "Secondary Types":
        issues.append(
            ApiGuidelineIssue(
                type_sections[1].title_line, "with two type sections, the second must be Secondary Types."
            )
        )

    first_section_line = sections[0].title_line
    for index in range(3, first_section_line - 1):
        if is_heading(lines, index, "-"):
            issues.append(ApiGuidelineIssue(index + 1, "subsections are only allowed inside Core Semantics."))
    for section in sections:
        if section.title == "Core Semantics":
            continue
        for index in range(section.content_start, section.content_end - 1):
            if is_heading(lines, index, "-"):
                issues.append(ApiGuidelineIssue(index + 1, "subsections are only allowed inside Core Semantics."))


def validate_core_semantics(lines: list[str], section: Section, issues: list[ApiGuidelineIssue]) -> None:
    """Validate size and subsection limits of Core Semantics."""
    content = trim_blank_lines(lines[section.content_start : section.content_end])
    if len(content) > MAX_CORE_SEMANTICS_LINES:
        issues.append(
            ApiGuidelineIssue(
                section.title_line,
                f"Core Semantics must not exceed {MAX_CORE_SEMANTICS_LINES} lines.",
            )
        )
    subsection_lines = [
        index for index in range(section.content_start, section.content_end - 1) if is_heading(lines, index, "-")
    ]
    if len(subsection_lines) > MAX_CORE_SEMANTICS_SUBSECTIONS:
        issues.append(
            ApiGuidelineIssue(
                subsection_lines[MAX_CORE_SEMANTICS_SUBSECTIONS] + 1,
                f"Core Semantics allows at most {MAX_CORE_SEMANTICS_SUBSECTIONS} subsections.",
            )
        )


def validate_code_section(
    lines: list[str],
    section: Section,
    line_kind: str,
    issues: list[ApiGuidelineIssue],
) -> None:
    """Validate a types, definitions, or patterns code-block section."""
    content = lines[section.content_start : section.content_end]
    directive_lines = [offset for offset, line in enumerate(content) if line.lstrip().startswith(".. code-block::")]
    if len(directive_lines) != 1:
        issues.append(
            ApiGuidelineIssue(
                section.title_line,
                "section must contain exactly one '.. code-block:: text' directive.",
            )
        )
        return
    if (
        len(content) < 5
        or content[0] != ""
        or content[1] != ".. code-block:: text"
        or content[2] != ""
        or content[-1] != ""
    ):
        issues.append(
            ApiGuidelineIssue(
                section.title_line,
                "section must contain only one correctly spaced '.. code-block:: text' block.",
            )
        )
    directive_offset = directive_lines[0]
    for offset, line in enumerate(content[directive_offset + 1 :], start=directive_offset + 1):
        line_number = section.content_start + offset + 1
        if not line:
            continue
        if not line.startswith("    ") or line.startswith("     ") or "\t" in line[:4]:
            issues.append(
                ApiGuidelineIssue(line_number, "code-block lines must use exactly four spaces of indentation.")
            )
            continue
        validate_code_line(line[4:], len(line), line_number, line_kind, issues)


def validate_code_line(
    line: str,
    source_length: int,
    line_number: int,
    line_kind: str,
    issues: list[ApiGuidelineIssue],
) -> None:
    """Validate one logical line in a code block."""
    if source_length > MAX_CODE_LINE_LENGTH:
        issues.append(
            ApiGuidelineIssue(line_number, f"code-block lines must not exceed {MAX_CODE_LINE_LENGTH} characters.")
        )
    match = re.fullmatch(r"(?P<expression>.+?) +// (?P<description>\S.*)", line)
    if match is None:
        issues.append(ApiGuidelineIssue(line_number, "line must contain ' // ' followed by a description."))
        return
    expression = match.group("expression")
    description = match.group("description")
    if description.endswith("."):
        issues.append(ApiGuidelineIssue(line_number, "description must not end with a period."))
    if line_kind == "type":
        if not is_type_list(expression, allow_slash=False):
            issues.append(
                ApiGuidelineIssue(
                    line_number,
                    "type entry must contain comma-separated type names or type patterns.",
                )
            )
    elif line_kind == "definition":
        validate_pattern_definition(expression, line_number, issues)
    else:
        validate_pattern(expression, line_number, issues)


def validate_pattern_definition(expression: str, line_number: int, issues: list[ApiGuidelineIssue]) -> None:
    """Validate one pattern shortcut definition."""
    if expression.count(" = ") != 1:
        issues.append(ApiGuidelineIssue(line_number, "pattern definition must contain exactly one ' = '."))
        return
    shortcuts, type_patterns = expression.split(" = ")
    if SHORTCUT_LIST.fullmatch(shortcuts) is None:
        issues.append(
            ApiGuidelineIssue(
                line_number,
                "pattern shortcuts must be comma-separated one- or two-character names starting uppercase.",
            )
        )
    if not is_type_list(type_patterns, allow_slash=True):
        issues.append(
            ApiGuidelineIssue(
                line_number,
                "pattern definition value must contain type patterns separated by '/' or ', '.",
            )
        )


def validate_pattern(expression: str, line_number: int, issues: list[ApiGuidelineIssue]) -> None:
    """Validate one API pattern."""
    if expression.count(" -> ") > 1:
        issues.append(ApiGuidelineIssue(line_number, "pattern allows at most one return type."))
        return
    if " -> " in expression:
        call, return_type = expression.split(" -> ")
        if not is_return_type(return_type):
            issues.append(ApiGuidelineIssue(line_number, "invalid return type in pattern."))
    else:
        call = expression
    if not is_call_pattern(call):
        issues.append(
            ApiGuidelineIssue(
                line_number,
                "pattern must be a T constructor, T:: static method, o. object method, or free function.",
            )
        )


def is_return_type(value: str) -> bool:
    """Test whether a value describes exactly one return type."""
    if not value or "/" in value or " -> " in value:
        return False
    if value.startswith("[") and value.endswith("]"):
        value = value[1:-1]
    value = value.removeprefix("const ")
    if value.endswith("&&"):
        value = value[:-2]
    elif value.endswith(("&", "*")):
        value = value[:-1]
    if re.fullmatch(RETURN_BASE, value):
        return True
    template_start = value.find("<")
    if template_start <= 0 or not value.endswith(">"):
        return False
    if re.fullmatch(RETURN_BASE, value[:template_start]) is None:
        return False
    return valid_template_arguments(value[template_start:])


def is_type_list(value: str, allow_slash: bool) -> bool:
    """Test a comma-separated type list, optionally also allowing slash alternatives."""
    separators = {","}
    if allow_slash:
        separators.add("/")
    parts = split_top_level(value, separators)
    if not parts:
        return False
    return all(re.fullmatch(TYPE_NAME, part.strip()) is not None for part in parts)


def is_call_pattern(value: str) -> bool:
    """Test the allowed constructor, method, and free-function pattern forms."""
    if not value.endswith(")") or not balanced_parentheses(value):
        return False
    if value.startswith("T("):
        return value.find("(") == 1
    if value.startswith("T::"):
        remainder = value[3:]
        identifier_pattern = PATTERN_IDENTIFIER
    elif value.startswith("o."):
        remainder = value[2:]
        identifier_pattern = PATTERN_IDENTIFIER
    else:
        remainder = value
        identifier_pattern = FREE_IDENTIFIER
    alternatives = split_call_alternatives(remainder)
    if not alternatives or "(" not in alternatives[-1]:
        return False
    call_pattern = rf"{identifier_pattern}(?:<[^<>\n]+>)?\(.*\)"
    return all(
        (
            re.fullmatch(call_pattern, alternative) is not None
            if "(" in alternative
            else re.fullmatch(identifier_pattern, alternative) is not None
        )
        for alternative in alternatives
    )


def split_call_alternatives(value: str) -> list[str]:
    """Split slash-separated call alternatives outside parameter lists."""
    result = []
    start = 0
    depth = 0
    for index, character in enumerate(value):
        if character == "(":
            depth += 1
        elif character == ")":
            depth -= 1
        elif character == "/" and depth == 0:
            result.append(value[start:index])
            start = index + 1
    result.append(value[start:])
    return result


def balanced_parentheses(value: str) -> bool:
    """Test balanced call parentheses while allowing optional-parameter brackets."""
    depth = 0
    bracket_depth = 0
    for character in value:
        if character == "(":
            depth += 1
        elif character == ")":
            depth -= 1
            if depth < 0:
                return False
        elif character == "[":
            bracket_depth += 1
        elif character == "]":
            bracket_depth -= 1
            if bracket_depth < 0:
                return False
        elif character.isspace() and depth == 0:
            return False
    return depth == 0 and bracket_depth == 0


def split_top_level(value: str, separators: set[str]) -> list[str]:
    """Split at selected separators outside angle brackets."""
    result = []
    start = 0
    depth = 0
    for index, character in enumerate(value):
        if character == "<":
            depth += 1
        elif character == ">":
            depth -= 1
            if depth < 0:
                return []
        elif depth == 0 and character in separators:
            result.append(value[start:index])
            start = index + 1
    if depth != 0:
        return []
    result.append(value[start:])
    return result


def balanced_template_arguments(value: str) -> bool:
    """Test balanced non-empty angle-bracket template arguments."""
    depth = 0
    for index, character in enumerate(value):
        if character == "<":
            depth += 1
        elif character == ">":
            depth -= 1
            if depth < 0 or (depth == 0 and index != len(value) - 1):
                return False
        elif depth == 0:
            return False
    return depth == 0 and value != "<>"


def valid_template_arguments(value: str) -> bool:
    """Test a template argument list containing one or more types."""
    if not balanced_template_arguments(value):
        return False
    arguments = split_top_level(value[1:-1], {","})
    return bool(arguments) and all(is_return_type(argument.strip()) for argument in arguments)


def trim_blank_lines(lines: list[str]) -> list[str]:
    """Remove leading and trailing blank lines."""
    start = 0
    end = len(lines)
    while start < end and not lines[start]:
        start += 1
    while end > start and not lines[end - 1]:
        end -= 1
    return lines[start:end]


def is_heading(lines: list[str], index: int, adornment: str) -> bool:
    """Test for one underlined heading at an index."""
    return 0 <= index < len(lines) - 1 and bool(lines[index]) and lines[index + 1] == adornment * len(lines[index])
