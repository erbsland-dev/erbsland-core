# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Inspect and validate the library's source-level include dependency graph."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path

from lib.config import read_elcl_file, validate_local_names, validate_source_relative_path
from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.include_dependencies import (
    DependencyRule,
    IncludeGraph,
    PublicHeaderPolicy,
    summary_text,
    tree_text,
)
from lib.path_safety import require_safe_existing_file
from lib.safe_tool import safe_subprocess_environment
from lib.utility import UtilityApp


class IncludeDependenciesApp(UtilityApp):
    """Query and validate the complete source-level include graph."""

    description = "Inspect and validate source-level include dependencies."
    OUTPUT_DIRECTORY = Path("temporary_data/include_dependencies")

    def __init__(self) -> None:
        super().__init__()
        self.command = "summary"
        self.header = ""
        self.source = ""
        self.target = ""
        self.reverse = False
        self.max_depth: int | None = None
        self.export_format = "json"
        self.output_name: str | None = None
        self.rule_name: str | None = None
        self.show_all_violations = False
        self.public_policy = PublicHeaderPolicy()
        self.rules: tuple[DependencyRule, ...] = ()

    def add_command_line_args(self, parser: argparse.ArgumentParser) -> None:
        subparsers = parser.add_subparsers(dest="command")
        subparsers.add_parser("summary", help="Show graph, role, external-header and cycle totals.")

        tree_parser = subparsers.add_parser("tree", help="Show a complete dependency tree for one source.")
        tree_parser.add_argument("header", help="Library source path or unambiguous path suffix.")
        tree_parser.add_argument("--reverse", action="store_true", help="Show dependents instead of dependencies.")
        tree_parser.add_argument("--max-depth", type=int, help="Optional maximum traversal depth.")

        path_parser = subparsers.add_parser("path", help="Show the shortest include path between two sources.")
        path_parser.add_argument("source", help="Source library path.")
        path_parser.add_argument("target", help="Target library path.")

        subparsers.add_parser("cycles", help="List all include cycles.")

        export_parser = subparsers.add_parser("export", help="Export the complete graph as JSON or Graphviz DOT.")
        export_parser.add_argument("--format", choices=("json", "dot"), default="json", help="Export format.")
        export_parser.add_argument(
            "--output",
            metavar="NAME",
            help="Write below temporary_data/include_dependencies; otherwise print to standard output.",
        )

        check_parser = subparsers.add_parser("check", help="Run configured required-dependency checks.")
        check_parser.add_argument("rule", nargs="?", help="Optional configured rule name; default is all rules.")
        check_parser.add_argument(
            "--all", action="store_true", dest="show_all_violations", help="Also list every affected public header."
        )

    def handle_command_line_args(self, args: argparse.Namespace) -> None:
        self.command = args.command or "summary"
        self.header = getattr(args, "header", "")
        self.source = getattr(args, "source", "")
        self.target = getattr(args, "target", "")
        self.reverse = getattr(args, "reverse", False)
        self.max_depth = getattr(args, "max_depth", None)
        self.export_format = getattr(args, "format", "json")
        self.output_name = getattr(args, "output", None)
        self.rule_name = getattr(args, "rule", None)
        self.show_all_violations = getattr(args, "show_all_violations", False)
        if self.max_depth is not None and self.max_depth < 0:
            raise UtilityError("Maximum tree depth must not be negative.")

    def read_config(self) -> None:
        """Read public-header policy and named dependency checks."""
        config = read_elcl_file(self.config_file_path())
        main_config = config["main"]
        excluded_directories = frozenset(main_config.get_list("excluded_directories", str, default=[]))
        excluded_headers = frozenset(Path(path) for path in main_config.get_list("excluded_headers", str, default=[]))
        validate_local_names(excluded_directories, "Excluded Directories")
        for header in excluded_headers:
            validate_source_relative_path(header.as_posix(), "Excluded Headers")
        self.public_policy = PublicHeaderPolicy(excluded_directories, excluded_headers)

        rules = []
        names = set()
        for index, rule_config in enumerate(config.get("required_dependencies", []), start=1):
            label = f"Required Dependencies entry {index}"
            name = rule_config.get_text("name")
            validate_local_names([name], f"{label} Name")
            if name in names:
                raise UtilityError(f"Duplicate required-dependency rule name: {name}")
            names.add(name)
            target = rule_config.get_text("target")
            validate_source_relative_path(target, f"{label} Target")
            excluded = frozenset(rule_config.get_list("excluded_headers", str, default=[]))
            for header in excluded:
                validate_source_relative_path(header, f"{label} Excluded Headers")
            rules.append(DependencyRule(name=name, target=target, excluded_headers=excluded))
        if not rules:
            raise UtilityError("No required-dependency rules are configured.")
        self.rules = tuple(rules)

    def graph(self) -> IncludeGraph:
        """Build the configured library include graph."""
        return IncludeGraph.from_project(self.project_directory, self.public_policy)

    def run_summary(self, graph: IncludeGraph) -> None:
        """Print graph summary statistics."""
        print(summary_text(graph), end="")

    def run_tree(self, graph: IncludeGraph) -> None:
        """Print a forward or reverse dependency tree."""
        print(tree_text(graph, self.header, reverse=self.reverse, max_depth=self.max_depth), end="")

    def run_path(self, graph: IncludeGraph) -> None:
        """Print a shortest include path."""
        path = graph.shortest_path(self.source, self.target)
        if path is None:
            raise UtilityError(f"No include path from {self.source} to {self.target}.")
        print("\n".join(path))

    def run_cycles(self, graph: IncludeGraph) -> None:
        """Print every graph cycle."""
        components = graph.strongly_connected_components()
        if not components:
            print("No include cycles found.")
            return
        print(f"Include cycles: {len(components)}")
        for index, component in enumerate(components, start=1):
            print(f"{index}:")
            for path in component:
                print(f"  {path}")

    @classmethod
    def validate_output_name(cls, name: str, export_format: str) -> str:
        """Validate a safe local report filename and its format suffix."""
        path = Path(name)
        if not name.strip() or path.is_absolute() or path.name != name or name in {".", ".."}:
            raise UtilityError(f"Output must be a filename without directory components: {name}")
        if "/" in name or "\\" in name:
            raise UtilityError(f"Output must be a filename without directory components: {name}")
        expected_suffix = f".{export_format}"
        if path.suffix and path.suffix.casefold() != expected_suffix:
            raise UtilityError(f"{export_format.upper()} output filename must end with {expected_suffix}: {name}")
        return name if path.suffix else f"{name}{expected_suffix}"

    def verified_output_path(self, name: str) -> Path:
        """Resolve an output path and verify that Git ignores it before writing."""
        safe_name = self.validate_output_name(name, self.export_format)
        project_directory = self.project_directory.resolve()
        output_directory = project_directory / self.OUTPUT_DIRECTORY
        output_path = output_directory / safe_name
        for parent in [output_path.parent, *output_path.parent.parents]:
            if parent == project_directory.parent:
                break
            if parent.is_symlink():
                raise UtilityError(f"Include dependency output parent directory must not be a symbolic link: {parent}")
            if parent.exists() and not parent.is_dir():
                raise UtilityError(f"Include dependency output parent path must be a directory: {parent}")
        require_safe_existing_file(output_path, "Include dependency output", FileUpdate.MAX_COMPARE_FILE_SIZE)
        relative_output_path = output_path.relative_to(project_directory).as_posix()
        try:
            tracked_result = subprocess.run(
                [
                    "git",
                    "-C",
                    str(self.project_directory),
                    "ls-files",
                    "--error-unmatch",
                    "--",
                    relative_output_path,
                ],
                capture_output=True,
                text=True,
                check=False,
                env=safe_subprocess_environment(),
            )
            if tracked_result.returncode == 0:
                raise UtilityError(f"Refusing to overwrite a tracked include dependency report: {output_path}")
            if tracked_result.returncode != 1:
                detail = tracked_result.stderr.strip() or "Git repository check failed."
                raise UtilityError(f"Unable to verify include dependency output tracking state: {detail}")
            ignored_result = subprocess.run(
                [
                    "git",
                    "-C",
                    str(self.project_directory),
                    "check-ignore",
                    "--quiet",
                    "--",
                    relative_output_path,
                ],
                capture_output=True,
                text=True,
                check=False,
                env=safe_subprocess_environment(),
            )
        except OSError as error:
            raise UtilityError(f"Unable to verify that include dependency output is ignored: {error}") from error
        if ignored_result.returncode != 0:
            detail = ignored_result.stderr.strip()
            if detail:
                detail = f" ({detail})"
            raise UtilityError(
                f"Refusing to write include dependency output because Git did not confirm it is ignored: "
                f"{output_path}{detail}"
            )
        return output_path

    def run_export(self, graph: IncludeGraph) -> None:
        """Print or safely write a complete graph export."""
        text = graph.to_json() if self.export_format == "json" else graph.to_dot()
        if self.output_name is None:
            print(text, end="")
            return
        output_path = self.verified_output_path(self.output_name)
        FileUpdate(self.print_verbose if self.verbose else None).write_if_changed(output_path, text)
        print(f"Wrote ignored include dependency report: {output_path.relative_to(self.project_directory.resolve())}")

    def selected_rules(self) -> tuple[DependencyRule, ...]:
        """Select one configured rule or all rules."""
        if self.rule_name is None:
            return self.rules
        selected = tuple(rule for rule in self.rules if rule.name == self.rule_name)
        if not selected:
            available = ", ".join(rule.name for rule in self.rules)
            raise UtilityError(f"Unknown required-dependency rule: {self.rule_name}. Available: {available}")
        return selected

    def run_check(self, graph: IncludeGraph) -> None:
        """Run configured transitive required-dependency checks."""
        failures = []
        for rule in self.selected_rules():
            analysis = graph.analyze_required_dependency(rule)
            if not analysis.missing_headers:
                print(f"{rule.name}: all public headers reach {graph.resolve_identifier(rule.target)}.")
                continue
            failures.append((rule, analysis))
        if failures:
            lines = ["Required include dependency checks failed:"]
            for rule, analysis in failures:
                lines.append(
                    f"- {rule.name}: {len(analysis.missing_headers)} public headers do not reach {analysis.target}."
                )
                lines.append(f"  Add a direct include to these {len(analysis.insertion_points)} leaf insertion points:")
                for path in analysis.insertion_points:
                    include_target = graph.relative_include_target(path, analysis.target)
                    lines.append(f'    {path}: #include "{include_target}"')
                covered_count = len(analysis.missing_headers) - len(analysis.insertion_points)
                if covered_count:
                    lines.append(f"  This transitively covers the other {covered_count} affected public headers.")
                if self.show_all_violations:
                    lines.append("  All affected public headers:")
                    lines.extend(f"    {path}" for path in analysis.missing_headers)
            raise UtilityError("\n".join(lines))

    def run(self, argv=None) -> None:
        """Build the include graph and run the selected query."""
        super().run(argv)
        self.read_config()
        graph = self.graph()
        handlers = {
            "summary": self.run_summary,
            "tree": self.run_tree,
            "path": self.run_path,
            "cycles": self.run_cycles,
            "export": self.run_export,
            "check": self.run_check,
        }
        handlers[self.command](graph)


def main() -> None:
    """Run the include dependency utility."""
    raise SystemExit(IncludeDependenciesApp().main())


if __name__ == "__main__":
    main()
