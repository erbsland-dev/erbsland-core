# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import argparse
import re
import xml.etree.ElementTree as ET
from collections import defaultdict
from collections.abc import Callable, Sequence
from dataclasses import dataclass
from pathlib import Path

from lib.config import read_elcl_file
from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.path_safety import read_safe_text, require_directory, require_safe_existing_file, resolve_project_path
from lib.utility import UtilityApp

WarningFn = Callable[[Path, int, str], None]


@dataclass(frozen=True)
class CleanupRstConfig:
    """Configuration for the reStructuredText cleanup utility."""

    project_dir: Path
    line_width: int
    doxygen_index: Path

    @classmethod
    def read(cls, project_dir: Path, config_file: Path) -> "CleanupRstConfig":
        """Read the cleanup-rst configuration from an ELCL file."""
        project_dir = project_dir.resolve()
        main_config = read_elcl_file(config_file)["main"]
        result = cls(
            project_dir=project_dir,
            line_width=main_config.get_int("line_width", default=120),
            doxygen_index=resolve_project_path(project_dir, main_config.get_text("doxygen_index"), "Doxygen Index"),
        )
        result.validate()
        return result

    def validate(self) -> None:
        """Validate all configured values."""
        if self.line_width < 20:
            raise UtilityError(f"Line Width must be at least 20: {self.line_width}")
        require_safe_existing_file(self.doxygen_index, "Doxygen Index", FileUpdate.MAX_COMPARE_FILE_SIZE)
        if not self.doxygen_index.exists():
            raise UtilityError(f"Doxygen Index does not exist: {self.doxygen_index}")


@dataclass(frozen=True)
class DoxygenSymbol:
    """One public symbol read from the Doxygen index."""

    role: str
    full_name: str

    @property
    def short_name(self) -> str:
        """Get the final C++ name component."""
        return self.full_name.rsplit("::", 1)[-1]


@dataclass(frozen=True)
class InlineReference:
    """One parsed inline reference body."""

    display: str
    target: str


