# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from enum import Enum
from fnmatch import fnmatchcase
from pathlib import Path

from lib.config import project_relative_posix, read_elcl_file, validate_local_names, validate_source_relative_path
from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.path_safety import (
    read_safe_text,
    require_directory,
    require_safe_existing_file,
    require_safe_parent_directory,
    resolve_project_path,
)


class ApiEntryKind(Enum):
    """The supported kinds of namespace-scope API entries."""

    CLASS = "class"
    STRUCT = "struct"
    ENUM_CLASS = "enum_class"
    TYPEDEF = "typedef"
    FUNCTION = "function"


@dataclass(frozen=True)
class ApiEntry:
    """One documented namespace-scope API entry."""

    kind: ApiEntryKind
    name: str
    namespace: tuple[str, ...]
    signature: str = ""

    @property
    def full_name(self) -> str:
        """Get the fully qualified C++ name."""
        result = "::".join((*self.namespace, self.name))
        if self.kind == ApiEntryKind.FUNCTION:
            result += self.signature
        return result


@dataclass(frozen=True)
class ReferenceGroup:
    """One configured reference documentation group."""

    page_path: Path
    relative_page_path: Path
    title: str
    header_paths: tuple[Path, ...] = ()
    header_globs: tuple[str, ...] = ()
    excluded_header_globs: tuple[str, ...] = ()

    @property
    def page_entry(self) -> str:
        """Get the toctree entry for this group page."""
        return self.relative_page_path.with_suffix("").as_posix()


@dataclass(frozen=True)
class HeaderApi:
    """The documentation-relevant API extracted from one header."""

    path: Path
    relative_path: Path
    page_path: Path | None
    entries: tuple[ApiEntry, ...]
    group: ReferenceGroup | None = None


@dataclass(frozen=True)
class NamespaceBlock:
    """One tracked C++ namespace block."""

    parts: tuple[str, ...]
    close_depth: int


