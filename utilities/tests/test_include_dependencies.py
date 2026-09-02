# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from dev.include_dependencies import IncludeDependenciesApp
from lib.error import UtilityError
from lib.include_dependencies import (
    DependencyRule,
    IncludeGraph,
    PublicHeaderPolicy,
    direct_includes,
    tree_text,
)


class FixtureIncludeDependenciesApp(IncludeDependenciesApp):
    """Run output-path checks against a temporary project."""

    def __init__(self, project_directory: Path) -> None:
        super().__init__()
        self._project_directory = project_directory

    @property
    def project_directory(self) -> Path:
        return self._project_directory


class IncludeDependenciesTest(unittest.TestCase):
    """Tests for the reusable include graph and utility safety boundaries."""

    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.project_dir = Path(self.temp_dir.name)
        self.source_dir = self.project_dir / "src" / "erbsland"
        self.source_dir.mkdir(parents=True)
        self.policy = PublicHeaderPolicy(
            excluded_directories=frozenset({"impl", "attribute", "constraint"}),
            excluded_headers=frozenset({Path("system/MakeOneNamespace.hpp")}),
        )

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def write_source(self, name: str, text: str) -> Path:
        """Write one graph fixture file."""
        path = self.source_dir / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
        return path

    def sample_graph(self) -> IncludeGraph:
        """Build a graph with paths, external leaves, special files and one cycle."""
        self.write_source(
            "core/Definitions.hpp",
            '#pragma once\n#include "Namespaces.hpp"\n#include "MakeOneNamespace.hpp"\n',
        )
        self.write_source("core/Namespaces.hpp", "#pragma once\n")
        self.write_source("core/MakeOneNamespace.hpp", '#pragma once\n#include "Namespaces.hpp"\n')
        self.write_source("sample/Leaf.hpp", '#pragma once\n#include "../core/Definitions.hpp"\n#include <vector>\n')
        self.write_source("sample/Alpha.hpp", '#pragma once\n#include "Leaf.hpp"\n')
        self.write_source("sample/Beta.hpp", '#pragma once\n#include "Leaf.hpp"\n')
        self.write_source("sample/Root.hpp", '#pragma once\n#include "Beta.hpp"\n#include "Alpha.hpp"\n')
        self.write_source("sample/Missing.hpp", "#pragma once\n")
        self.write_source("sample/Thing_fwd.hpp", "#pragma once\n")
        self.write_source("sample/Thing.tpp", "// template\n")
        self.write_source("sample/all.hpp", '#pragma once\n#include "Root.hpp"\n')
        self.write_source("sample/impl/Private.hpp", "#pragma once\n")
        self.write_source("cycle/First.hpp", '#pragma once\n#include "Second.hpp"\n')
        self.write_source("cycle/Second.hpp", '#pragma once\n#include "First.hpp"\n')
        return IncludeGraph.from_project(self.project_dir, self.policy)

    def test_extracts_direct_includes_with_lines_and_kinds(self) -> None:
        includes = direct_includes('# include "Local.hpp"\n\n#include <vector>\n')

        self.assertEqual((1, "Local.hpp", False), (includes[0].line_number, includes[0].target, includes[0].system))
        self.assertEqual((3, "vector", True), (includes[1].line_number, includes[1].target, includes[1].system))

    def test_builds_complete_graph_and_classifies_public_headers(self) -> None:
        graph = self.sample_graph()

        self.assertEqual(14, len(graph.nodes))
        self.assertEqual(("vector",), graph.external_headers)
        self.assertTrue(graph.nodes["sample/Root.hpp"].public)
        self.assertFalse(graph.nodes["sample/Thing_fwd.hpp"].public)
        self.assertFalse(graph.nodes["sample/Thing.tpp"].public)
        self.assertFalse(graph.nodes["sample/all.hpp"].public)
        self.assertFalse(graph.nodes["sample/impl/Private.hpp"].public)

    def test_reports_missing_quoted_include(self) -> None:
        self.write_source("sample/Broken.hpp", '#include "Missing.hpp"\n')

        with self.assertRaisesRegex(UtilityError, r"sample/Broken\.hpp:1: Missing\.hpp"):
            IncludeGraph.from_project(self.project_dir, self.policy)

    def test_resolves_full_relative_and_unambiguous_suffix_paths(self) -> None:
        graph = self.sample_graph()

        self.assertEqual("sample/Root.hpp", graph.resolve_identifier("sample/Root.hpp"))
        self.assertEqual("sample/Root.hpp", graph.resolve_identifier("src/erbsland/sample/Root.hpp"))
        self.assertEqual("sample/Root.hpp", graph.resolve_identifier("Root.hpp"))
        self.write_source("other/Root.hpp", '#include "../core/Definitions.hpp"\n')
        ambiguous_graph = IncludeGraph.from_project(self.project_dir, self.policy)
        with self.assertRaisesRegex(UtilityError, "Ambiguous"):
            ambiguous_graph.resolve_identifier("Root.hpp")

    def test_finds_reachability_reverse_dependencies_and_deterministic_shortest_path(self) -> None:
        graph = self.sample_graph()

        self.assertTrue(graph.reaches("Root.hpp", "core/Definitions.hpp"))
        self.assertTrue(graph.reaches("Definitions.hpp", "Definitions.hpp"))
        self.assertFalse(graph.reaches("Missing.hpp", "Definitions.hpp"))
        self.assertIn("sample/Root.hpp", graph.reachable("Leaf.hpp", reverse=True))
        self.assertEqual(
            ("sample/Root.hpp", "sample/Alpha.hpp", "sample/Leaf.hpp", "core/Definitions.hpp"),
            graph.shortest_path("Root.hpp", "Definitions.hpp"),
        )

    def test_finds_strongly_connected_components(self) -> None:
        graph = self.sample_graph()

        self.assertEqual((("cycle/First.hpp", "cycle/Second.hpp"),), graph.strongly_connected_components())

    def test_dependency_rule_uses_public_scope_and_explicit_exceptions(self) -> None:
        graph = self.sample_graph()
        rule = DependencyRule(
            name="definitions",
            target="core/Definitions.hpp",
            excluded_headers=frozenset(
                {"core/Namespaces.hpp", "core/MakeOneNamespace.hpp", "cycle/First.hpp", "cycle/Second.hpp"}
            ),
        )

        self.assertEqual(("sample/Missing.hpp",), graph.missing_required_dependency(rule))

    def test_dependency_analysis_finds_minimal_transitive_leaf_frontier(self) -> None:
        self.sample_graph()
        self.write_source("sample/MissingParent.hpp", '#include "Missing.hpp"\n')
        graph = IncludeGraph.from_project(self.project_dir, self.policy)
        rule = DependencyRule(
            name="definitions",
            target="core/Definitions.hpp",
            excluded_headers=frozenset(
                {"core/Namespaces.hpp", "core/MakeOneNamespace.hpp", "cycle/First.hpp", "cycle/Second.hpp"}
            ),
        )

        analysis = graph.analyze_required_dependency(rule)

        self.assertEqual(("sample/Missing.hpp", "sample/MissingParent.hpp"), analysis.missing_headers)
        self.assertEqual(("sample/Missing.hpp",), analysis.insertion_points)
        self.assertEqual(
            "../core/Definitions.hpp", graph.relative_include_target("sample/Missing.hpp", analysis.target)
        )

    def test_dependency_analysis_chooses_one_representative_for_a_leaf_cycle(self) -> None:
        graph = self.sample_graph()
        rule = DependencyRule(
            name="definitions",
            target="core/Definitions.hpp",
            excluded_headers=frozenset({"core/Namespaces.hpp", "core/MakeOneNamespace.hpp"}),
        )

        analysis = graph.analyze_required_dependency(rule)

        self.assertEqual(("cycle/First.hpp", "sample/Missing.hpp"), analysis.insertion_points)

    def test_check_prints_only_leaf_directives_unless_all_is_requested(self) -> None:
        self.sample_graph()
        self.write_source("sample/MissingParent.hpp", '#include "Missing.hpp"\n')
        graph = IncludeGraph.from_project(self.project_dir, self.policy)
        rule = DependencyRule(
            name="definitions",
            target="core/Definitions.hpp",
            excluded_headers=frozenset(
                {"core/Namespaces.hpp", "core/MakeOneNamespace.hpp", "cycle/First.hpp", "cycle/Second.hpp"}
            ),
        )
        app = FixtureIncludeDependenciesApp(self.project_dir)
        app.rules = (rule,)

        with self.assertRaises(UtilityError) as context:
            app.run_check(graph)

        message = str(context.exception)
        self.assertIn('sample/Missing.hpp: #include "../core/Definitions.hpp"', message)
        self.assertNotIn("sample/MissingParent.hpp", message)

        app.show_all_violations = True
        with self.assertRaises(UtilityError) as context:
            app.run_check(graph)
        self.assertIn("sample/MissingParent.hpp", str(context.exception))

    def test_tree_marks_seen_cycles_external_leaves_and_max_depth(self) -> None:
        graph = self.sample_graph()

        root_tree = tree_text(graph, "Root.hpp")
        cycle_tree = tree_text(graph, "cycle/First.hpp")
        shallow_tree = tree_text(graph, "Root.hpp", max_depth=0)

        self.assertIn("<vector>", root_tree)
        self.assertIn("[seen]", root_tree)
        self.assertIn("[cycle]", cycle_tree)
        self.assertIn("[max depth]", shallow_tree)

    def test_serializes_stable_json_and_valid_dot_strings(self) -> None:
        graph = self.sample_graph()

        document = json.loads(graph.to_json())
        dot = graph.to_dot()

        self.assertEqual(1, document["schema"])
        self.assertEqual(14, document["summary"]["internal_nodes"])
        root_node = next(node for node in document["nodes"] if node["path"] == "sample/Root.hpp")
        self.assertEqual(2, root_node["direct_internal_dependencies"])
        self.assertEqual(6, root_node["transitive_internal_dependencies"])
        self.assertIn({"path": "<vector>", "kind": "external", "role": "external", "public": False}, document["nodes"])
        self.assertTrue(dot.startswith("digraph include_dependencies {\n"))
        self.assertIn('"sample/Leaf.hpp" -> "<vector>"', dot)
        self.assertEqual('a\\\\b\\"c\\nd', IncludeGraph._dot_escape('a\\b"c\nd'))

    def test_output_name_rejects_paths_traversal_and_wrong_suffixes(self) -> None:
        self.assertEqual("report.json", IncludeDependenciesApp.validate_output_name("report", "json"))
        self.assertEqual("report.dot", IncludeDependenciesApp.validate_output_name("report.dot", "dot"))
        for name in ("../report", "dir/report", "/tmp/report", "..", "report.dot"):
            with self.subTest(name=name), self.assertRaises(UtilityError):
                IncludeDependenciesApp.validate_output_name(name, "json")

    def test_output_requires_git_ignore_confirmation(self) -> None:
        subprocess.run(["git", "init", "--quiet", str(self.project_dir)], check=True)
        app = FixtureIncludeDependenciesApp(self.project_dir)

        with self.assertRaisesRegex(UtilityError, "did not confirm it is ignored"):
            app.verified_output_path("report")

        (self.project_dir / ".gitignore").write_text("/temporary_data/include_dependencies/\n", encoding="utf-8")
        self.assertEqual(
            self.project_dir.resolve() / "temporary_data/include_dependencies/report.json",
            app.verified_output_path("report"),
        )

    def test_output_rejects_symlinked_directory(self) -> None:
        subprocess.run(["git", "init", "--quiet", str(self.project_dir)], check=True)
        (self.project_dir / ".gitignore").write_text("/temporary_data/include_dependencies/\n", encoding="utf-8")
        temporary_directory = self.project_dir / "temporary_data"
        temporary_directory.mkdir()
        target = self.project_dir / "elsewhere"
        target.mkdir()
        (temporary_directory / "include_dependencies").symlink_to(target, target_is_directory=True)
        app = FixtureIncludeDependenciesApp(self.project_dir)

        with self.assertRaisesRegex(UtilityError, "must not be a symbolic link"):
            app.verified_output_path("report")

    def test_output_rejects_a_force_tracked_report(self) -> None:
        subprocess.run(["git", "init", "--quiet", str(self.project_dir)], check=True)
        (self.project_dir / ".gitignore").write_text("/temporary_data/include_dependencies/\n", encoding="utf-8")
        output_directory = self.project_dir / "temporary_data/include_dependencies"
        output_directory.mkdir(parents=True)
        output_path = output_directory / "report.json"
        output_path.write_text("{}\n", encoding="utf-8")
        subprocess.run(["git", "-C", str(self.project_dir), "add", "--force", str(output_path)], check=True)
        app = FixtureIncludeDependenciesApp(self.project_dir)

        with self.assertRaisesRegex(UtilityError, "tracked include dependency report"):
            app.verified_output_path("report")

    def test_stdout_export_creates_no_generated_files(self) -> None:
        graph = self.sample_graph()
        app = FixtureIncludeDependenciesApp(self.project_dir)
        app.output_name = None
        app.export_format = "json"

        with redirect_stdout(StringIO()) as output:
            app.run_export(graph)

        self.assertTrue(output.getvalue().startswith("{\n"))
        self.assertFalse((self.project_dir / "temporary_data").exists())

    def test_file_export_is_written_only_to_ignored_directory(self) -> None:
        graph = self.sample_graph()
        subprocess.run(["git", "init", "--quiet", str(self.project_dir)], check=True)
        (self.project_dir / ".gitignore").write_text("/temporary_data/include_dependencies/\n", encoding="utf-8")
        app = FixtureIncludeDependenciesApp(self.project_dir)
        app.output_name = "graph"
        app.export_format = "json"

        with redirect_stdout(StringIO()):
            app.run_export(graph)

        output_path = self.project_dir / "temporary_data/include_dependencies/graph.json"
        self.assertTrue(output_path.is_file())
        status = subprocess.run(
            ["git", "-C", str(self.project_dir), "status", "--short", "--untracked-files=all"],
            capture_output=True,
            text=True,
            check=True,
        ).stdout
        self.assertNotIn("graph.json", status)


if __name__ == "__main__":
    unittest.main()
