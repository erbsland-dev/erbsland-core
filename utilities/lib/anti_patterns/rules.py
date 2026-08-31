# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import re
from abc import ABC, abstractmethod
from collections.abc import Iterable
from functools import cache
from pathlib import Path

from .model import Candidate, RuleInfo, Severity
from .source import Block, Comment, SourceFile

_standard_library_extension = re.compile(r"\b(?:class|struct)\s+(?P<qualified>std::)?(?:formatter|hash)\s*<")
_defaulted_or_deleted_special_member = re.compile(
    r"(?m)^[ \t]*(?P<declaration>"
    r"(?:(?:\[\[[^\n]*\]\]\s*)|(?:(?:constexpr|consteval|constinit|explicit|inline|static|virtual)\s+))*"
    r"(?:(?P<name>~?[A-Za-z_]\w*)\s*\([^;{}]*\)|"
    r"auto\s+operator\s*=\s*\([^;{}]*\)\s*(?:noexcept\s*)?(?:->[^;={}]+)?)"
    r"\s*(?:noexcept\s*)?(?:(?:override|final)\s*)*(?:requires\s+[^;{}=]+)?=\s*(?:default|delete)\s*;)"
)
_defaults_group_marker = re.compile(r"^\s*defaults(?:/deletions)?\.?\s*$", re.IGNORECASE)


def _is_namespace_type(source: SourceFile, block: Block) -> bool:
    """Test whether a type is declared directly in global or namespace scope."""
    parent = source.immediate_parent(block)
    while parent is not None:
        if parent.kind != "namespace":
            return False
        parent = source.immediate_parent(parent)
    return True


def _is_special_member(source: SourceFile, start: int, name: str | None) -> bool:
    """Test whether a declaration is a constructor, destructor, or assignment operator."""
    blocks = source.enclosing_blocks(start)
    if not blocks or blocks[-1].kind not in {"class", "struct"}:
        return False
    return name is None or name.removeprefix("~") == blocks[-1].name


def _is_declaration_preamble(line: str) -> bool:
    """Test whether a line may separate an API doc block from its declaration."""
    return (
        re.match(r"template\s*<", line) is not None
        or re.match(r"requires(?:\s|\()", line) is not None
        or (line.startswith("[[") and line.endswith("]]"))
    )


def _is_template_continuation(source: SourceFile, line_number: int) -> bool:
    """Test whether a line is part of a multi-line template parameter list."""
    current_line_end = source.line_end(line_number)
    for previous_line_number in range(line_number, 0, -1):
        line_start = source.line_start(previous_line_number)
        line_end = source.line_end(previous_line_number)
        line = source.masked_text[line_start:line_end].strip()
        template_match = re.match(r"template\s*<", line)
        if template_match is not None:
            template_start = line_start + source.masked_text[line_start:line_end].find("<")
            depth = 0
            closing_offset = None
            for offset in range(template_start, current_line_end):
                character = source.masked_text[offset]
                if character == "<":
                    depth += 1
                elif character == ">" and depth:
                    depth -= 1
                    if depth == 0:
                        closing_offset = offset
                        break
            return closing_offset is None or source.line_number(closing_offset) == line_number
        if ";" in line or "{" in line or "}" in line or source.text[line_start:line_end].lstrip().startswith("///"):
            return False
    return False


def _is_constraint_continuation(source: SourceFile, line_number: int) -> bool:
    """Test whether a line continues a multi-line requires clause."""
    current_line = source.masked_text[source.line_start(line_number) : source.line_end(line_number)].strip()
    if ";" in current_line:
        return False
    for previous_line_number in range(line_number - 1, max(0, line_number - 12), -1):
        line_start = source.line_start(previous_line_number)
        line_end = source.line_end(previous_line_number)
        line = source.masked_text[line_start:line_end].strip()
        if line.startswith("requires"):
            return True
        if ";" in line or line.startswith("template ") or source.text[line_start:line_end].lstrip().startswith("///"):
            return False
    return False


def _has_api_documentation(source: SourceFile, start: int) -> bool:
    """Test whether a contiguous API documentation block precedes a declaration."""
    line_number = source.line_number(start) - 1
    while line_number >= 1:
        line_start = source.line_start(line_number)
        line_end = source.line_end(line_number)
        raw_line = source.text[line_start:line_end]
        stripped_line = source.masked_text[line_start:line_end].strip()
        if raw_line.lstrip().startswith("///"):
            return True
        if (
            not _is_declaration_preamble(stripped_line)
            and not _is_template_continuation(source, line_number)
            and not _is_constraint_continuation(source, line_number)
        ):
            return False
        line_number -= 1
    return False


def _is_defaults_group_gap(source: SourceFile, start: int, end: int) -> bool:
    """Test whether a gap contains only whitespace or declaration preambles."""
    first_line = source.line_number(start)
    last_line = source.line_number(max(start, end - 1))
    for line_number in range(first_line, last_line + 1):
        line_start = max(start, source.line_start(line_number))
        line_end = min(end, source.line_end(line_number))
        line = source.masked_text[line_start:line_end].strip()
        if not line:
            continue
        if (
            _is_declaration_preamble(line)
            or _is_template_continuation(source, line_number)
            or _is_constraint_continuation(source, line_number)
        ):
            continue
        return False
    return True