@dataclass(frozen=True)
class ReferenceDocConfig:
    """Configuration for reference documentation utilities."""

    project_dir: Path
    source_dir: Path
    reference_dir: Path
    excluded_directory_names: frozenset[str]
    excluded_header_names: frozenset[str]
    excluded_header_globs: tuple[str, ...]
    manual_page_relative_paths: frozenset[Path]
    exclude_underscore_headers: bool
    reference_groups: tuple[ReferenceGroup, ...] = ()

    @classmethod
    def read(cls, project_dir: Path, config_file: Path) -> "ReferenceDocConfig":
        """Read the reference documentation configuration."""
        project_dir = project_dir.resolve()
        config = read_elcl_file(config_file)
        main_config = config["main"]
        source_dir = resolve_project_path(project_dir, main_config.get_text("source_directory"), "Source Directory")
        reference_dir = resolve_project_path(
            project_dir, main_config.get_text("reference_directory"), "Reference Directory"
        )
        groups = cls.read_reference_groups(config, reference_dir)
        result = cls(
            project_dir=project_dir,
            source_dir=source_dir,
            reference_dir=reference_dir,
            excluded_directory_names=frozenset(main_config.get_list("excluded_directory_names", str, default=[])),
            excluded_header_names=frozenset(main_config.get_list("excluded_header_names", str, default=[])),
            excluded_header_globs=tuple(main_config.get_list("excluded_header_globs", str, default=[])),
            manual_page_relative_paths=frozenset(
                Path(path) for path in main_config.get_list("manual_reference_pages", str, default=[])
            ),
            exclude_underscore_headers=main_config.get_bool("exclude_underscore_headers", default=True),
            reference_groups=groups,
        )
        result.validate()
        return result.with_expanded_group_headers()

    @classmethod
    def read_reference_groups(cls, config, reference_dir: Path) -> tuple[ReferenceGroup, ...]:
        """Read all optional reference groups from the configuration."""
        result = []
        for index, group_config in enumerate(config.get("reference_groups", []), start=1):
            label = f"Reference Groups entry {index}"
            page_text = group_config.get_text("page")
            validate_source_relative_path(page_text, f"{label} Page")
            relative_page_path = Path(page_text)
            if relative_page_path.suffix != ".rst":
                raise UtilityError(f"{label} Page must be an .rst file: {page_text}")
            if relative_page_path.name == "index.rst":
                raise UtilityError(f"{label} Page must not be an index page: {page_text}")
            page_path = reference_dir / relative_page_path
            title = group_config.get_text("title", default="")
            header_paths = []
            for header_text in group_config.get_list("headers", str, default=[]):
                validate_source_relative_path(header_text, f"{label} Headers")
                header_path = Path(header_text)
                if header_path.suffix != ".hpp":
                    raise UtilityError(f"{label} Headers entries must be .hpp files: {header_text}")
                header_paths.append(header_path)
            header_globs = tuple(group_config.get_list("header_globs", str, default=[]))
            excluded_header_globs = tuple(group_config.get_list("excluded_header_globs", str, default=[]))
            if not header_paths and not header_globs:
                raise UtilityError(f"{label} must list at least one header or header glob.")
            result.append(
                ReferenceGroup(
                    page_path=page_path,
                    relative_page_path=relative_page_path,
                    title=title,
                    header_paths=tuple(header_paths),
                    header_globs=header_globs,
                    excluded_header_globs=excluded_header_globs,
                )
            )
        return tuple(result)

    def validate(self) -> None:
        """Validate all configured paths and names."""
        require_directory(self.source_dir, "Source Directory")
        if self.reference_dir.exists():
            require_directory(self.reference_dir, "Reference Directory")
        else:
            require_safe_parent_directory(self.reference_dir, "Reference Directory")
        validate_local_names(self.excluded_directory_names, "Excluded Directory Names")
        validate_local_names(self.excluded_header_names, "Excluded Header Names")
        for name in self.excluded_header_names:
            if Path(name).suffix != ".hpp":
                raise UtilityError(f"Excluded Header Names entries must be .hpp files: {name}")
        for pattern in self.excluded_header_globs:
            self.validate_header_glob(pattern, "Excluded Header Globs")
        for path in self.manual_page_relative_paths:
            path_text = path.as_posix()
            validate_source_relative_path(path_text, "Manual Reference Pages")
            if path.suffix != ".rst" or path.name == "index.rst":
                raise UtilityError(f"Manual Reference Pages entries must be non-index .rst files: {path_text}")
        self.validate_reference_groups()

    @staticmethod
    def validate_header_glob(pattern: str, label: str) -> None:
        """Validate one source-relative header glob."""
        validate_source_relative_path(pattern, label)
        if not pattern.endswith(".hpp"):
            raise UtilityError(f"{label} entries must match .hpp files: {pattern}")

    def validate_reference_groups(self) -> None:
        """Validate configured reference groups."""
        pages: dict[Path, ReferenceGroup] = {}
        headers: dict[Path, ReferenceGroup] = {}
        for group in self.reference_groups:
            if group.relative_page_path in pages:
                raise UtilityError(f"Duplicate reference group page: {group.relative_page_path.as_posix()}")
            pages[group.relative_page_path] = group
            require_safe_parent_directory(group.page_path, "Reference Group Page")
            for pattern in (*group.header_globs, *group.excluded_header_globs):
                self.validate_header_glob(pattern, "Reference Group Header Globs")
            for header_path in group.header_paths:
                if header_path in headers:
                    raise UtilityError(f"Duplicate reference group header: {header_path.as_posix()}")
                if not (self.source_dir / header_path).exists():
                    raise UtilityError(f"Reference group header does not exist: {header_path.as_posix()}")
                if self.should_skip_header(self.source_dir / header_path):
                    raise UtilityError(
                        f"Reference group header is excluded from public API scanning: {header_path.as_posix()}"
                    )
                headers[header_path] = group

    def with_expanded_group_headers(self) -> "ReferenceDocConfig":
        """Expand configured header globs into their exact matched header paths."""
        groups = []
        for group in self.reference_groups:
            paths = set(group.header_paths)
            for pattern in group.header_globs:
                paths.update(
                    path.relative_to(self.source_dir)
                    for path in self.source_dir.glob(pattern)
                    if path.is_file() and path.suffix == ".hpp"
                )
            paths = {
                path
                for path in paths
                if not any(fnmatchcase(path.as_posix(), pattern) for pattern in group.excluded_header_globs)
                and not self.should_skip_header(self.source_dir / path)
            }
            if not paths:
                raise UtilityError(f"Reference group has no matching headers: {group.relative_page_path.as_posix()}")
            groups.append(
                ReferenceGroup(
                    page_path=group.page_path,
                    relative_page_path=group.relative_page_path,
                    title=group.title,
                    header_paths=tuple(sorted(paths, key=lambda path: path.as_posix().casefold())),
                    header_globs=group.header_globs,
                    excluded_header_globs=group.excluded_header_globs,
                )
            )
        result = ReferenceDocConfig(
            project_dir=self.project_dir,
            source_dir=self.source_dir,
            reference_dir=self.reference_dir,
            excluded_directory_names=self.excluded_directory_names,
            excluded_header_names=self.excluded_header_names,
            excluded_header_globs=self.excluded_header_globs,
            manual_page_relative_paths=self.manual_page_relative_paths,
            exclude_underscore_headers=self.exclude_underscore_headers,
            reference_groups=tuple(groups),
        )
        result.validate_reference_groups()
        return result

    def display_path(self, path: Path) -> str:
        """Create a stable project-relative display path."""
        return project_relative_posix(self.project_dir, path)

    def should_skip_header(self, path: Path) -> bool:
        """Test if a header file is excluded by the configuration."""
        relative_path = path.relative_to(self.source_dir)
        if any(part in self.excluded_directory_names for part in relative_path.parts[:-1]):
            return True
        if path.name in self.excluded_header_names:
            return True
        if any(fnmatchcase(relative_path.as_posix(), pattern) for pattern in self.excluded_header_globs):
            return True
        return self.exclude_underscore_headers and "_" in path.name

    def reference_group_for_header(self, relative_path: Path) -> ReferenceGroup | None:
        """Find the configured reference group for a source-relative header."""
        for group in self.reference_groups:
            if relative_path in group.header_paths:
                return group
        return None

    def manual_reference_page_paths(self) -> set[Path]:
        """Get all manually maintained reference page paths."""
        return {self.reference_dir / path for path in self.manual_page_relative_paths}


