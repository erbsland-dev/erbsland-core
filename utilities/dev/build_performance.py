# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Audit and measure source-level C++ build performance."""

from __future__ import annotations

import argparse
import json
import os
import platform
import shlex
import shutil
import statistics
import subprocess
import tempfile
import time
from pathlib import Path

from lib.build_performance import (
    AuditFile,
    add_forward_include_text,
    collect_audit_files,
    collect_compiler_metrics,
    collect_compiler_total_us,
    collect_trace_metrics,
    direct_includes,
    parse_ninja_dependencies,
    scan_report,
    scan_report_text,
    transitive_dependency_counts,
    validate_source_tree,
)
from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.utility import UtilityApp


class BuildPerformanceApp(UtilityApp):
    """Audit library sources and collect compiler build-performance evidence."""

    description = "Audit and measure source-level C++ build performance."

    def __init__(self) -> None:
        super().__init__()
        self.command = "check"
        self.build_directory: Path | None = None
        self.output_path: Path | None = None
        self.paths: list[str] = []
        self.limit = 0
        self.jobs = 8
        self.runs = 3
        self.trace = False

    def add_command_line_args(self, parser: argparse.ArgumentParser) -> None:
        subparsers = parser.add_subparsers(dest="command")
        subparsers.add_parser("check", help="Check current source-tree ownership and forward-header invariants.")
        subparsers.add_parser(
            "fix-forward-includes", help="Add canonical forward-header includes to matching implementation headers."
        )

        scan_parser = subparsers.add_parser("scan", help="Create a ranked compiler and include report.")
        scan_parser.add_argument("--build-dir", type=Path, required=True, help="Configured Ninja build directory.")
        scan_parser.add_argument("--output", type=Path, help="Optional JSON report output path.")
        scan_parser.add_argument("--limit", type=int, default=100, help="Number of entries printed to the terminal.")

        probe_parser = subparsers.add_parser("probe", help="Probe direct includes using temporary syntax checks.")
        probe_parser.add_argument("--build-dir", type=Path, required=True, help="Build with compile_commands.json.")
        probe_parser.add_argument("--path", action="append", default=[], help="Source-relative path to probe.")
        probe_parser.add_argument("--limit", type=int, default=0, help="Maximum number of files; zero means all.")

        header_parser = subparsers.add_parser("headers", help="Compile headers as standalone translation units.")
        header_parser.add_argument("--build-dir", type=Path, required=True, help="Build with compile_commands.json.")
        header_parser.add_argument("--path", action="append", default=[], help="Source-relative header to compile.")
        header_parser.add_argument("--limit", type=int, default=0, help="Maximum number of headers; zero means all.")

        benchmark_parser = subparsers.add_parser("benchmark", help="Benchmark conventional and developer profiles.")
        benchmark_parser.add_argument("--jobs", type=int, default=8, help="Parallel build job count.")
        benchmark_parser.add_argument("--runs", type=int, default=3, help="Measured clean builds per profile.")
        benchmark_parser.add_argument("--trace", action="store_true", help="Enable Clang time traces.")
        benchmark_parser.add_argument("--output", type=Path, help="Optional JSON report output path.")

    def handle_command_line_args(self, args: argparse.Namespace) -> None:
        self.command = args.command or "check"
        self.build_directory = getattr(args, "build_dir", None)
        self.output_path = getattr(args, "output", None)
        self.paths = getattr(args, "path", [])
        self.limit = getattr(args, "limit", 0)
        self.jobs = getattr(args, "jobs", 8)
        self.runs = getattr(args, "runs", 3)
        self.trace = getattr(args, "trace", False)

    def run_check(self) -> None:
        """Validate current structural source invariants without persistent audit state."""
        issues = validate_source_tree(self.project_directory)
        if issues:
            raise UtilityError("Build-performance source check failed:\n" + "\n".join(f"- {issue}" for issue in issues))
        print(f"Checked {len(collect_audit_files(self.project_directory))} current library source files.")

    def run_fix_forward_includes(self) -> None:
        """Add every missing matching forward-header include."""
        source_directory = self.project_directory / "src" / "erbsland"
        file_update = FileUpdate(self.print_verbose if self.verbose else None)
        changed = 0
        for forward_path in sorted(source_directory.rglob("*_fwd.hpp")):
            implementation_path = forward_path.with_name(forward_path.name.removesuffix("_fwd.hpp") + ".hpp")
            if not implementation_path.is_file():
                continue
            text = implementation_path.read_text(encoding="utf-8")
            includes = direct_includes(text)
            if any(
                not include.system and (implementation_path.parent / include.target).resolve() == forward_path.resolve()
                for include in includes
            ):
                continue
            try:
                updated = add_forward_include_text(text, forward_path.name)
            except ValueError as error:
                raise UtilityError(f"Unable to update {implementation_path}: {error}") from error
            file_update.write_if_changed(implementation_path, updated)
            changed += 1
        print(f"Added {changed} canonical forward-header includes.")

    def require_build_directory(self) -> Path:
        """Validate and return the selected build directory."""
        if self.build_directory is None:
            raise UtilityError("A build directory is required.")
        result = self.build_directory.resolve()
        if not result.is_dir():
            raise UtilityError(f"Build directory not found: {result}")
        return result

    def run_scan(self) -> None:
        """Combine include inventory, Ninja dependencies, and Clang traces."""
        build_directory = self.require_build_directory()
        ninja_result = subprocess.run(
            ["ninja", "-C", str(build_directory), "-t", "deps"], capture_output=True, text=True, check=False
        )
        if ninja_result.returncode != 0:
            raise UtilityError(f"Unable to read Ninja dependencies:\n{ninja_result.stderr.rstrip()}")
        files = collect_audit_files(self.project_directory)
        fanout = parse_ninja_dependencies(ninja_result.stdout, self.project_directory)
        traces = collect_trace_metrics(build_directory, self.project_directory)
        compiler_duration_us, translation_units = collect_compiler_metrics(build_directory)
        document = scan_report(
            files,
            fanout,
            traces,
            transitive_dependency_counts(files, self.project_directory),
            compiler_duration_us,
            translation_units,
        )
        if self.output_path:
            output_path = self.output_path.resolve()
            FileUpdate(self.print_verbose if self.verbose else None).write_if_changed(
                output_path, json.dumps(document, indent=2) + "\n"
            )
        print(scan_report_text(document, self.limit), end="")

    def load_compile_commands(self, build_directory: Path) -> list[dict]:
        """Load a compilation database from a configured build."""
        path = build_directory / "compile_commands.json"
        if not path.is_file():
            raise UtilityError(f"Compilation database not found: {path}")
        try:
            document = json.loads(path.read_text(encoding="utf-8"))
        except json.JSONDecodeError as error:
            raise UtilityError(f"Invalid compilation database: {error}") from error
        if not isinstance(document, list) or not document:
            raise UtilityError("Compilation database is empty.")
        return document

    @staticmethod
    def command_arguments(entry: dict) -> list[str]:
        """Read one command from a compilation database entry."""
        if isinstance(entry.get("arguments"), list):
            return list(entry["arguments"])
        if isinstance(entry.get("command"), str):
            return shlex.split(entry["command"])
        raise UtilityError("Compilation database entry has no command.")

    @staticmethod
    def default_compile_entry(entries: list[dict]) -> dict:
        """Select a normal compilation command instead of a PCH-generation command."""
        for entry in entries:
            if "cmake_pch" not in str(entry.get("file", "")):
                return entry
        return entries[0]

    @staticmethod
    def syntax_arguments(entry: dict, source_parent: Path, input_path: Path | None = None) -> list[str]:
        """Convert a compiler command into a syntax-only check."""
        arguments = BuildPerformanceApp.command_arguments(entry)
        compiler_name = Path(arguments[0]).name.casefold()
        is_msvc = compiler_name in {"cl", "cl.exe", "clang-cl", "clang-cl.exe"}
        if is_msvc and input_path is None:
            raise UtilityError("MSVC syntax checks require a temporary source path.")
        source_path = str(Path(entry["file"]).resolve())
        result = [arguments[0]]
        index = 1
        while index < len(arguments):
            argument = arguments[index]
            if argument in {"-o", "-MF", "-MT", "-MQ", "-include-pch", "-x"}:
                index += 2
                continue
            if (
                argument == "-Xclang"
                and index + 1 < len(arguments)
                and arguments[index + 1]
                in {
                    "-emit-pch",
                    "-include-pch",
                    "-include",
                }
            ):
                index += 2
                if index + 1 < len(arguments) and arguments[index] == "-Xclang":
                    index += 2
                continue
            if argument in {"-c", "-MD", "-MMD", "-MP", "-ftime-trace", "/c"} or argument == source_path:
                index += 1
                continue
            if argument.startswith("-o") and len(argument) > 2:
                index += 1
                continue
            if is_msvc and argument.casefold().startswith(("/fo", "/fd", "/fp", "/yu")):
                index += 1
                continue
            if argument == "-Winvalid-pch":
                index += 1
                continue
            result.append(argument)
            index += 1
        if is_msvc:
            result.extend(["/Zs", "/TP", f"/I{source_parent}", str(input_path)])
        else:
            result.extend(["-iquote", str(source_parent), "-fsyntax-only"])
            if input_path:
                result.append(str(input_path))
            else:
                result.extend(["-x", "c++", "-"])
        return result

    def syntax_check(self, entry: dict, source_parent: Path, text: str) -> subprocess.CompletedProcess[str]:
        """Run one temporary syntax check."""
        text = text.replace("#pragma once\n", "", 1)
        return subprocess.run(
            self.syntax_arguments(entry, source_parent),
            cwd=entry.get("directory"),
            input=text,
            capture_output=True,
            text=True,
            check=False,
        )

    def syntax_file_check(self, entry: dict, path: Path) -> subprocess.CompletedProcess[str]:
        """Run a syntax check for a temporary mirrored source file."""
        return subprocess.run(
            self.syntax_arguments(entry, path.parent, path),
            cwd=entry.get("directory"),
            capture_output=True,
            text=True,
            check=False,
        )

    def select_files(self, files: list[AuditFile], suffix: str | None = None) -> list[AuditFile]:
        """Select audited files using optional source-relative path filters."""
        selected = files
        if suffix:
            selected = [file for file in selected if file.path.endswith(suffix)]
        if self.paths:
            requested = {path.removeprefix("./") for path in self.paths}
            selected = [
                file
                for file in selected
                if file.path in requested or file.path.removeprefix("src/erbsland/") in requested
            ]
            found = {file.path for file in selected}
            for requested_path in sorted(requested):
                canonical = (
                    requested_path if requested_path.startswith("src/erbsland/") else f"src/erbsland/{requested_path}"
                )
                if canonical not in found:
                    raise UtilityError(f"Audited source path not found: {requested_path}")
        selected = [file for file in selected if self.platform_matches(file.platform)]
        if self.limit > 0:
            selected = selected[: self.limit]
        return selected

    @staticmethod
    def platform_matches(source_platform: str) -> bool:
        """Test whether a source platform is supported by the host."""
        host = platform.system().casefold()
        return source_platform == "all" or source_platform == host or (source_platform == "posix" and host != "windows")

    def run_headers(self) -> None:
        """Compile selected headers standalone."""
        build_directory = self.require_build_directory()
        commands = self.load_compile_commands(build_directory)
        entry = self.default_compile_entry(commands)
        selected = self.select_files(collect_audit_files(self.project_directory), ".hpp")
        failures = []
        with tempfile.TemporaryDirectory(prefix="erbsland-core-header-check-") as temp_name:
            wrapper_path = Path(temp_name) / "header_check.cpp"
            for index, file in enumerate(selected, start=1):
                path = self.project_directory / file.path
                wrapper_path.write_text(f'#include "{path}"\n', encoding="utf-8")
                result = self.syntax_file_check(entry, wrapper_path)
                if result.returncode != 0:
                    details = (result.stderr or result.stdout).rstrip().splitlines()
                    detail = " | ".join(details[-8:]) if details else "compiler failed"
                    failures.append(f"{file.path}: {detail}")
                elif self.verbose:
                    print(f"[{index}/{len(selected)}] {file.path}")
        if failures:
            raise UtilityError("Standalone header checks failed:\n" + "\n".join(f"- {item}" for item in failures))
        print(f"Compiled {len(selected)} headers as standalone translation units.")

    def run_probe(self) -> None:
        """Probe whether direct includes can be removed without a syntax failure."""
        build_directory = self.require_build_directory()
        commands = self.load_compile_commands(build_directory)
        commands_by_file = {str(Path(entry["file"]).resolve()): entry for entry in commands}
        default_entry = self.default_compile_entry(commands)
        selected = self.select_files(collect_audit_files(self.project_directory))
        candidates = []
        failures = []
        with tempfile.TemporaryDirectory(prefix="erbsland-core-include-probe-") as temp_name:
            mirror_root = Path(temp_name) / "src" / "erbsland"
            shutil.copytree(self.project_directory / "src" / "erbsland", mirror_root)
            for file in selected:
                path = self.project_directory / file.path
                mirror_path = Path(temp_name) / file.path
                entry = commands_by_file.get(str(path.resolve()), default_entry)
                original = mirror_path.read_text(encoding="utf-8")
                base = self.syntax_file_check(entry, mirror_path)
                if base.returncode != 0:
                    detail = " | ".join(base.stderr.rstrip().splitlines()[-3:])
                    failures.append(f"{file.path}: baseline syntax check failed: {detail}")
                    continue
                lines = original.splitlines(keepends=True)
                for include in file.direct_includes:
                    modified = "".join(lines[: include.line_number - 1] + lines[include.line_number :])
                    mirror_path.write_text(modified, encoding="utf-8")
                    result = self.syntax_file_check(entry, mirror_path)
                    mirror_path.write_text(original, encoding="utf-8")
                    if result.returncode == 0:
                        candidates.append(f"{file.path}:{include.line_number}: {include.target}")
        if candidates:
            print("Removable-include candidates:")
            for candidate in candidates:
                print(f"- {candidate}")
        else:
            print("No removable-include candidates found.")
        if failures:
            raise UtilityError("Some files could not be probed:\n" + "\n".join(f"- {item}" for item in failures))

    def configure_profile(self, build_directory: Path, profile: str, source_directory: Path | None = None) -> None:
        """Configure one isolated benchmark profile."""
        conventional = profile == "conventional"
        if source_directory is None:
            source_directory = self.project_directory
        command = [
            "cmake",
            "-S",
            str(source_directory),
            "-B",
            str(build_directory),
            "-G",
            "Ninja",
            "-DCMAKE_BUILD_TYPE=Debug",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            "-DERBSLAND_CORE_ENABLE_TESTS=OFF",
            "-DERBSLAND_CORE_ENABLE_DEMOS=OFF",
            f"-DERBSLAND_CORE_DEVELOPER_BUILD={'OFF' if conventional else 'ON'}",
            f"-DERBSLAND_CORE_ENABLE_PRECOMPILED_HEADERS={'OFF' if conventional else 'ON'}",
            f"-DCMAKE_UNITY_BUILD={'OFF' if conventional else 'ON'}",
        ]
        if self.trace:
            command.append("-DCMAKE_CXX_FLAGS=-ftime-trace")
        result = subprocess.run(command, capture_output=True, text=True, check=False)
        if result.returncode != 0:
            raise UtilityError(f"Unable to configure {profile} benchmark:\n{result.stderr.rstrip()}")

    def benchmark_build(self, build_directory: Path) -> dict[str, float | int]:
        """Run and time one clean configured build."""
        start = time.perf_counter()
        result = subprocess.run(
            ["cmake", "--build", str(build_directory), "--parallel", str(self.jobs)],
            capture_output=True,
            text=True,
            check=False,
        )
        elapsed = time.perf_counter() - start
        if result.returncode != 0:
            raise UtilityError(f"Benchmark build failed:\n{result.stdout.rstrip()}\n{result.stderr.rstrip()}")
        return {
            "wall_seconds": elapsed,
            "compiler_seconds": collect_compiler_total_us(build_directory) / 1_000_000 if self.trace else 0.0,
        }

    @staticmethod
    def object_timestamps(build_directory: Path) -> dict[Path, int]:
        """Snapshot object-file timestamps for rebuilt-object accounting."""
        return {path: path.stat().st_mtime_ns for path in build_directory.rglob("*.o")}

    def benchmark_incremental_headers(self, temp_directory: Path) -> list[dict[str, object]]:
        """Benchmark isolated rebuilds for the ten highest-fanout headers in a source copy."""
        source_directory = temp_directory / "incremental-source"

        def ignore(directory: str, names: list[str]) -> set[str]:
            del directory
            return {
                name
                for name in names
                if name in {".git", ".venv", ".idea", ".codex"}
                or name.startswith("cmake-build-")
                or name.startswith("_build")
            }

        shutil.copytree(self.project_directory, source_directory, ignore=ignore)
        build_directory = temp_directory / "incremental-build"
        self.configure_profile(build_directory, "conventional", source_directory)
        self.benchmark_build(build_directory)
        dependencies = subprocess.run(
            ["ninja", "-C", str(build_directory), "-t", "deps"],
            capture_output=True,
            text=True,
            check=False,
        )
        if dependencies.returncode != 0:
            raise UtilityError(f"Unable to inspect incremental benchmark dependencies:\n{dependencies.stderr.rstrip()}")
        fanout = parse_ninja_dependencies(dependencies.stdout, source_directory)
        headers = sorted(
            ((path, count) for path, count in fanout.items() if path.endswith(".hpp") and not path.endswith("all.hpp")),
            key=lambda item: (-item[1], item[0]),
        )[:10]
        results = []
        for relative_path, dependent_count in headers:
            before = self.object_timestamps(build_directory)
            if self.trace:
                for trace_path in build_directory.rglob("*.json"):
                    trace_path.unlink()
            header_path = source_directory / "src" / "erbsland" / relative_path
            os.utime(header_path, None)
            start = time.perf_counter()
            result = subprocess.run(
                ["cmake", "--build", str(build_directory), "--parallel", str(self.jobs)],
                capture_output=True,
                text=True,
                check=False,
            )
            elapsed = time.perf_counter() - start
            if result.returncode != 0:
                raise UtilityError(
                    f"Incremental benchmark failed for {relative_path}:\n"
                    f"{result.stdout.rstrip()}\n{result.stderr.rstrip()}"
                )
            after = self.object_timestamps(build_directory)
            rebuilt_objects = sum(1 for path, timestamp in after.items() if before.get(path) != timestamp)
            results.append(
                {
                    "path": f"src/erbsland/{relative_path}",
                    "dependent_translation_units": dependent_count,
                    "wall_seconds": elapsed,
                    "compiler_seconds": (
                        collect_compiler_total_us(build_directory) / 1_000_000 if self.trace else None
                    ),
                    "rebuilt_objects": rebuilt_objects,
                }
            )
        return results

    def run_benchmark(self) -> None:
        """Benchmark isolated conventional and developer-default build profiles."""
        if self.jobs < 1 or self.runs < 1:
            raise UtilityError("Benchmark jobs and runs must be positive.")
        report: dict[str, object] = {"schema": 1, "jobs": self.jobs, "runs": self.runs, "profiles": {}}
        with tempfile.TemporaryDirectory(prefix="erbsland-core-build-performance-") as temp_name:
            temp_directory = Path(temp_name)
            for profile in ("conventional", "developer-default"):
                samples = []
                for index in range(self.runs + 1):
                    build_directory = temp_directory / f"{profile}-{index}"
                    self.configure_profile(build_directory, profile)
                    sample = self.benchmark_build(build_directory)
                    if index > 0:
                        samples.append(sample)
                wall_values = [float(sample["wall_seconds"]) for sample in samples]
                compiler_values = [float(sample["compiler_seconds"]) for sample in samples]
                report["profiles"][profile] = {
                    "samples": samples,
                    "median_wall_seconds": statistics.median(wall_values),
                    "median_compiler_seconds": statistics.median(compiler_values) if self.trace else None,
                }
            report["incremental_headers"] = self.benchmark_incremental_headers(temp_directory)
        text = json.dumps(report, indent=2) + "\n"
        if self.output_path:
            FileUpdate(self.print_verbose if self.verbose else None).write_if_changed(self.output_path.resolve(), text)
        print(text, end="")

    def run(self, argv=None) -> None:
        """Run the selected audit or measurement command."""
        super().run(argv)
        handlers = {
            "check": self.run_check,
            "fix-forward-includes": self.run_fix_forward_includes,
            "scan": self.run_scan,
            "probe": self.run_probe,
            "headers": self.run_headers,
            "benchmark": self.run_benchmark,
        }
        handlers[self.command]()


def main() -> None:
    """Run the build-performance utility."""
    raise SystemExit(BuildPerformanceApp().main())


if __name__ == "__main__":
    main()