@cache
def _default_special_member_pattern(type_name: str, signature_index: int) -> re.Pattern[str]:
    """Create and cache one default special-member pattern for a type name."""
    name = re.escape(type_name)
    prefix = r"(?:(?:\[\[[^]]+\]\]|constexpr|consteval|explicit|inline|virtual)\s+)*"
    parameter_name = r"(?:[A-Za-z_]\w*)?"
    patterns = (
        rf"{prefix}{name}\s*\(\s*\)",
        rf"{prefix}~{name}\s*\(\s*\)",
        rf"{prefix}{name}\s*\(\s*(?:(?:const|volatile)\s+)*{name}\s*&\s*{parameter_name}\s*\)",
        rf"{prefix}{name}\s*\(\s*{name}\s*&&\s*{parameter_name}\s*\)",
        rf"{prefix}auto\s+operator\s*=\s*\(\s*(?:(?:const|volatile)\s+)*{name}\s*&\s*{parameter_name}\s*\)",
        rf"{prefix}auto\s+operator\s*=\s*\(\s*{name}\s*&&\s*{parameter_name}\s*\)",
    )
    return re.compile(patterns[signature_index])


def _is_default_special_member_declaration(source: SourceFile, start: int) -> bool:
    """Test whether a declaration has one of the standard default special-member signatures."""
    blocks = source.enclosing_blocks(start)
    if not blocks or blocks[-1].kind not in {"class", "struct"}:
        return False
    statement = " ".join(source.masked_text[start : source.statement_end(start)].split())
    return any(
        _default_special_member_pattern(blocks[-1].name, signature_index).match(statement) is not None
        for signature_index in range(6)
    )


def _is_standard_library_extension(source: SourceFile, offset: int, declarations: Iterable[str]) -> bool:
    """Test whether declarations describe a supported standard-library extension point."""
    in_std_namespace = any(
        block.kind == "namespace"
        and re.search(r"\bnamespace\s+std\s*$", source.masked_text[block.header_start : block.start]) is not None
        for block in source.enclosing_blocks(offset)
    )
    for declaration in declarations:
        match = _standard_library_extension.search(declaration)
        if match is not None and (match.group("qualified") is not None or in_std_namespace):
            return True
    return False


class AntiPatternRule(ABC):
    """Base class for an independently extensible anti-pattern rule."""

    info: RuleInfo

    @abstractmethod
    def scan(self, source: SourceFile) -> Iterable[Candidate]:
        """Yield all candidates found in one source file."""


class RegexRule(AntiPatternRule):
    """Base for simple rules operating on the masked source text."""

    pattern: re.Pattern[str]

    def scan(self, source: SourceFile) -> Iterable[Candidate]:
        for match in self.pattern.finditer(source.masked_text):
            yield Candidate(self.info, match.start(), source.statement_end(match.start()))


class OversizedFileRule(AntiPatternRule):
    """Reject handwritten source files that exceed 500 counted lines."""

    info = RuleInfo("oversized_file", "Files Longer Than 500 Lines", Severity.High)
    _maximum_lines = 500

    def scan(self, source: SourceFile) -> Iterable[Candidate]:
        """Yield the first counted line beyond the file-size limit."""
        if source.is_generated:
            return
        api_documentation_lines = {
            comment.start_line
            for comment in source.comments
            if source.path.suffix == ".hpp"
            and comment.line_comment
            and comment.text.startswith("/")
            and not source.text[source.line_start(comment.start_line) : comment.start].strip()
        }
        counted_lines = 0
        for line_number in range(1, len(source.lines) + 1):
            if line_number in api_documentation_lines:
                continue
            counted_lines += 1
            if counted_lines <= self._maximum_lines:
                continue
            start = source.line_start(line_number)
            yield Candidate(self.info, start, source.line_end(line_number))
            return


class AnonymousNamespaceRule(RegexRule):
    info = RuleInfo("anonymous_namespace", "Anonymous Namespaces", Severity.High)
    pattern = re.compile(r"\bnamespace\s*\{")