class HeaderScanner:
    """Extract documented namespace-scope API declarations from a C++ header."""

    RE_NAMESPACE_BLOCK = re.compile(r"^namespace\s+(?P<name>[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*\{\s*(?://.*)?$")
    RE_EMPTY_NAMESPACE = re.compile(r"^namespace\s+[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*\s*\{\s*\}\s*(?://.*)?$")
    RE_NAMESPACE_ALIAS = re.compile(r"^namespace\s+[A-Za-z_]\w*\s*=\s*[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*\s*;")
    RE_CLASS_OR_STRUCT = re.compile(r"^(?P<kind>class|struct)\s+(?P<name>[A-Za-z_]\w*)\b")
    RE_ENUM_CLASS = re.compile(r"^enum\s+class\s+(?P<name>[A-Za-z_]\w*)\b")
    RE_USING = re.compile(r"^using\s+(?P<name>[A-Za-z_]\w*)\s*=")
    RE_UNSUPPORTED_DOCUMENTED_TYPE = re.compile(r"^(union|concept|typedef)\b")
    RE_FUNCTION = re.compile(
        r"^(?:\[\[[^\]]+\]\]\s*)*(?:(?:inline|constexpr)\s+)*(?:auto|void)\s+"
        r"(?P<name>[A-Za-z_]\w*|operator\"\"_[A-Za-z_]\w*)\s*(?P<signature>\(.*)$"
    )
    RE_FUNCTION_RETURN_PREFIX = re.compile(r"^(?:\[\[[^\]]+\]\]\s*)*(?:(?:inline|constexpr)\s+)*(?:auto|void)\s*$")
    RE_FUNCTION_CONTINUATION = re.compile(r"^(?P<name>[A-Za-z_]\w*|operator\"\"_[A-Za-z_]\w*)\s*(?P<signature>\(.*)$")

    def __init__(self, config: ReferenceDocConfig) -> None:
        self.config = config

    def scan(self, path: Path) -> HeaderApi:
        """Scan one header file."""
        require_safe_existing_file(path, "Header file", FileUpdate.MAX_COMPARE_FILE_SIZE)
        text = read_safe_text(path, "Header file", FileUpdate.MAX_COMPARE_FILE_SIZE)
        entries = self.scan_text(text, path)
        relative_path = path.relative_to(self.config.source_dir)
        group = self.config.reference_group_for_header(relative_path)
        return HeaderApi(
            path=path,
            relative_path=relative_path,
            page_path=group.page_path if group is not None else None,
            entries=tuple(entries),
            group=group,
        )

    def scan_text(self, text: str, path: Path) -> list[ApiEntry]:
        """Scan header text for documented namespace-scope API entries."""
        entries: list[ApiEntry] = []
        namespace_stack: list[NamespaceBlock] = []
        brace_depth = 0
        pending_api_doc = False
        pending_function_return = False
        pending_function_entry: ApiEntry | None = None

        for line_number, line in enumerate(text.splitlines(), start=1):
            if pending_function_entry is not None:
                pending_function_entry = self.extend_function_entry(pending_function_entry, line)
                if is_function_signature_complete(
                    pending_function_entry.signature
                ) and has_function_declaration_terminator(line):
                    entries.append(pending_function_entry)
                    pending_function_entry = None
                brace_depth = self.updated_brace_depth(brace_depth, line, path, line_number)
                self.close_namespaces(namespace_stack, brace_depth)
                continue
            if line.startswith("///"):
                pending_api_doc = True
                pending_function_return = False
                continue
            if line.startswith("namespace "):
                self.process_namespace_line(line, brace_depth, namespace_stack, path, line_number)
            if pending_api_doc and pending_function_return:
                entry = self.parse_function_continuation(
                    line, self.current_namespace(namespace_stack), path, line_number
                )
                if entry is not None:
                    if is_function_signature_complete(entry.signature) and has_function_declaration_terminator(line):
                        entries.append(entry)
                    else:
                        pending_function_entry = entry
                    pending_api_doc = False
                    pending_function_return = False
                else:
                    pending_function_return = False
            if pending_api_doc and self.is_api_doc_preamble(line):
                brace_depth = self.updated_brace_depth(brace_depth, line, path, line_number)
                self.close_namespaces(namespace_stack, brace_depth)
                continue
            if pending_api_doc and self.is_function_return_prefix(line):
                pending_function_return = True
                brace_depth = self.updated_brace_depth(brace_depth, line, path, line_number)
                self.close_namespaces(namespace_stack, brace_depth)
                continue
            if pending_api_doc:
                entry = self.parse_documented_entry(line, self.current_namespace(namespace_stack), path, line_number)
                if entry is not None:
                    if entry.kind == ApiEntryKind.FUNCTION and (
                        not is_function_signature_complete(entry.signature)
                        or not has_function_declaration_terminator(line)
                    ):
                        pending_function_entry = entry
                    else:
                        entries.append(entry)
                pending_api_doc = False

            brace_depth = self.updated_brace_depth(brace_depth, line, path, line_number)
            self.close_namespaces(namespace_stack, brace_depth)

        if namespace_stack:
            raise UtilityError(f"Unclosed namespace block in {self.config.display_path(path)}.")
        return entries

    def process_namespace_line(
        self, line: str, brace_depth: int, namespace_stack: list[NamespaceBlock], path: Path, line_number: int
    ) -> None:
        """Track supported namespace block lines."""
        if self.RE_EMPTY_NAMESPACE.fullmatch(line) or self.RE_NAMESPACE_ALIAS.match(line):
            return
        match = self.RE_NAMESPACE_BLOCK.fullmatch(line)
        if match is None:
            location = f"{self.config.display_path(path)}:{line_number}"
            raise UtilityError(f"Unsupported namespace declaration at {location}: {line}")
        namespace_stack.append(NamespaceBlock(tuple(match.group("name").split("::")), brace_depth + 1))

    @staticmethod
    def is_api_doc_preamble(line: str) -> bool:
        """Test if a line can stand between an API doc block and the declaration."""
        return (
            line.startswith("template ")
            or line.startswith("requires ")
            or (line.startswith("    ") and line.strip())
            or re.fullmatch(r"\[\[[^\]]+\]\]\s*", line) is not None
        )

    @classmethod
    def is_function_return_prefix(cls, line: str) -> bool:
        """Test if a line starts a function declaration split after its return token."""
        return cls.RE_FUNCTION_RETURN_PREFIX.fullmatch(line) is not None

    def parse_documented_entry(
        self, line: str, namespace: tuple[str, ...], path: Path, line_number: int
    ) -> ApiEntry | None:
        """Parse one documented declaration line."""
        if not line or line[0].isspace():
            return None
        match = self.RE_CLASS_OR_STRUCT.match(line)
        if match is not None:
            kind = ApiEntryKind.CLASS if match.group("kind") == "class" else ApiEntryKind.STRUCT
            return self.create_entry(kind, match.group("name"), namespace, path, line_number)
        match = self.RE_ENUM_CLASS.match(line)
        if match is not None:
            return self.create_entry(ApiEntryKind.ENUM_CLASS, match.group("name"), namespace, path, line_number)
        match = self.RE_USING.match(line)
        if match is not None:
            return self.create_entry(ApiEntryKind.TYPEDEF, match.group("name"), namespace, path, line_number)
        match = self.RE_FUNCTION.match(line)
        if match is not None:
            return self.create_function_entry(
                match.group("name"), match.group("signature"), namespace, path, line_number
            )
        if self.RE_UNSUPPORTED_DOCUMENTED_TYPE.match(line):
            location = f"{self.config.display_path(path)}:{line_number}"
            raise UtilityError(f"Unsupported documented API declaration at {location}: {line}")
        return None

    def parse_function_continuation(
        self, line: str, namespace: tuple[str, ...], path: Path, line_number: int
    ) -> ApiEntry | None:
        """Parse the name and signature after a split function return token."""
        if not line or line[0].isspace():
            return None
        match = self.RE_FUNCTION_CONTINUATION.match(line)
        if match is None:
            return None
        return self.create_function_entry(match.group("name"), match.group("signature"), namespace, path, line_number)

    def create_entry(
        self, kind: ApiEntryKind, name: str, namespace: tuple[str, ...], path: Path, line_number: int
    ) -> ApiEntry:
        """Create an API entry and validate that it is inside a namespace."""
        if not namespace:
            location = f"{self.config.display_path(path)}:{line_number}"
            raise UtilityError(f"Documented API declaration outside a namespace at {location}.")
        return ApiEntry(kind, name, namespace)

    def create_function_entry(
        self, name: str, signature: str, namespace: tuple[str, ...], path: Path, line_number: int
    ) -> ApiEntry:
        """Create a documented namespace-scope function entry."""
        entry = self.create_entry(ApiEntryKind.FUNCTION, name, namespace, path, line_number)
        return ApiEntry(entry.kind, entry.name, entry.namespace, clean_function_signature(signature))

    @staticmethod
    def extend_function_entry(entry: ApiEntry, line: str) -> ApiEntry:
        """Append a continued signature line to a pending function entry."""
        return ApiEntry(
            entry.kind, entry.name, entry.namespace, clean_function_signature(f"{entry.signature} {line.strip()}")
        )

    @staticmethod
    def current_namespace(namespace_stack: list[NamespaceBlock]) -> tuple[str, ...]:
        """Build the current namespace from all open namespace blocks."""
        return tuple(part for namespace in namespace_stack for part in namespace.parts)

    @classmethod
    def updated_brace_depth(cls, brace_depth: int, line: str, path: Path, line_number: int) -> int:
        """Update and validate the simple brace depth used for namespace tracking."""
        code = strip_line_comment_and_literals(line)
        brace_depth += code.count("{")
        brace_depth -= code.count("}")
        if brace_depth < 0:
            raise UtilityError(f"Unmatched closing brace in {path}:{line_number}.")
        return brace_depth

    @staticmethod
    def close_namespaces(namespace_stack: list[NamespaceBlock], brace_depth: int) -> None:
        """Close namespace blocks whose brace depth ended."""
        while namespace_stack and brace_depth < namespace_stack[-1].close_depth:
            namespace_stack.pop()


