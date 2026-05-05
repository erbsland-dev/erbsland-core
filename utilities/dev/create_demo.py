# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import TextIO

from lib.error import UtilityError
from lib.file_lock import ProjectFileLock
from lib.file_update import write_text_atomically
from lib.path_safety import require_safe_parent_directory
from lib.utility import UtilityApp

CMAKE_HEADER = """# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)
"""
CXX_HEADER = """// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
"""
DOMAIN_RE = re.compile(r"^[_a-z]+$")
DEMO_NAME_RE = re.compile(r"^[A-Z][a-zA-Z0-9]*$")
RE_ADD_SUBDIRECTORY = re.compile(r"(?m)^add_subdirectory\(\s*(?P<name>[-_a-zA-Z0-9.]+)\s*\)\s*$")
RE_DEMO_TARGET = re.compile(r"(?m)^erbsland_core_add_demo\(\s*(?P<target>[-_a-zA-Z0-9_]+)\s*\)\s*$")
RE_TARGET_SOURCES = re.compile(r"(?ms)^target_sources\(\s*(?P<target>[-_a-zA-Z0-9_]+)\s+PRIVATE\s*(?P<content>.*?)\)")
RE_DEMO_DECLARATION = re.compile(r"^\s*void\s+(?P<name>[A-Za-z][A-Za-z0-9]*)\(\);\s*$")
RE_DEMO_REGISTRATION = re.compile(
    r'^\s*app\.registerDemo\("(?P<name>[A-Za-z][A-Za-z0-9]*)"_el,\s*(?P<function>[A-Za-z][A-Za-z0-9]*)\);\s*$'
)


@dataclass(frozen=True)
class DemoPath:
    """The validated demo path passed on the command line."""

    domain: str
    demo_name: str
    demo_part: str

    @classmethod
    def parse(cls, value: str) -> "DemoPath":
        """Parse and validate a demo path."""
        path = Path(value)
        if path.is_absolute():
            raise UtilityError(f"Demo path must be relative: {value}")
        parts = path.parts
        if len(parts) != 3:
            raise UtilityError("Demo path must have exactly three parts: <domain>/<DemoName>/<DemoPart>.")
        if any(part in {"", ".", ".."} for part in parts):
            raise UtilityError(f"Demo path contains an invalid part: {value}")
        domain, demo_name, demo_part = parts
        if DOMAIN_RE.fullmatch(domain) is None:
            raise UtilityError(f"Invalid domain name: {domain!r}. Expected [_a-z]+.")
        if DEMO_NAME_RE.fullmatch(demo_name) is None:
            raise UtilityError(f"Invalid demo name: {demo_name!r}. Expected [A-Z][a-zA-Z0-9]*.")
        if DEMO_NAME_RE.fullmatch(demo_part) is None:
            raise UtilityError(f"Invalid demo part name: {demo_part!r}. Expected [A-Z][a-zA-Z0-9]*.")
        return cls(domain, demo_name, demo_part)

    @property
    def function_name(self) -> str:
        """Get the function name for the demo part."""
        return self.demo_part[:1].lower() + self.demo_part[1:]

    @property
    def header_name(self) -> str:
        """Get the generated demo declaration header name."""
        return f"{self.demo_name}Demos.hpp"

    @property
    def part_source_name(self) -> str:
        """Get the source file name for the demo part."""
        return f"{self.demo_part}.cpp"

    def source_option(self) -> str:
        """Get the documentation source path."""
        return f"{self.domain}/{self.demo_name}/{self.part_source_name}"


@dataclass(frozen=True)
class PlannedFile:
    """A file to create or update."""

    path: Path
    content: str
    action: str