class TypeInWrongUnitRule(AntiPatternRule):
    """Reject namespace-scope type definitions in implementation files."""

    info = RuleInfo("type_in_wrong_unit", "Classes and Structs in the Wrong Units", Severity.High)
    _named_type = re.compile(
        r"\b(?P<kind>class|struct)\s+"
        r"(?P<type>(?:[A-Za-z_]\w*(?:\s*<[^;{}()]+>)?\s*::\s*)*"
        r"[A-Za-z_]\w*(?:\s*<[^;{}()]+>)?)"
        r"(?P<suffix>[^;{}]*)$"
    )
    _unnamed_type = re.compile(r"\b(?:class|struct)(?:\s+\[\[[^]]+\]\])?\s*$")
    _namespace_qualifier = re.compile(r"([a-z_]\w*)\s*(?:<[^;{}()]+>)?\s*::")

    def scan(self, source: SourceFile) -> Iterable[Candidate]:
        """Yield misplaced namespace-scope type definitions."""
        reported_starts: set[int] = set()
        for block in source.blocks:
            declaration = self._type_declaration(source, block)
            if declaration is None:
                continue
            start, namespace_qualifiers = declaration
            if source.path.suffix == ".cpp" and _is_namespace_type(source, block):
                yield Candidate(self.info, start, self._definition_end(source, block))
                reported_starts.add(start)
            if not _is_namespace_type(source, block):
                continue
            if _is_standard_library_extension(
                source, block.header_start, (source.masked_text[block.header_start : block.start],)
            ):
                continue
            namespaces = [*self._enclosing_namespace(source, block), *namespace_qualifiers]
            if self._matches_source_directory(source.path, namespaces):
                continue
            if start in reported_starts:
                continue
            yield Candidate(self.info, start, block.start + 1)
            reported_starts.add(start)

    @classmethod
    def _type_declaration(cls, source: SourceFile, block: Block) -> tuple[int, tuple[str, ...]] | None:
        """Return the declaration start and namespace qualifiers for an actual type definition."""
        if block.kind == "enum_class":
            return None
        header = source.masked_text[block.header_start : block.start]
        if block.kind not in {"class", "struct"} and "class" not in header and "struct" not in header:
            return None
        named_match = cls._named_type.search(header)
        if named_match is not None:
            suffix = named_match.group("suffix").strip()
            if suffix and not (
                suffix.startswith(":") or re.match(r"^(?:(?:final|requires)\b|\[\[)", suffix) is not None
            ):
                return None
            qualifiers = tuple(cls._namespace_qualifier.findall(named_match.group("type")))
            return block.header_start + named_match.start("kind"), qualifiers
        unnamed_match = cls._unnamed_type.search(header)
        if unnamed_match is None:
            return None
        return block.header_start + unnamed_match.start(), ()

    @staticmethod
    def _definition_end(source: SourceFile, block: Block) -> int:
        """Return the end of a type definition, including its optional declarator and semicolon."""
        return source.statement_end(block.end + 1)

    @staticmethod
    def _enclosing_namespace(source: SourceFile, block: Block) -> tuple[str, ...]:
        """Return flattened namespace components enclosing a block."""
        result: list[str] = []
        parent = source.immediate_parent(block)
        parents: list[Block] = []
        while parent is not None:
            if parent.kind == "namespace" and parent.name:
                parents.append(parent)
            parent = source.immediate_parent(parent)
        for namespace in reversed(parents):
            result.extend(namespace.name.split("::"))
        return tuple(result)

    @staticmethod
    def _source_directory(path: Path) -> tuple[str, ...] | None:
        """Return directories below src/erbsland, or None outside the library source tree."""
        parts = path.parent.parts
        if len(parts) < 2 or parts[:2] != ("src", "erbsland"):
            return None
        return parts[2:]

    @classmethod
    def _matches_source_directory(cls, path: Path, namespaces: Iterable[str]) -> bool:
        """Test the domain directory and exact implementation-boundary relationship."""
        directories = cls._source_directory(path)
        if directories is None:
            return True
        namespace_parts = tuple(namespaces)
        if not namespace_parts or namespace_parts[0] != "erbsland":
            return False
        namespace_domain = namespace_parts[1] if len(namespace_parts) > 1 else None
        directory_domain = directories[0] if directories else None
        if namespace_domain != directory_domain:
            return False
        return ("impl" in namespace_parts[2:]) == ("impl" in directories[1:])


class NamespaceInWrongUnitRule(AntiPatternRule):
    """Reject namespace declarations whose API boundary differs from their source path."""

    info = RuleInfo("namespace_in_wrong_unit", "Namespaces in the Wrong Units", Severity.High)
    _namespace_keyword = re.compile(r"\bnamespace\b")

    def scan(self, source: SourceFile) -> Iterable[Candidate]:
        """Yield namespace declarations that do not match their source directory."""
        for block in source.blocks:
            if block.kind != "namespace" or not block.name:
                continue
            namespaces = self._namespace_for_block(source, block)
            if not namespaces or namespaces[0] != "erbsland":
                continue
            if len(namespaces) == 1 and self._source_directory(source.path) not in {None, ()}:
                continue
            if self._matches_source_directory(source.path, namespaces):
                continue
            start = self._namespace_declaration_start(source, block)
            yield Candidate(self.info, start, block.start + 1)

    @staticmethod
    def _enclosing_namespace(source: SourceFile, block: Block) -> tuple[str, ...]:
        """Return flattened namespace components enclosing a block."""
        result: list[str] = []
        parent = source.immediate_parent(block)
        parents: list[Block] = []
        while parent is not None:
            if parent.kind == "namespace" and parent.name:
                parents.append(parent)
            parent = source.immediate_parent(parent)
        for namespace in reversed(parents):
            result.extend(namespace.name.split("::"))
        return tuple(result)

    @classmethod
    def _namespace_for_block(cls, source: SourceFile, block: Block) -> tuple[str, ...]:
        """Return flattened namespace components including the given namespace block."""
        return (*cls._enclosing_namespace(source, block), *block.name.split("::"))

    @staticmethod
    def _source_directory(path: Path) -> tuple[str, ...] | None:
        """Return directories below src/erbsland, or None outside the library source tree."""
        parts = path.parent.parts
        if len(parts) < 2 or parts[:2] != ("src", "erbsland"):
            return None
        return parts[2:]

    @classmethod
    def _matches_source_directory(cls, path: Path, namespaces: Iterable[str]) -> bool:
        """Test the domain directory and exact implementation-boundary relationship."""
        directories = cls._source_directory(path)
        if directories is None:
            return True
        namespace_parts = tuple(namespaces)
        if not namespace_parts or namespace_parts[0] != "erbsland":
            return False
        namespace_domain = namespace_parts[1] if len(namespace_parts) > 1 else None
        directory_domain = directories[0] if directories else None
        if namespace_domain != directory_domain:
            return False
        return ("impl" in namespace_parts[2:]) == ("impl" in directories[1:])

    @classmethod
    def _namespace_declaration_start(cls, source: SourceFile, block: Block) -> int:
        """Return the namespace keyword offset for a namespace block."""
        header = source.masked_text[block.header_start : block.start]
        match = cls._namespace_keyword.search(header)
        return block.header_start if match is None else block.header_start + match.start()