class DoxygenSymbolIndex:
    """A lookup table for public C++ symbols from the Doxygen index."""

    COMPOUND_KIND_TO_ROLE = {
        "class": "class",
        "struct": "struct",
        "concept": "concept",
    }
    MEMBER_KIND_TO_ROLE = {
        "enum": "enum",
        "function": "func",
        "typedef": "type",
    }

    def __init__(self, symbols: Sequence[DoxygenSymbol]) -> None:
        self.symbols = tuple(symbols)
        self.by_full_name: dict[str, list[DoxygenSymbol]] = defaultdict(list)
        self.by_short_name: dict[str, list[DoxygenSymbol]] = defaultdict(list)
        self.by_suffix: dict[str, list[DoxygenSymbol]] = defaultdict(list)
        for symbol in self.symbols:
            self.by_full_name[symbol.full_name].append(symbol)
            self.by_short_name[symbol.short_name].append(symbol)
            parts = symbol.full_name.split("::")
            for suffix_start in range(1, len(parts)):
                suffix = "::".join(parts[suffix_start:])
                self.by_suffix[suffix].append(symbol)

    @classmethod
    def read(cls, index_path: Path) -> "DoxygenSymbolIndex":
        """Read a Doxygen XML index."""
        try:
            root = ET.parse(index_path).getroot()
        except ET.ParseError as error:
            raise UtilityError(f"Could not parse Doxygen Index: {error}") from None

        symbols: set[DoxygenSymbol] = set()
        for compound in root.findall("compound"):
            compound_name = compound.findtext("name", default="").strip()
            compound_kind = compound.get("kind", "")
            compound_role = cls.COMPOUND_KIND_TO_ROLE.get(compound_kind)
            if compound_role is not None and cls.is_public_symbol(compound_name):
                symbols.add(DoxygenSymbol(compound_role, compound_name))

            if not cls.is_public_scope(compound_name, compound_kind):
                continue
            for member in compound.findall("member"):
                member_role = cls.MEMBER_KIND_TO_ROLE.get(member.get("kind", ""))
                member_name = member.findtext("name", default="").strip()
                if member_role is None or not member_name:
                    continue
                if cls.is_constructor_or_destructor(compound_name, member_name):
                    continue
                full_name = f"{compound_name}::{member_name}"
                if cls.is_public_symbol(full_name):
                    symbols.add(DoxygenSymbol(member_role, full_name))
        return cls(cls.without_duplicate_deduction_guides(symbols))

    @staticmethod
    def is_public_scope(name: str, kind: str) -> bool:
        """Test if a Doxygen compound can contain public API members."""
        return kind in {"namespace", "class", "struct"} and name.startswith("erbsland::")

    @staticmethod
    def is_public_symbol(name: str) -> bool:
        """Test if a symbol name belongs to the public API surface."""
        if not name.startswith("erbsland::") or "::impl::" in name:
            return False
        parts = name.split("::")
        return all(part and not part.startswith("_") and "<" not in part and ">" not in part for part in parts)

    @staticmethod
    def is_constructor_or_destructor(compound_name: str, member_name: str) -> bool:
        """Test if a class member is a constructor or destructor entry."""
        class_name = compound_name.rsplit("::", 1)[-1]
        return member_name == class_name or member_name == f"~{class_name}"

    @classmethod
    def without_duplicate_deduction_guides(cls, symbols: set[DoxygenSymbol]) -> list[DoxygenSymbol]:
        """Remove Doxygen function entries that duplicate class template names."""
        compound_names = {
            symbol.full_name for symbol in symbols if symbol.role in cls.COMPOUND_KIND_TO_ROLE.values()
        }
        filtered_symbols = {
            symbol for symbol in symbols if not (symbol.role == "func" and symbol.full_name in compound_names)
        }
        return sorted(filtered_symbols, key=lambda symbol: (symbol.full_name, symbol.role))

    def resolve(self, query: str) -> list[DoxygenSymbol]:
        """Resolve a full, suffix, or short symbol name."""
        query = self.normalize_query(query)
        if not query:
            return []
        if query.startswith("el::"):
            query = f"erbsland::{query[4:]}"
        if query.startswith("erbsland::"):
            return self.unique_symbols(self.by_full_name.get(query, []))
        if "::" in query:
            direct = self.by_full_name.get(f"erbsland::{query}", [])
            suffix = self.by_suffix.get(query, [])
            return self.unique_symbols([*direct, *suffix])
        return self.unique_symbols(self.by_short_name.get(query, []))

    @staticmethod
    def normalize_query(query: str) -> str:
        """Normalize user-written C++ reference text for lookup."""
        query = query.strip()
        if query.endswith("()"):
            query = query[:-2]
        return query

    @staticmethod
    def unique_symbols(symbols: Sequence[DoxygenSymbol]) -> list[DoxygenSymbol]:
        """Deduplicate matching symbols by role and full target."""
        result = sorted(set(symbols), key=lambda symbol: (symbol.full_name, symbol.role))
        return result


class InlineLinker:
    """Rewrite inline C++ references to fully-qualified Sphinx roles."""

    RE_REFERENCE = re.compile(
        r"""
        (?P<prefix>:cpp:(?P<role>any|type|class|struct|enum|func):)?
        (?<!`)
        `
        (?P<body>[^`\n]+)
        `
        (?!`)
        """,
        re.VERBOSE,
    )
    RE_CPP_REFERENCE_BODY = re.compile(
        r"""
        ^
        (?P<name>
            [A-Za-z_][A-Za-z0-9_]*
            (?:
                (?:::|::operator|::operator\"\"|::operator)
                [A-Za-z0-9_+\-*/%<>=!&|^~\[\](),"]*
            )*
            (?:\(\))?
        )
        $
        """,
        re.VERBOSE,
    )
    RE_EXPLICIT_TITLE = re.compile(r"^(?P<display>.+?)\s+<(?P<target>[^<>]+)>$")

    def __init__(self, index: DoxygenSymbolIndex, warn: WarningFn) -> None:
        self.index = index
        self.warn = warn

    def cleanup_line(self, path: Path, line_number: int, line: str) -> str:
        """Rewrite all supported inline references in one line."""

        def replace(match: re.Match[str]) -> str:
            prefix = match.group("prefix")
            body = match.group("body").strip()
            if prefix is None and self.is_probably_not_bare_cpp_reference(match, line):
                return match.group(0)
            reference = self.parse_reference_body(body)
            if reference is None:
                return match.group(0)
            symbols = self.index.resolve(reference.target)
            if len(symbols) != 1:
                reason = "No Doxygen symbol found" if not symbols else "Ambiguous Doxygen symbol"
                self.warn(path, line_number, f"{reason} for '{reference.target}'.")
                return f"``{self.literal_text(reference.display)}``"
            symbol = symbols[0]
            display = reference.display or symbol.short_name
            return f":cpp:{symbol.role}:`{display} <{symbol.full_name}>`"

        return self.RE_REFERENCE.sub(replace, line)

    @staticmethod
    def is_probably_not_bare_cpp_reference(match: re.Match[str], line: str) -> bool:
        """Avoid matching the target part of a non-C++ Sphinx role."""
        start = match.start()
        if start == 0:
            return False
        return line[start - 1] == ":"

    @classmethod
    def parse_reference_body(cls, body: str) -> InlineReference | None:
        """Parse inline role body text."""
        match = cls.RE_EXPLICIT_TITLE.fullmatch(body)
        if match is not None:
            display = match.group("display").strip()
            target = match.group("target").strip()
            if not cls.RE_CPP_REFERENCE_BODY.fullmatch(target):
                return None
            return InlineReference(display, target)
        if not cls.RE_CPP_REFERENCE_BODY.fullmatch(body):
            return None
        return InlineReference(body, body)

    @staticmethod
    def literal_text(text: str) -> str:
        """Create safe inline literal text."""
        return text.replace("``", "` `").strip()