class ReferenceDocGenerator:
    """Generate and update reference documentation files."""

    INTERFACE_TITLE = "Interface"
    INTERFACE_UNDERLINE = "========="

    def __init__(self, config: ReferenceDocConfig, print_verbose=None) -> None:
        self.config = config
        self.print_verbose = print_verbose
        self.file_update = FileUpdate(print_verbose)
        self.scanner = HeaderScanner(config)

    def collect_headers(self) -> list[HeaderApi]:
        """Collect and scan all configured public headers."""
        headers = []
        for path in sorted(self.config.source_dir.rglob("*.hpp"), key=lambda item: item.as_posix().casefold()):
            require_safe_existing_file(path, "Header file", FileUpdate.MAX_COMPARE_FILE_SIZE)
            if self.config.should_skip_header(path):
                self.print_progress(f"Skipping header: {self.config.display_path(path)}")
                continue
            self.print_progress(f"Scanning header: {self.config.display_path(path)}")
            headers.append(self.scanner.scan(path))
        return headers

    def update_reference_pages(self, headers: list[HeaderApi]) -> None:
        """Create or update reference pages for all headers."""
        for page_path, page_headers in self.headers_by_page(headers).items():
            interface_text = self.interface_for_headers(page_headers)
            if page_path.exists():
                text = read_safe_text(page_path, "Reference page", FileUpdate.MAX_COMPARE_FILE_SIZE)
                updated_text = self.replace_interface_body(page_path, text, interface_text)
            else:
                updated_text = self.create_reference_page(page_headers, interface_text)
            self.print_progress(f"Updating page: {self.config.display_path(page_path)}")
            self.file_update.write_if_changed(page_path, updated_text)

    def headers_by_page(self, headers: list[HeaderApi]) -> dict[Path, list[HeaderApi]]:
        """Group headers by their effective reference page."""
        result: dict[Path, list[HeaderApi]] = {}
        for header in headers:
            if header.page_path is not None:
                result.setdefault(header.page_path, []).append(header)
        return {
            page: sorted(page_headers, key=lambda item: item.relative_path.as_posix().casefold())
            for page, page_headers in sorted(result.items(), key=lambda item: item[0].as_posix().casefold())
        }

    def update_orphan_pages(self, headers: list[HeaderApi]) -> None:
        """Delete unmanaged reference pages after their content was migrated."""
        expected_pages = {header.page_path for header in headers if header.page_path is not None}
        expected_pages.update(self.config.manual_reference_page_paths())
        if not self.config.reference_dir.exists():
            return
        for path in sorted(self.config.reference_dir.rglob("*.rst"), key=lambda item: item.as_posix().casefold()):
            if path.name == "index.rst" or path in expected_pages:
                continue
            require_safe_existing_file(path, "Reference page", FileUpdate.MAX_COMPARE_FILE_SIZE)
            if path.is_symlink():
                raise UtilityError(f"Refusing to remove symbolic link: {self.config.display_path(path)}")
            self.print_progress(f"Removing orphan page: {self.config.display_path(path)}")
            path.unlink()

    def update_indexes(self, headers: list[HeaderApi]) -> None:
        """Create or update all reference index files."""
        index_entries = self.index_entries(headers)
        for directory, entries in sorted(index_entries.items(), key=lambda item: item[0].as_posix().casefold()):
            path = directory / "index.rst"
            if path.exists():
                text = read_safe_text(path, "Reference index page", FileUpdate.MAX_COMPARE_FILE_SIZE)
            else:
                text = self.create_index_page(directory)
            updated_text = self.replace_toctree_entries(path, text, entries)
            self.print_progress(f"Updating index: {self.config.display_path(path)}")
            self.file_update.write_if_changed(path, updated_text)

    def index_entries(self, headers: list[HeaderApi]) -> dict[Path, list[str]]:
        """Build sorted toctree entries for root and namespace index pages."""
        result: dict[Path, set[str]] = {self.config.reference_dir: set()}
        page_paths = {header.page_path for header in headers if header.page_path is not None}
        page_paths.update(self.config.manual_reference_page_paths())
        for page_path in page_paths:
            parent = page_path.parent
            result.setdefault(parent, set()).add(page_path.stem)
            relative_parent = parent.relative_to(self.config.reference_dir)
            if relative_parent == Path("."):
                continue
            current = self.config.reference_dir
            for part in relative_parent.parts:
                next_dir = current / part
                result.setdefault(current, set()).add(f"{part}/index")
                result.setdefault(next_dir, set())
                current = next_dir
        return {directory: sorted(entries, key=str.casefold) for directory, entries in result.items()}

    def interface_for_headers(self, headers: list[HeaderApi]) -> str:
        """Create the managed Interface section body for one or more headers."""
        entries = [self.interface_for_header(header).rstrip() for header in headers]
        return "\n".join(entry for entry in entries if entry).rstrip() + "\n"

    def interface_for_header(self, header: HeaderApi) -> str:
        """Create the managed Interface section body for one header."""
        if not header.entries:
            return ""
        result = []
        for entry in header.entries:
            result.append(self.interface_for_entry(entry))
        return "\n".join(result).rstrip() + "\n"

    @staticmethod
    def interface_for_entry(entry: ApiEntry) -> str:
        """Create one Breathe directive for an API entry."""
        if entry.kind == ApiEntryKind.CLASS:
            return f".. doxygenclass:: {entry.full_name}\n    :members:\n"
        if entry.kind == ApiEntryKind.STRUCT:
            return f".. doxygenstruct:: {entry.full_name}\n    :members:\n"
        if entry.kind == ApiEntryKind.ENUM_CLASS:
            return f".. doxygenenum:: {entry.full_name}\n"
        if entry.kind == ApiEntryKind.TYPEDEF:
            return f".. doxygentypedef:: {entry.full_name}\n"
        if entry.kind == ApiEntryKind.FUNCTION:
            return f".. doxygenfunction:: {entry.full_name}\n"
        raise UtilityError(f"Unsupported API entry kind: {entry.kind}")

    def create_reference_page(self, headers: list[HeaderApi] | HeaderApi, interface_text: str) -> str:
        """Create a new reference page template."""
        if isinstance(headers, HeaderApi):
            headers = [headers]
        title = self.reference_page_title(headers)
        underline = "*" * len(title)
        page = (
            ".. index::\n"
            f"    single: {title}\n"
            "\n"
            f"{underline}\n"
            f"{title}\n"
            f"{underline}\n"
            "\n"
            "Interface\n"
            "=========\n"
        )
        interface_text = interface_text.rstrip()
        return f"{page}\n{interface_text}\n" if interface_text else page

    def reference_page_title(self, headers: list[HeaderApi]) -> str:
        """Create the title for a generated reference page."""
        group = headers[0].group
        if group is not None:
            if group.title:
                return group.title
            return split_identifier_words(group.relative_page_path.stem)
        return split_identifier_words(headers[0].relative_path.stem)

    def create_index_page(self, directory: Path) -> str:
        """Create a new reference index page."""
        title = self.index_title(directory)
        underline = "*" * len(title)
        return f"{underline}\n{title}\n{underline}\n\n.. toctree::\n    :maxdepth: 1\n\n"

    def index_title(self, directory: Path) -> str:
        """Create a readable title for a new index page."""
        if directory == self.config.reference_dir:
            return "Reference"
        return f"{split_identifier_words(directory.name)} Reference"

    def replace_interface_body(self, path: Path, text: str, interface_text: str) -> str:
        """Replace only the body of the final Interface section."""
        lines = text.splitlines()
        interface_indices = [
            index
            for index in range(len(lines) - 1)
            if lines[index] == self.INTERFACE_TITLE and lines[index + 1] == self.INTERFACE_UNDERLINE
        ]
        if not interface_indices:
            raise UtilityError(f"Reference page has no Interface section: {self.config.display_path(path)}")
        if len(interface_indices) > 1:
            raise UtilityError(f"Reference page has multiple Interface sections: {self.config.display_path(path)}")
        index = interface_indices[0]
        prefix = lines[: index + 2]
        interface_text = interface_text.rstrip()
        if not interface_text:
            return "\n".join(prefix).rstrip() + "\n"
        return "\n".join(prefix).rstrip() + "\n\n" + interface_text + "\n"

    def replace_toctree_entries(self, path: Path, text: str, entries: list[str]) -> str:
        """Replace entries in the single toctree of an index page."""
        lines = text.splitlines()
        toctree_indices = [index for index, line in enumerate(lines) if line == ".. toctree::"]
        if not toctree_indices:
            raise UtilityError(f"Reference index page has no toctree: {self.config.display_path(path)}")
        if len(toctree_indices) > 1:
            raise UtilityError(f"Reference index page has multiple toctrees: {self.config.display_path(path)}")

        index = toctree_indices[0]
        block_end = index + 1
        while block_end < len(lines) and (not lines[block_end] or lines[block_end].startswith(" ")):
            block_end += 1

        option_end = index + 1
        while option_end < block_end:
            line = lines[option_end]
            if not line:
                option_end += 1
                continue
            if line.startswith(" ") and line.strip().startswith(":"):
                option_end += 1
                continue
            break

        option_lines = lines[index + 1 : option_end]
        while option_lines and option_lines[-1] == "":
            option_lines.pop()

        result = lines[: index + 1]
        result.extend(option_lines)
        result.append("")
        result.extend(f"    {entry}" for entry in sorted(entries, key=str.casefold))
        result.extend(lines[block_end:])
        return "\n".join(result).rstrip() + "\n"

    def uncategorized_headers(self, headers: list[HeaderApi]) -> list[HeaderApi]:
        """Return public headers that are not assigned to a reference group."""
        return [header for header in headers if header.group is None]

    def warn_uncategorized_headers(self, headers: list[HeaderApi]) -> None:
        """Print a warning for public headers that are not assigned to a reference group."""
        uncategorized = self.uncategorized_headers(headers)
        if not uncategorized:
            return
        print(
            f"Warning: {len(uncategorized)} public API header(s) are not assigned to reference groups.",
            file=sys.stderr,
        )
        if self.print_verbose is None:
            return
        for header in uncategorized:
            self.print_progress(f"Uncategorized header: {header.relative_path.as_posix()}")

    def print_progress(self, message: str) -> None:
        """Print a verbose progress message."""
        if self.print_verbose is not None:
            self.print_verbose(message)