class ImplementationInWrongUnitRule(AntiPatternRule):
    """Reject obvious class member definitions outside their owning implementation unit."""

    info = RuleInfo("implementation_in_wrong_unit", "Implementations in the Wrong Units", Severity.High)
    _member_definition = re.compile(
        r"^[ \t]*(?:[^\n;{}=()]+?\s+)?"
        r"(?P<qualified>(?:[A-Za-z_]\w*(?:\s*<[^;{}()\n]+>)?\s*::\s*)+)"
        r"(?P<member>~?[A-Za-z_]\w*|operator\s*(?:\(\s*\)|\[\s*\]|[^\s(]+))\s*\("
    )
    _qualified_component = re.compile(r"([A-Za-z_]\w*)\s*(?:<[^;{}()\n]+>)?\s*::")

    def scan(self, source: SourceFile) -> Iterable[Candidate]:
        """Yield obvious member definitions whose primary class does not match the file unit."""
        if source.path.suffix != ".cpp":
            return
        file_unit = source.path.name.removesuffix(".cpp")
        for line_number in range(1, len(source.lines) + 1):
            line_start = source.line_start(line_number)
            line = source.masked_text[line_start : source.line_end(line_number)]
            if "::" not in line or "(" not in line:
                continue
            match = self._member_definition.match(line)
            if match is None:
                continue
            start = line_start + match.start("qualified")
            if not source.is_namespace_scope(start):
                continue
            owner = self._primary_owner(match.group("qualified"))
            if owner is None or file_unit == owner or file_unit.startswith(f"{owner}_"):
                continue
            yield Candidate(self.info, start, source.statement_end(start))

    @classmethod
    def _primary_owner(cls, qualified: str) -> str | None:
        """Return the first PascalCase qualifier, which identifies the primary owning type."""
        for component in cls._qualified_component.findall(qualified):
            if component[0].isupper():
                return component
        return None


class OversizedNestedTypeRule(AntiPatternRule):
    """Reject nested class and struct definitions that exceed ten code lines."""

    info = RuleInfo("oversized_nested_type", "Oversized Nested Types", Severity.High)
    _maximum_code_lines = 10
    _qualified_nested_declaration = re.compile(
        r"\b(?:class|struct)\s+" r"[A-Z][A-Za-z0-9_]*(?:\s*<[^;{}]+?>)?\s*::\s*[A-Za-z_]\w*"
    )

    def scan(self, source: SourceFile) -> Iterable[Candidate]:
        """Yield nested type definitions whose definition exceeds the line limit."""
        for block in source.blocks:
            if block.kind not in {"class", "struct"} or not self._is_nested_definition(source, block):
                continue
            start = source.type_declaration_start(block)
            if self._code_line_count(source, start, block.end + 1) <= self._maximum_code_lines:
                continue
            yield Candidate(self.info, start, self._definition_end(source, block))

    @classmethod
    def _is_nested_definition(cls, source: SourceFile, block: Block) -> bool:
        """Test whether a block defines a member type inline or at namespace scope."""
        parent = source.immediate_parent(block)
        if parent is not None and parent.kind in {"class", "struct"}:
            return True
        if not source.is_namespace_scope(source.type_declaration_start(block)):
            return False
        header = source.masked_text[block.header_start : block.start]
        if _is_standard_library_extension(source, block.header_start, (header,)):
            return False
        return cls._qualified_nested_declaration.search(header) is not None

    @staticmethod
    def _code_line_count(source: SourceFile, start: int, end: int) -> int:
        """Count non-empty masked source lines in a definition range."""
        first_line = source.line_number(start)
        last_line = source.line_number(max(start, end - 1))
        result = 0
        for line_number in range(first_line, last_line + 1):
            line_start = max(start, source.line_start(line_number))
            line_end = min(end, source.line_end(line_number))
            if source.masked_text[line_start:line_end].strip():
                result += 1
        return result

    @staticmethod
    def _definition_end(source: SourceFile, block: Block) -> int:
        """Return the end of a type definition, including its optional semicolon."""
        end = block.end + 1
        while end < len(source.masked_text) and source.masked_text[end] in " \t":
            end += 1
        if end < len(source.masked_text) and source.masked_text[end] == ";":
            end += 1
        return end


class ForwardDeclarationRule(AntiPatternRule):
    info = RuleInfo("forward_declaration", "Forward Declarations at the Usage Location", Severity.High)
    pattern = re.compile(r"\b(?:class|struct)\s+[A-Za-z_]\w*\s*;")

    def scan(self, source: SourceFile) -> Iterable[Candidate]:
        if source.path.name.endswith("_fwd.hpp"):
            return
        for match in self.pattern.finditer(source.masked_text):
            if source.is_namespace_scope(match.start()):
                yield Candidate(self.info, match.start(), match.end())