class ParagraphWrapper:
    """Wrap simple top-level paragraphs."""

    RE_ATOMIC_MARKUP_START = re.compile(r":[A-Za-z][A-Za-z0-9_-]*(?::[A-Za-z][A-Za-z0-9_-]*)*:`")
    RE_INITIALISM_TOKEN = re.compile(r"^(?:[A-Za-z]\.)+$")
    RE_DIGIT_FULL_STOP = re.compile(r"\d\.$")
    FULL_STOP_ABBREVIATIONS = frozenset(
        {
            "ca.",
            "cf.",
            "ch.",
            "chap.",
            "e.g.",
            "etc.",
            "fig.",
            "figs.",
            "i.e.",
            "max.",
            "min.",
            "no.",
            "nos.",
            "ref.",
            "refs.",
            "sec.",
            "secs.",
            "std.",
            "vs.",
        }
    )
    WRAPPING_PUNCTUATION = "\"'()[]{}"

    def __init__(self, line_width: int) -> None:
        self.line_width = line_width

    def wrap(self, text: str) -> list[str] | None:
        """Wrap a paragraph, or return `None` if it must be left as-is."""
        tokens = self.tokens(text)
        if any(len(token) > self.line_width for token in tokens):
            return None

        lines: list[str] = []
        current = ""
        for token in tokens:
            if not current:
                current = token
            elif len(current) + 1 + len(token) <= self.line_width:
                current = f"{current} {token}"
            else:
                lines.append(current)
                current = token
            if self.is_sentence_end(token):
                lines.append(current)
                current = ""
        if current:
            lines.append(current)
        return lines

    @classmethod
    def is_sentence_end(cls, token: str) -> bool:
        """Test if a token-ending full stop should create a sentence break."""
        token = token.strip(cls.WRAPPING_PUNCTUATION)
        if not token.endswith("."):
            return False
        return not cls.is_full_stop_exception(token)

    @classmethod
    def is_full_stop_exception(cls, token: str) -> bool:
        """Test if a full stop belongs to an abbreviation, initialism or number."""
        if token.casefold() in cls.FULL_STOP_ABBREVIATIONS:
            return True
        if cls.RE_INITIALISM_TOKEN.fullmatch(token) is not None:
            return True
        return cls.RE_DIGIT_FULL_STOP.search(token) is not None

    def tokens(self, text: str) -> list[str]:
        """Split a paragraph into wrapping tokens while preserving inline markup."""
        result: list[str] = []
        index = 0
        while index < len(text):
            while index < len(text) and text[index].isspace():
                index += 1
            if index >= len(text):
                break
            end = self.atomic_markup_end(text, index)
            if end is None:
                end = index + 1
                while end < len(text) and not text[end].isspace():
                    end += 1
            while end < len(text) and text[end] in ",.;:":
                end += 1
            result.append(text[index:end])
            index = end
        return result

    def atomic_markup_end(self, text: str, index: int) -> int | None:
        """Find the end of an inline literal or role token starting at index."""
        if text.startswith("``", index):
            end = text.find("``", index + 2)
            return None if end < 0 else end + 2
        match = self.RE_ATOMIC_MARKUP_START.match(text, index)
        if match is not None:
            end = text.find("`", match.end())
            return None if end < 0 else end + 1
        return None