@dataclass(frozen=True)
class CreateDemoPlan:
    """The complete non-mutating plan for creating or extending a demo."""

    demo_path: DemoPath
    target_name: str
    files: tuple[PlannedFile, ...]
    directories: tuple[Path, ...]
    warnings: tuple[str, ...]
    project_dir: Path

    def display_path(self, path: Path) -> str:
        """Convert a path to a project-relative display path where possible."""
        try:
            return path.relative_to(self.project_dir).as_posix()
        except ValueError:
            return path.as_posix()

    def print_summary(self, output: TextIO) -> None:
        """Print the planned changes."""
        print(f"Demo: {self.demo_path.domain}/{self.demo_path.demo_name}/{self.demo_path.demo_part}", file=output)
        print(f"Executable: {self.target_name}", file=output)
        print("", file=output)
        if self.directories:
            print("Directories to create:", file=output)
            for directory in self.directories:
                print(f"  - {self.display_path(directory)}/", file=output)
        files_to_create = [planned_file for planned_file in self.files if planned_file.action == "create"]
        files_to_update = [planned_file for planned_file in self.files if planned_file.action == "update"]
        if files_to_create:
            print("Files to create:", file=output)
            for planned_file in files_to_create:
                print(f"  - {self.display_path(planned_file.path)}", file=output)
        if files_to_update:
            print("Files to update:", file=output)
            for planned_file in files_to_update:
                print(f"  - {self.display_path(planned_file.path)}", file=output)
        if self.warnings:
            print("Warnings:", file=output)
            for warning in self.warnings:
                print(f"  - {warning}", file=output)
        if not self.directories and not self.files:
            print("No changes are required.", file=output)

    def print_documentation_block(self, output: TextIO) -> None:
        """Print the documentation block for the new demo."""
        print("", file=output)
        print("Documentation block:", file=output)
        print(".. erbsland-demo::", file=output)
        print(f"    :source: {self.demo_path.source_option()}", file=output)
        print(f"    :exec: {self.target_name} --demo {self.demo_path.demo_part}", file=output)
        print("", file=output)
        print(".. erbsland-demo-end::", file=output)

    def apply(self) -> None:
        """Apply all planned file changes."""
        for directory in self.directories:
            require_safe_parent_directory(directory, "Demo directory")
            directory.mkdir(parents=True, exist_ok=True)
        for planned_file in self.files:
            require_safe_parent_directory(planned_file.path, "Demo file")
            planned_file.path.parent.mkdir(parents=True, exist_ok=True)
            write_text_atomically(planned_file.path, planned_file.content)