class StaticGlobalObjectRule(AntiPatternRule):
    info = RuleInfo("static_global_object", "Static Global Object Construction", Severity.High)
    _line_pattern = re.compile(
        r"(?m)^[ \t]*(?P<declaration>"
        r"(?!(?:constexpr|constinit|extern|thread_local)\b)"
        r"(?:(?:inline|static|const|volatile)\s+)*"
        r"(?P<type>(?:[A-Z][A-Za-z0-9_]*|(?:[A-Za-z_]\w*::)+[A-Za-z_]\w*)"
        r"(?:\s*<[^;{}=()]+>)?)\s+"
        r"(?P<name>[A-Za-z_]\w*)\s*(?P<initializer>\{|=))"
    )
    _array_pattern = re.compile(
        r"(?m)^[ \t]*(?P<declaration>"
        r"(?!(?:constexpr|extern)\b)(?:(?:inline|static|const|volatile)\s+)*"
        r"[A-Za-z_][A-Za-z0-9_:<> ,]*\s+[A-Za-z_]\w*\s*\[[^\]]*\]\s*(?:\{|=))"
    )

    def scan(self, source: SourceFile) -> Iterable[Candidate]:
        matches = [*self._line_pattern.finditer(source.masked_text), *self._array_pattern.finditer(source.masked_text)]
        seen: set[int] = set()
        for match in sorted(matches, key=lambda item: item.start()):
            start = match.start("declaration")
            if start in seen or not source.is_namespace_scope(start):
                continue
            declaration = match.group("declaration")
            if "*" in declaration or "&" in declaration:
                continue
            if source.has_unclosed_delimiter_before(start, "(", ")"):
                continue
            if source.has_unclosed_delimiter_before(start, "<", ">"):
                continue
            seen.add(start)
            yield Candidate(self.info, start, source.statement_end(start))


class StaticOnlyClassRule(AntiPatternRule):
    info = RuleInfo("static_only_class", "Classes with Only Static Methods", Severity.High)

    def scan(self, source: SourceFile) -> Iterable[Candidate]:
        for block in source.blocks:
            if block.kind not in {"class", "struct"} or not _is_namespace_type(source, block):
                continue
            header = source.masked_text[block.header_start : block.start]
            if ":" in header:
                continue
            if self._has_only_static_members(source, block):
                yield Candidate(self.info, source.type_declaration_start(block), block.start + 1)

    def _has_only_static_members(self, source: SourceFile, block: Block) -> bool:
        body = list(source.masked_text[block.start + 1 : block.end])
        for child in source.blocks:
            if child.parent_index is None or source.blocks[child.parent_index] != block:
                continue
            relative_start = max(0, child.header_start - block.start - 1)
            relative_end = min(len(body), child.end - block.start)
            header = source.masked_text[child.header_start : child.start]
            if child.kind == "other" and "(" in header and "static" not in header:
                return False
            for index in range(relative_start, relative_end):
                if body[index] not in "\r\n":
                    body[index] = " "
        static_member = False
        for raw_member in "".join(body).split(";"):
            member = re.sub(r"\b(?:public|protected|private)\s*:\s*", "", raw_member).strip()
            if not member:
                continue
            if re.match(r"^(?:using|typedef|friend|static_assert|class|struct|enum)\b", member):
                continue
            if re.search(r"=\s*(?:delete|default)\s*$", member) and (block.name in member or "operator=" in member):
                continue
            if re.search(r"\bstatic\b", member):
                static_member = True
                continue
            return False
        return static_member