class RstCleaner:
    """Normalize one reStructuredText document."""

    TITLE_CHARS = {"*", "=", "-", "~"}

    def __init__(self, config: CleanupRstConfig, index: DoxygenSymbolIndex, warn: WarningFn) -> None:
        self.config = config
        self.linker = InlineLinker(index, warn)
        self.wrapper = ParagraphWrapper(config.line_width)

    def cleanup_text(self, path: Path, text: str) -> str:
        """Clean up one reStructuredText document."""
        had_final_newline = text.endswith("\n")
        lines = self.fix_title_lines(text.splitlines())
        code_lines = self.code_block_lines(lines)
        result: list[str] = []
        index = 0
        while index < len(lines):
            if index in code_lines:
                result.append(lines[index])
                index += 1
                continue
            if self.is_simple_paragraph_line(lines, index):
                start = index
                paragraph_lines: list[str] = []
                while index < len(lines) and index not in code_lines and self.is_simple_paragraph_line(lines, index):
                    paragraph_lines.append(lines[index])
                    index += 1
                linked_lines = [
                    self.linker.cleanup_line(path, line_index, line)
                    for line_index, line in enumerate(paragraph_lines, start=start + 1)
                ]
                paragraph = " ".join(line.strip() for line in linked_lines)
                wrapped_lines = self.wrapper.wrap(paragraph)
                result.extend(linked_lines if wrapped_lines is None else wrapped_lines)
                continue
            result.append(self.linker.cleanup_line(path, index + 1, lines[index]))
            index += 1
        updated_text = "\n".join(result)
        if had_final_newline or text == "":
            updated_text += "\n"
        return updated_text

    @classmethod
    def fix_title_lines(cls, lines: list[str]) -> list[str]:
        """Fix supported reStructuredText title adornment lengths."""
        result = list(lines)
        index = 0
        while index < len(result):
            if cls.is_title_adornment(result[index]):
                char = result[index][0]
                if (
                    index + 2 < len(result)
                    and result[index + 1].strip()
                    and not result[index + 1][0].isspace()
                    and cls.is_title_adornment(result[index + 2], char)
                ):
                    length = len(result[index + 1].strip())
                    result[index] = char * length
                    result[index + 2] = char * length
                    index += 3
                    continue
            if (
                index + 1 < len(result)
                and result[index].strip()
                and not result[index][0].isspace()
                and cls.is_title_adornment(result[index + 1])
            ):
                char = result[index + 1][0]
                result[index + 1] = char * len(result[index].strip())
                index += 2
                continue
            index += 1
        return result

    @classmethod
    def is_title_adornment(cls, line: str, expected_char: str | None = None) -> bool:
        """Test if a line is a supported title adornment."""
        if len(line) < 3 or line[0] not in cls.TITLE_CHARS:
            return False
        if expected_char is not None and line[0] != expected_char:
            return False
        return all(char == line[0] for char in line)

    @classmethod
    def code_block_lines(cls, lines: list[str]) -> set[int]:
        """Find indented lines that belong to explicit code blocks."""
        result: set[int] = set()
        in_code_block = False
        for index, line in enumerate(lines):
            stripped = line.strip()
            if in_code_block:
                if not stripped or line.startswith((" ", "\t")):
                    result.add(index)
                    continue
                in_code_block = False
            if re.match(r"^\.\.\s+(?:code-block|sourcecode)::", line):
                in_code_block = True
        return result

    @classmethod
    def is_simple_paragraph_line(cls, lines: list[str], index: int) -> bool:
        """Test if a line can participate in simple paragraph reflow."""
        line = lines[index]
        if not line or line[0].isspace() or cls.is_title_adornment(line):
            return False
        if index + 1 < len(lines) and cls.is_title_adornment(lines[index + 1]):
            return False
        if index > 0 and cls.is_title_adornment(lines[index - 1]):
            return False
        stripped = line.strip()
        if stripped.startswith((".. ", ".._", "::", "|", "+", ":")):
            return False
        if re.match(r"^(?:[*+-]|\d+\.|#\.)\s+", stripped):
            return False
        if stripped.startswith(".."):
            return False
        return True