class CreateDemoPlanner:
    """Build a create-demo plan without writing files."""

    def __init__(self, project_dir: Path, demo_path: DemoPath) -> None:
        self.project_dir = project_dir
        self.demo_path = demo_path
        self.demos_dir = self.project_dir / "demos"
        self.domain_dir = self.demos_dir / demo_path.domain
        self.demo_dir = self.domain_dir / demo_path.demo_name
        self.root_cmake_path = self.demos_dir / "CMakeLists.txt"
        self.domain_cmake_path = self.domain_dir / "CMakeLists.txt"
        self.demo_cmake_path = self.demo_dir / "CMakeLists.txt"
        self.main_path = self.demo_dir / "main.cpp"
        self.header_path = self.demo_dir / demo_path.header_name
        self.part_source_path = self.demo_dir / demo_path.part_source_name

    def create_plan(self) -> CreateDemoPlan:
        """Create a complete plan."""
        if self.part_source_path.exists():
            raise UtilityError(f"Demo part already exists: {self.display_path(self.part_source_path)}")
        if self.part_source_path.is_symlink():
            raise UtilityError(f"Demo part path must not be a symbolic link: {self.part_source_path}")

        target_name = self.existing_or_generated_target_name()
        warnings = self.create_warnings()
        directories = self.create_directories()
        files = self.create_planned_files(target_name)
        return CreateDemoPlan(
            self.demo_path,
            target_name,
            tuple(files),
            tuple(directories),
            tuple(warnings),
            self.project_dir,
        )

    def display_path(self, path: Path) -> str:
        """Create a project-relative display path."""
        try:
            return path.relative_to(self.project_dir).as_posix()
        except ValueError:
            return path.as_posix()

    def create_warnings(self) -> list[str]:
        """Create non-fatal warnings for suspicious demo names."""
        warnings: list[str] = []
        source_domain_dir = self.project_dir / "src" / "erbsland" / self.demo_path.domain
        if not source_domain_dir.is_dir():
            warnings.append(f"Library domain does not exist: {self.display_path(source_domain_dir)}/")
            return warnings
        header_name = f"{self.demo_path.demo_name}.hpp"
        if not any(path.is_file() and not path.is_symlink() for path in source_domain_dir.rglob(header_name)):
            warnings.append(
                f"No matching library header found below {self.display_path(source_domain_dir)}/: {header_name}"
            )
        return warnings

    def create_directories(self) -> list[Path]:
        """Create the list of directories that must be created."""
        directories = []
        if not self.domain_dir.exists():
            directories.append(self.domain_dir)
        if not self.demo_dir.exists():
            directories.append(self.demo_dir)
        return directories

    def create_planned_files(self, target_name: str) -> list[PlannedFile]:
        """Create the list of file writes."""
        files: list[PlannedFile] = []
        self.add_file(
            files, self.root_cmake_path, self.render_subdirectory_cmake(self.root_cmake_path, self.demo_path.domain)
        )
        self.add_file(
            files,
            self.domain_cmake_path,
            self.render_subdirectory_cmake(self.domain_cmake_path, self.demo_path.demo_name),
        )
        if self.demo_dir.exists():
            self.require_existing_schema()
            files.extend(self.create_existing_demo_files(target_name))
        else:
            files.extend(self.create_new_demo_files(target_name))
        return files

    def create_new_demo_files(self, target_name: str) -> list[PlannedFile]:
        """Create all files for a new demo."""
        files: list[PlannedFile] = []
        source_files = [self.demo_path.header_name, self.demo_path.part_source_name, "main.cpp"]
        self.add_file(files, self.demo_cmake_path, self.render_demo_cmake(target_name, source_files))
        self.add_file(files, self.header_path, self.render_demo_header([self.demo_path.function_name]))
        self.add_file(
            files,
            self.main_path,
            self.render_main_cpp([(self.demo_path.demo_part, self.demo_path.function_name)]),
        )
        self.add_file(files, self.part_source_path, self.render_demo_source())
        return files

    def create_existing_demo_files(self, target_name: str) -> list[PlannedFile]:
        """Create all updates for an existing schema-style demo."""
        files: list[PlannedFile] = []
        cmake_sources = self.demo_cmake_sources()
        cmake_sources.append(self.demo_path.part_source_name)
        self.add_file(files, self.demo_cmake_path, self.render_demo_cmake(target_name, cmake_sources))

        declarations = self.header_declarations()
        declarations.append(self.demo_path.function_name)
        self.add_file(files, self.header_path, self.render_demo_header(declarations))

        registrations = self.main_registrations()
        registrations.append((self.demo_path.demo_part, self.demo_path.function_name))
        self.add_file(files, self.main_path, self.render_main_cpp(registrations))

        self.add_file(files, self.part_source_path, self.render_demo_source())
        return files

    def require_existing_schema(self) -> None:
        """Validate the existing demo can be extended by this utility."""
        for path in (self.demo_cmake_path, self.main_path, self.header_path):
            if path.is_symlink():
                raise UtilityError(f"Existing demo file must not be a symbolic link: {path}")
            if not path.is_file():
                raise UtilityError(
                    f"Existing demo does not follow the multi-part schema; missing {self.display_path(path)}"
                )
        main_text = self.main_path.read_text(encoding="utf-8")
        if "DemoApplication" not in main_text or "return app.run();" not in main_text:
            raise UtilityError(
                f"Existing demo does not follow the multi-part schema: {self.display_path(self.main_path)}"
            )

    def existing_or_generated_target_name(self) -> str:
        """Use the existing CMake demo target or create a new one."""
        if self.demo_cmake_path.is_file() and not self.demo_cmake_path.is_symlink():
            text = self.demo_cmake_path.read_text(encoding="utf-8")
            match = RE_DEMO_TARGET.search(text)
            if match is not None:
                return match.group("target")
        return lower_underscore(self.demo_path.demo_name)

    def demo_cmake_sources(self) -> list[str]:
        """Read source entries from the existing demo CMakeLists.txt."""
        text = self.demo_cmake_path.read_text(encoding="utf-8")
        match = RE_TARGET_SOURCES.search(text)
        if match is None:
            raise UtilityError(
                f"Demo CMake file has no target_sources block: {self.display_path(self.demo_cmake_path)}"
            )
        return source_tokens(match.group("content"))

    def header_declarations(self) -> list[str]:
        """Read demo function declarations from the existing header."""
        text = self.header_path.read_text(encoding="utf-8")
        return [match.group("name") for line in text.splitlines() if (match := RE_DEMO_DECLARATION.match(line))]

    def main_registrations(self) -> list[tuple[str, str]]:
        """Read demo registrations from the existing main.cpp."""
        text = self.main_path.read_text(encoding="utf-8")
        return [
            (match.group("name"), match.group("function"))
            for line in text.splitlines()
            if (match := RE_DEMO_REGISTRATION.match(line))
        ]

    def render_subdirectory_cmake(self, path: Path, added_entry: str) -> str:
        """Render a CMakeLists.txt with sorted add_subdirectory entries."""
        entries = []
        if path.is_file() and not path.is_symlink():
            entries = [match.group("name") for match in RE_ADD_SUBDIRECTORY.finditer(path.read_text(encoding="utf-8"))]
        entries.append(added_entry)
        lines = [CMAKE_HEADER.rstrip(), ""]
        lines.extend(f"add_subdirectory({entry})" for entry in sorted(set(entries), key=str.casefold))
        return "\n".join(lines) + "\n"

    def render_demo_cmake(self, target_name: str, source_files: list[str]) -> str:
        """Render a schema-style demo CMakeLists.txt."""
        lines = [
            CMAKE_HEADER.rstrip(),
            "",
            f"erbsland_core_add_demo({target_name})",
            f"target_sources({target_name} PRIVATE",
        ]
        for source_file in sorted(set(source_files), key=str.casefold):
            lines.append(f"        {source_file}")
        lines.append(")")
        return "\n".join(lines) + "\n"

    def render_demo_header(self, function_names: list[str]) -> str:
        """Render the demo declarations header."""
        lines = [CXX_HEADER.rstrip(), "#pragma once", "", "#include <DemoCommon.hpp>", ""]
        lines.extend(f"void {function_name}();" for function_name in sorted(set(function_names), key=str.casefold))
        return "\n".join(lines) + "\n"

    def render_main_cpp(self, registrations: list[tuple[str, str]]) -> str:
        """Render the demo main.cpp."""
        unique_registrations = sorted(set(registrations), key=lambda registration: registration[0].casefold())
        lines = [
            CXX_HEADER.rstrip(),
            "",
            f'#include "{self.demo_path.header_name}"',
            "",
            "#include <DemoCommon.hpp>",
            "",
            "auto main(const int argc, char *argv[]) -> int {",
            "    auto app = DemoApplication{argc, argv};",
        ]
        for demo_name, function_name in unique_registrations:
            lines.append(f'    app.registerDemo("{demo_name}"_el, {function_name});')
        lines.extend(["    return app.run();", "}"])
        return "\n".join(lines) + "\n"

    def render_demo_source(self) -> str:
        """Render the empty demo source file."""
        return (
            f"{CXX_HEADER}\n"
            "#include <DemoCommon.hpp>\n"
            "\n"
            "/// <Generic Title>\n"
            f"void {self.demo_path.function_name}() {{\n"
            "    // FIXME! Implement this demo.\n"
            "}\n"
        )

    @staticmethod
    def add_file(files: list[PlannedFile], path: Path, content: str) -> None:
        """Add a planned file only when the file content changes."""
        if path.exists() and not path.is_file():
            raise UtilityError(f"Demo path is not a regular file: {path}")
        if path.is_symlink():
            raise UtilityError(f"Demo file must not be a symbolic link: {path}")
        action = "update" if path.exists() else "create"
        if path.exists() and path.read_text(encoding="utf-8") == content:
            return
        files.append(PlannedFile(path, content, action))