def camel_to_snake(text: str) -> str:
    """Convert a C++ header stem to a snake-case reference page stem."""
    text = re.sub(r"(.)([A-Z][a-z]+)", r"\1_\2", text)
    text = re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", text)
    return text.replace("-", "_").lower()


def split_identifier_words(text: str) -> str:
    """Create a readable title from a C++ identifier or file stem."""
    return " ".join(part.capitalize() for part in camel_to_snake(text).split("_"))


def strip_line_comment_and_literals(line: str) -> str:
    """Remove line comments and simple literals before brace counting."""
    code = line.split("//", 1)[0]
    code = re.sub(r'"(?:\\.|[^"\\])*"', '""', code)
    return re.sub(r"'(?:\\.|[^'\\])*'", "''", code)


def clean_function_signature(signature: str) -> str:
    """Keep only the function signature text needed by Breathe."""
    signature = signature.split("//", 1)[0].strip()
    signature = signature.split("{", 1)[0].strip()
    signature = signature.split(";", 1)[0].strip()
    signature = re.sub(r"\(\s+", "(", signature)
    signature = re.sub(r"\s+\)", ")", signature)
    return signature


def is_function_signature_complete(signature: str) -> bool:
    """Test if a function signature includes a complete parameter list."""
    return signature.count("(") > 0 and signature.count("(") <= signature.count(")")


def has_function_declaration_terminator(line: str) -> bool:
    """Test if a declaration line reaches its semicolon or body."""
    code = line.split("//", 1)[0]
    return ";" in code or "{" in code