class MissingApiDocumentationRule(AntiPatternRule):
    info = RuleInfo("missing_api_documentation", "Missing API Documentation", Severity.Medium)
    pattern = re.compile(
        r"(?m)^[ \t]*(?P<declaration>"
        r"(?:(?:\[\[[^\n]*\]\]\s*)|(?:(?:constexpr|consteval|constinit|explicit|inline|static|virtual)\s+))*"
        r"(?:class|struct|enum\s+class|using\s+[A-Za-z_]\w*\s*=|auto|void)\b)"
    )
    _constructor_or_destructor = re.compile(
        r"(?m)^[ \t]*(?P<declaration>"
        r"(?:(?:\[\[[^\n]*\]\]\s*)|(?:(?:constexpr|consteval|constinit|explicit|inline|virtual)\s+))*"
        r"(?P<name>~?[A-Za-z_]\w*)\s*\()"
    )
    _defaulted_comparison_operator = re.compile(
        r"\boperator\s*(?:==|!=|<=|>=|<=>|<|>)\s*\([^;]*\)[^;=]*=\s*default\s*;"
    )
    _override = re.compile(r"\boverride\b")
    _private_tag = re.compile(r"\b(?:class|struct)\s+PrivateTag\b")

    def scan(self, source: SourceFile) -> Iterable[Candidate]:
        if source.path.suffix != ".hpp" or source.path.name.endswith("_fwd.hpp"):
            return
        declarations = tuple(self._declarations(source))
        defaulted_or_deleted_starts = {
            match.start("declaration")
            for match in _defaulted_or_deleted_special_member.finditer(source.masked_text)
            if match.group("name") is None
            or _is_special_member(source, match.start("declaration"), match.group("name"))
        }
        defaults_group_starts = self._defaults_group_starts(source, declarations)
        for start, declaration in declarations:
            if (
                not self._is_api_scope(source, start)
                or not self._is_documentable_declaration(source, start, declaration)
                or _has_api_documentation(source, start)
            ):
                continue
            if self._is_standard_library_extension(source, start):
                continue
            if self._is_private_tag(source, start):
                continue
            if self._is_overriding_method(source, start):
                continue
            if start in defaulted_or_deleted_starts or start in defaults_group_starts:
                continue
            if self._is_defaulted_comparison_operator(source, start):
                continue
            yield Candidate(self.info, start, source.statement_end(start))

    @classmethod
    def _declarations(cls, source: SourceFile) -> Iterable[tuple[int, str]]:
        """Yield deduplicated likely API declarations in source order."""
        candidates = [
            (match.start("declaration"), match.group("declaration"))
            for match in cls.pattern.finditer(source.masked_text)
        ]
        candidates.extend(
            (match.start("declaration"), match.group("declaration"))
            for match in cls._constructor_or_destructor.finditer(source.masked_text)
            if cls._is_constructor_or_destructor(source, match)
        )
        seen: set[int] = set()
        for start, declaration in sorted(candidates, key=lambda item: item[0]):
            if start in seen:
                continue
            seen.add(start)
            yield start, declaration

    @staticmethod
    def _defaults_group_starts(source: SourceFile, declarations: tuple[tuple[int, str], ...]) -> frozenset[int]:
        """Find contiguous default special-member declarations beneath defaults-group markers."""
        result: set[int] = set()
        for marker in source.comments:
            if _defaults_group_marker.fullmatch(marker.text) is None:
                continue
            position = marker.end
            for start, _ in declarations:
                if start <= marker.end:
                    continue
                if not _is_defaults_group_gap(source, position, start):
                    break
                if not _is_default_special_member_declaration(source, start):
                    break
                result.add(start)
                position = source.statement_end(start)
        return frozenset(result)

    @staticmethod
    def _is_constructor_or_destructor(source: SourceFile, match: re.Match[str]) -> bool:
        """Test whether a name begins a class special-member declaration rather than an initializer expression."""
        start = match.start("declaration")
        if not _is_special_member(source, start, match.group("name")) or source.has_unclosed_delimiter_before(
            start, "(", ")"
        ):
            return False
        previous_line_number = source.line_number(start) - 1
        while previous_line_number >= 1:
            line = source.masked_text[
                source.line_start(previous_line_number) : source.line_end(previous_line_number)
            ].strip()
            if line:
                break
            previous_line_number -= 1
        if previous_line_number < 1:
            return True
        if re.fullmatch(r"(?:public|protected|private)\s*:", line) is not None:
            return True
        if line.endswith((";", "{", "}")):
            return True
        return (
            _is_declaration_preamble(line)
            or _is_template_continuation(source, previous_line_number)
            or _is_constraint_continuation(source, previous_line_number)
        )

    @staticmethod
    def _is_api_scope(source: SourceFile, start: int) -> bool:
        """Test whether a declaration is outside a function or lambda body."""
        return all(block.kind in {"namespace", "class", "struct"} for block in source.enclosing_blocks(start))

    @staticmethod
    def _is_documentable_declaration(source: SourceFile, start: int, declaration: str) -> bool:
        """Test whether a declaration is an API instead of a forward declaration or data member."""
        statement = source.masked_text[start : source.statement_end(start)]
        if re.search(r"\b(?:class|struct|enum\s+class)\b", declaration) is not None:
            return "{" in statement
        if re.search(r"\busing\b", declaration) is not None:
            return True
        if re.search(r"\bvoid\b", declaration) is not None:
            return "(" in statement
        if "operator" in statement:
            return "(" in statement
        if re.search(r"\bauto\b", declaration) is None:
            return True
        auto_start = statement.find("auto")
        opening_parenthesis = statement.find("(", auto_start)
        assignment = statement.find("=", auto_start)
        opening_brace = statement.find("{", auto_start)
        return opening_parenthesis >= 0 and (
            (assignment < 0 or opening_parenthesis < assignment)
            and (opening_brace < 0 or opening_parenthesis < opening_brace)
        )

    @staticmethod
    def _is_standard_library_extension(source: SourceFile, start: int) -> bool:
        """Test whether the declaration belongs to an undocumented standard-library extension point."""
        statement_end = source.statement_end(start)
        preamble_start = source.line_start(max(1, source.line_number(start) - 1))
        declarations = (
            source.masked_text[start:statement_end],
            source.masked_text[preamble_start:statement_end],
            *(
                source.masked_text[block.header_start : block.start]
                for block in source.enclosing_blocks(start)
                if block.kind in {"class", "struct"}
            ),
        )
        return _is_standard_library_extension(source, start, declarations)

    @staticmethod
    def _is_private_tag(source: SourceFile, start: int) -> bool:
        """Test whether a self-explanatory private-constructor tag is declared."""
        statement = source.masked_text[start : source.statement_end(start)]
        return MissingApiDocumentationRule._private_tag.search(statement) is not None

    @staticmethod
    def _is_overriding_method(source: SourceFile, start: int) -> bool:
        """Test whether a method inherits its API documentation from a base declaration."""
        statement = source.masked_text[start : source.statement_end(start)]
        return MissingApiDocumentationRule._override.search(statement) is not None

    @staticmethod
    def _is_defaulted_comparison_operator(source: SourceFile, start: int) -> bool:
        """Test whether a defaulted comparison operator is self-describing."""
        statement = source.masked_text[start : source.statement_end(start)]
        return MissingApiDocumentationRule._defaulted_comparison_operator.search(statement) is not None