class CreateDemoRunner:
    """Run the create-demo workflow."""

    def __init__(
        self,
        project_dir: Path,
        demo_path: DemoPath,
        *,
        assume_yes: bool = False,
        input_stream: TextIO = sys.stdin,
        output_stream: TextIO = sys.stdout,
    ) -> None:
        self.project_dir = project_dir
        self.demo_path = demo_path
        self.assume_yes = assume_yes
        self.input_stream = input_stream
        self.output_stream = output_stream

    def run(self) -> bool:
        """Run the utility and return `True` if files were written."""
        with ProjectFileLock(self.project_dir, "create-demo"):
            plan = CreateDemoPlanner(self.project_dir, self.demo_path).create_plan()
            plan.print_summary(self.output_stream)
            if not self.assume_yes and not self.confirm():
                print("Aborted.", file=self.output_stream)
                return False
            plan.apply()
            plan.print_documentation_block(self.output_stream)
            return True

    def confirm(self) -> bool:
        """Ask the user to confirm the planned changes."""
        if not self.input_stream.isatty():
            raise UtilityError("Confirmation requires an interactive terminal; use --yes to create the demo.")
        print("Ok? [Y/n]: ", end="", file=self.output_stream, flush=True)
        answer = self.input_stream.readline()
        if answer == "":
            raise UtilityError("Could not read confirmation; use --yes to create the demo.")
        answer = answer.strip().lower()
        if answer in {"", "y", "yes"}:
            return True
        if answer in {"n", "no"}:
            return False
        raise UtilityError("Please answer 'y' or 'n'.")