class CleanupRstRunner:
    """Run cleanup-rst over a selected file set."""

    def __init__(
        self,
        config: CleanupRstConfig,
        *,
        dry_run: bool,
        recursive: bool,
        print_verbose: Callable[[str], None] | None = None,
    ) -> None:
        self.config = config
        self.dry_run = dry_run
        self.recursive = recursive
        self.print_verbose = print_verbose
        self.file_update = FileUpdate(print_verbose)
        self.index = DoxygenSymbolIndex.read(config.doxygen_index)
        self.cleaner = RstCleaner(config, self.index, self.warn)
        self.changed_files: list[Path] = []

    def warn(self, path: Path, line_number: int, message: str) -> None:
        """Print a cleanup warning."""
        print(f"warning: {self.display_path(path)}:{line_number}: {message}")

    def display_path(self, path: Path) -> str:
        """Create a project-relative path where possible."""
        try:
            return path.relative_to(self.config.project_dir).as_posix()
        except ValueError:
            return path.as_posix()

    def paths_for_target(self, target: Path) -> list[Path]:
        """Collect `.rst` files for one CLI target."""
        target = target.resolve(strict=False)
        project_dir = self.config.project_dir
        if target != project_dir and not target.is_relative_to(project_dir):
            raise UtilityError(f"Target escapes the project root: {target}")
        if target.is_file():
            if target.suffix != ".rst":
                raise UtilityError(f"Target file is not an .rst file: {target}")
            require_safe_existing_file(target, "reStructuredText file", FileUpdate.MAX_COMPARE_FILE_SIZE)
            return [target]
        require_directory(target, "Target Directory")
        iterator = target.rglob("*.rst") if self.recursive else target.glob("*.rst")
        paths = [path for path in iterator if path.is_file() and not path.is_symlink()]
        return sorted(paths, key=lambda path: path.as_posix().casefold())

    def process_file(self, path: Path) -> None:
        """Clean up one `.rst` file."""
        self.print_progress(f"Processing file: {self.display_path(path)}")
        original_text = read_safe_text(path, "reStructuredText file", FileUpdate.MAX_COMPARE_FILE_SIZE)
        updated_text = self.cleaner.cleanup_text(path, original_text)
        if updated_text == original_text:
            return
        self.changed_files.append(path)
        if self.dry_run:
            print(f"would update: {self.display_path(path)}")
        else:
            self.file_update.write_if_changed(path, updated_text)

    def run(self, target: Path) -> None:
        """Run the cleanup on all files selected by target."""
        for path in self.paths_for_target(target):
            self.process_file(path)
        if self.dry_run and not self.changed_files:
            print("No changes.")

    def print_progress(self, message: str) -> None:
        """Print a verbose progress message."""
        if self.print_verbose is not None:
            self.print_verbose(message)


class CleanupRstApp(UtilityApp):
    """Clean up reStructuredText documentation files."""

    description = "Normalize reStructuredText documentation files."

    def __init__(self) -> None:
        super().__init__()
        self.project_dir = Path()
        self.config: CleanupRstConfig | None = None
        self.dry_run = False
        self.recursive = False
        self.target = Path()

    def add_command_line_args(self, parser: argparse.ArgumentParser) -> None:
        parser.add_argument("-d", "--dry-run", action="store_true", help="Report changes without writing files.")
        parser.add_argument("-r", "--recursive", action="store_true", help="Process directories recursively.")
        parser.add_argument("target", type=Path, help="An .rst file or directory to process.")

    def handle_command_line_args(self, args: argparse.Namespace) -> None:
        self.project_dir = self.project_directory
        self.dry_run = args.dry_run
        self.recursive = args.recursive
        self.target = args.target

    def read_config(self) -> CleanupRstConfig:
        """Read the cleanup-rst configuration."""
        self.print_verbose("Reading the configuration")
        return CleanupRstConfig.read(self.project_dir, self.config_file_path())

    def run(self, argv=None) -> None:
        """Run this script."""
        super().run(argv)
        self.config = self.read_config()
        CleanupRstRunner(
            self.config,
            dry_run=self.dry_run,
            recursive=self.recursive,
            print_verbose=self.print_verbose,
        ).run(self.target)


def main() -> None:
    """Main entry point."""
    raise SystemExit(CleanupRstApp().main())


if __name__ == "__main__":
    main()