class MissingDefaultGroupCommentRule(AntiPatternRule):
    """Require explicitly defaulted or deleted special members to form a labeled group."""

    info = RuleInfo("missing_default_group_comment", "Missing Default Group Comment", Severity.Medium)

    def scan(self, source: SourceFile) -> Iterable[Candidate]:
        """Yield special members outside a defaults group in header APIs."""
        if source.path.suffix != ".hpp" or source.path.name.endswith("_fwd.hpp"):
            return
        for match in _defaulted_or_deleted_special_member.finditer(source.masked_text):
            start = match.start("declaration")
            if not _is_special_member(source, start, match.group("name")):
                continue
            if self._has_defaults_group(source, start):
                continue
            if self._is_documented_default_constructor(source, start, match.group("name")):
                continue
            yield Candidate(self.info, start, source.statement_end(start))

    @staticmethod
    def _has_defaults_group(source: SourceFile, start: int) -> bool:
        """Test whether a defaults-group marker directly precedes this special-member block."""
        marker = next(
            (
                comment
                for comment in reversed(source.comments)
                if comment.end <= start and _defaults_group_marker.fullmatch(comment.text) is not None
            ),
            None,
        )
        if marker is None:
            return False
        position = marker.end
        for match in _defaulted_or_deleted_special_member.finditer(
            source.masked_text, marker.end, source.statement_end(start)
        ):
            declaration_start = match.start("declaration")
            if not _is_defaults_group_gap(source, position, declaration_start):
                return False
            if declaration_start == start:
                return True
            position = match.end("declaration")
        return False

    @staticmethod
    def _is_documented_default_constructor(source: SourceFile, start: int, name: str | None) -> bool:
        """Test whether a documented explicitly defaulted default constructor stands on its own."""
        if name is None or name.startswith("~"):
            return False
        statement = source.masked_text[start : source.statement_end(start)]
        default_constructor = re.compile(rf"\b{re.escape(name)}\s*\(\s*\)\s*[^{{;=]*=\s*default\s*;")
        return default_constructor.search(statement) is not None and _has_api_documentation(source, start)


class MultipleTypesInHeaderRule(AntiPatternRule):
    info = RuleInfo("multiple_types_in_header", "Multiple Types in One Header", Severity.Medium)
    _class_declaration = re.compile(r"\b(?:class|struct)\s+(?P<name>[A-Za-z_]\w*)")

    def scan(self, source: SourceFile) -> Iterable[Candidate]:
        if source.path.suffix != ".hpp":
            return
        types = [
            block
            for block in source.blocks
            if block.kind in {"class", "struct", "enum_class"} and _is_namespace_type(source, block)
        ]
        if len(types) <= 1:
            return
        primary = next((block for block in types if block.name == source.path.stem), types[0])
        internal = "impl" in source.path.parts
        for block in types:
            if block == primary:
                continue
            if self._is_primary_template_specialization(source, primary, block):
                continue
            if self._is_standard_library_specialization(source, block):
                continue
            if internal and block.kind in {"class", "struct"} and self._small_functionless(source, block):
                continue
            yield Candidate(self.info, source.type_declaration_start(block), block.start + 1)

    @classmethod
    def _is_primary_template_specialization(cls, source: SourceFile, primary: Block, block: Block) -> bool:
        """Test whether a block specializes the primary class template of this header."""
        if (
            primary.kind not in {"class", "struct"}
            or block.kind not in {"class", "struct"}
            or primary.name != source.path.stem
            or block.name != primary.name
            or cls._namespace_scope(source, block) != cls._namespace_scope(source, primary)
        ):
            return False
        return (
            cls._template_declaration_kind(source, primary) == "primary"
            and cls._template_declaration_kind(source, block) == "specialization"
        )

    @classmethod
    def _template_declaration_kind(cls, source: SourceFile, block: Block) -> str:
        """Classify a type block as a primary template, specialization, or regular type."""
        header = source.masked_text[block.header_start : block.start]
        declarations = [match for match in cls._class_declaration.finditer(header) if match.group("name") == block.name]
        if not declarations:
            return "regular"
        declaration = declarations[-1]
        if re.search(r"\btemplate\s*<", header[: declaration.start()]) is None:
            return "regular"
        suffix = header[declaration.end() :].lstrip()
        return "specialization" if suffix.startswith("<") else "primary"

    @staticmethod
    def _namespace_scope(source: SourceFile, block: Block) -> tuple[str, ...]:
        """Return the canonical namespace path containing a type block."""
        namespaces: list[Block] = []
        parent = source.immediate_parent(block)
        while parent is not None:
            if parent.kind == "namespace":
                namespaces.append(parent)
            parent = source.immediate_parent(parent)
        result: list[str] = []
        for namespace in reversed(namespaces):
            if namespace.name:
                result.extend(namespace.name.split("::"))
            else:
                result.append(f"<anonymous:{namespace.start}>")
        return tuple(result)

    @staticmethod
    def _is_standard_library_specialization(source: SourceFile, block: Block) -> bool:
        """Test whether a type block is a supported standard-library template specialization."""
        header = source.masked_text[block.header_start : block.start]
        return _is_standard_library_extension(source, block.header_start, (header,))

    @staticmethod
    def _small_functionless(source: SourceFile, block: Block) -> bool:
        line_count = source.line_number(block.end) - source.line_number(block.header_start) + 1
        if line_count >= 12:
            return False
        body = source.masked_text[block.start + 1 : block.end]
        return "(" not in body