class CreateDemoApp(UtilityApp):
    """Create or extend a documentation demo."""

    description = "Create or extend an empty demo template."

    def __init__(self) -> None:
        super().__init__()
        self.demo_path = DemoPath("", "", "")
        self.assume_yes = False

    def add_command_line_args(self, parser: argparse.ArgumentParser) -> None:
        parser.add_argument("-y", "--yes", action="store_true", help="Create the demo without asking for confirmation.")
        parser.add_argument("demo_path", help="The demo path: <domain>/<DemoName>/<DemoPart>.")

    def handle_command_line_args(self, args: argparse.Namespace) -> None:
        self.demo_path = DemoPath.parse(args.demo_path)
        self.assume_yes = args.yes

    def run(self, argv=None) -> None:
        """Run the app."""
        super().run(argv)
        CreateDemoRunner(self.project_directory, self.demo_path, assume_yes=self.assume_yes).run()


def lower_underscore(value: str) -> str:
    """Convert PascalCase to lower_underscore."""
    value = re.sub(r"(.)([A-Z][a-z]+)", r"\1_\2", value)
    value = re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", value)
    return value.lower()


def source_tokens(content: str) -> list[str]:
    """Extract source file tokens from a target_sources body."""
    tokens: list[str] = []
    for line in content.splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        tokens.extend(token for token in line.split() if token != "PRIVATE")
    return tokens


def main() -> None:
    """Main entry point."""
    raise SystemExit(CreateDemoApp().main())


if __name__ == "__main__":
    main()