class RegularStringLiteralRule(AntiPatternRule):
    info = RuleInfo("regular_string_literal", "Regular Literal Strings for Erbsland Core APIs", Severity.Medium)
    _direct_construction = re.compile(
        r"(?:\b[A-Za-z_]\w*::)*(?:" r"[A-Za-z_]\w*(?:\s*<[^;{}]+>)?\s*\{" r"|[A-Z]\w*(?:\s*<[^;{}]+>)?\s*\()\s*$"
    )
    _assertion_context = re.compile(r"\b(?:static_assert|assert)\s*(?P<opening>\()")
    _deprecated_context = re.compile(r"\[\[[^]]*\bdeprecated\s*(?P<opening>\()")

    def scan(self, source: SourceFile) -> Iterable[Candidate]:
        literals = source.string_literals
        allowed_contexts = self._allowed_contexts(source)
        index = 0
        while index < len(literals):
            group = [literals[index]]
            index += 1
            while index < len(literals) and not source.text[group[-1].end : literals[index].start].strip():
                group.append(literals[index])
                index += 1
            if (
                any(literal.prefix for literal in group)
                or group[-1].suffix == "_el"
                or self._is_direct_construction(source, group[0].start)
                or self._is_in_allowed_context(group[0].start, allowed_contexts)
            ):
                continue
            next_index = group[-1].end
            while next_index < len(source.masked_text) and source.masked_text[next_index].isspace():
                next_index += 1
            if next_index >= len(source.masked_text) or source.masked_text[next_index] not in ";)}":
                continue
            start = group[0].start
            yield Candidate(self.info, start, source.statement_end(next_index))

    @classmethod
    def _is_direct_construction(cls, source: SourceFile, start: int) -> bool:
        """Test whether the literal is the first argument of an explicit direct construction."""
        prefix = source.masked_text[max(0, start - 256) : start]
        return cls._direct_construction.search(prefix) is not None

    @classmethod
    def _allowed_contexts(cls, source: SourceFile) -> tuple[tuple[int, int], ...]:
        """Find assertion and deprecation contexts where narrow literals are required."""
        ranges: list[tuple[int, int]] = []
        for pattern in (cls._assertion_context, cls._deprecated_context):
            for match in pattern.finditer(source.masked_text):
                opening = match.start("opening")
                ranges.append((opening, cls._closing_parenthesis(source, opening)))
        return tuple(ranges)

    @staticmethod
    def _closing_parenthesis(source: SourceFile, opening: int) -> int:
        """Find the closing parenthesis paired with an opening parenthesis."""
        depth = 0
        for offset in range(opening, len(source.masked_text)):
            character = source.masked_text[offset]
            if character == "(":
                depth += 1
            elif character == ")":
                depth -= 1
                if depth == 0:
                    return offset
        return opening

    @staticmethod
    def _is_in_allowed_context(start: int, contexts: tuple[tuple[int, int], ...]) -> bool:
        """Test whether a literal starts inside an allowed context range."""
        return any(opening < start < closing for opening, closing in contexts)


class StaticCastVoidRule(RegexRule):
    info = RuleInfo("static_cast_void", "Silencing nodiscard with static_cast<void>", Severity.Medium)
    pattern = re.compile(r"\bstatic_cast\s*<\s*void\s*>\s*\(")


class NestedNamespaceRule(AntiPatternRule):
    """Reject nested named namespace blocks and namespace end comments."""

    info = RuleInfo("nested_namespace", "Nested Namespace Blocks and End Comments", Severity.Low)
    _namespace_keyword = re.compile(r"\bnamespace\b")

    def scan(self, source: SourceFile) -> Iterable[Candidate]:
        """Yield nested namespace declarations and comments attached to namespace endings."""
        for block in source.blocks:
            if block.kind != "namespace":
                continue
            parent = source.immediate_parent(block)
            if block.name and parent is not None and parent.kind == "namespace" and parent.name:
                start = self._namespace_declaration_start(source, block)
                yield Candidate(self.info, start, block.start + 1)
            end_comment = self._end_comment(source, block)
            if end_comment is not None:
                yield Candidate(self.info, end_comment.start, end_comment.end)

    @classmethod
    def _namespace_declaration_start(cls, source: SourceFile, block: Block) -> int:
        """Return the namespace keyword offset for a namespace block."""
        header = source.masked_text[block.header_start : block.start]
        match = cls._namespace_keyword.search(header)
        return block.header_start if match is None else block.header_start + match.start()

    @staticmethod
    def _end_comment(source: SourceFile, block: Block) -> Comment | None:
        """Return a same-line comment attached to a namespace closing brace."""
        closing_line = source.line_number(block.end)
        for comment in source.comments:
            if comment.start_line != closing_line or comment.start <= block.end:
                continue
            if source.text[block.end + 1 : comment.start].strip():
                continue
            return comment
        return None


RULES: tuple[AntiPatternRule, ...] = (
    OversizedFileRule(),
    AnonymousNamespaceRule(),
    TypeInWrongUnitRule(),
    NamespaceInWrongUnitRule(),
    ImplementationInWrongUnitRule(),
    OversizedNestedTypeRule(),
    ForwardDeclarationRule(),
    StaticGlobalObjectRule(),
    StaticOnlyClassRule(),
    MissingApiDocumentationRule(),
    MissingDefaultGroupCommentRule(),
    MultipleTypesInHeaderRule(),
    RegularStringLiteralRule(),
    StaticCastVoidRule(),
    NestedNamespaceRule(),
)

RULES_BY_IDENTIFIER = {rule.info.identifier: rule for rule in RULES}
