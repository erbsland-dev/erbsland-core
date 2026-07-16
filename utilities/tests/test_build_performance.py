# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import shutil
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from dev.build_performance import BuildPerformanceApp
from lib.build_performance import (
    add_forward_include_text,
    collect_audit_files,
    direct_includes,
    infer_platform,
    parse_ninja_dependencies,
    scan_report,
    trace_source_events,
    transitive_dependency_counts,
    validate_source_tree,
)


class BuildPerformanceTest(unittest.TestCase):
    """Tests for build-performance audit and measurement helpers."""

    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.project_dir = Path(self.temp_dir.name)
        self.source_dir = self.project_dir / "src" / "erbsland" / "sample"
        self.source_dir.mkdir(parents=True)

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def write_source(self, name: str, text: str) -> Path:
        """Write a source fixture below the sample namespace."""
        path = self.source_dir / name
        path.write_text(text, encoding="utf-8")
        return path

    def test_extracts_direct_includes_with_lines(self) -> None:
        includes = direct_includes('#include "Local.hpp"\n\n#include <vector>\n')

        self.assertEqual(2, len(includes))
        self.assertEqual((1, "Local.hpp", False), (includes[0].line_number, includes[0].target, includes[0].system))
        self.assertEqual((3, "vector", True), (includes[1].line_number, includes[1].target, includes[1].system))

    def test_adds_forward_include_after_pragma_once(self) -> None:
        source = "// Header\n#pragma once\n\n#include <vector>\n"

        result = add_forward_include_text(source, "Widget_fwd.hpp")

        self.assertEqual(
            '// Header\n#pragma once\n\n#include "Widget_fwd.hpp"\n\n#include <vector>\n',
            result,
        )
        self.assertEqual(result, add_forward_include_text(result, "Widget_fwd.hpp"))

    def test_inventory_covers_roles_and_template_owners(self) -> None:
        self.write_source("Widget_fwd.hpp", "#pragma once\nclass Widget;\n")
        self.write_source(
            "Widget.hpp", '#pragma once\n#include "Widget_fwd.hpp"\nclass Widget {};\n#include "Widget.tpp"\n'
        )
        self.write_source("Widget.tpp", "template<typename T> void useWidget(T) {}\n")
        self.write_source("Widget.cpp", '#include "Widget.hpp"\n')

        entries = {entry.path: entry for entry in collect_audit_files(self.project_dir)}

        self.assertEqual("forward", entries["src/erbsland/sample/Widget_fwd.hpp"].role)
        self.assertEqual("header", entries["src/erbsland/sample/Widget.hpp"].role)
        self.assertEqual("source", entries["src/erbsland/sample/Widget.cpp"].role)
        self.assertEqual("src/erbsland/sample/Widget.hpp", entries["src/erbsland/sample/Widget.tpp"].owner)
        self.assertEqual([], validate_source_tree(self.project_dir))

    def test_source_check_reports_forward_invariant(self) -> None:
        self.write_source("Widget_fwd.hpp", "#pragma once\nclass Widget;\n")
        self.write_source("Widget.hpp", "#pragma once\nclass Widget {};\n")

        issues = validate_source_tree(self.project_dir)

        self.assertIn(
            "Implementation header must include its forward header: src/erbsland/sample/Widget.hpp",
            issues,
        )

    def test_source_check_reports_unowned_template(self) -> None:
        self.write_source("Widget.tpp", "template<typename T> void widget(T) {}\n")

        self.assertIn(
            "Template implementation has no unique owning header: src/erbsland/sample/Widget.tpp",
            validate_source_tree(self.project_dir),
        )

    def test_classifies_platform_from_file_or_directory(self) -> None:
        self.assertEqual("windows", infer_platform(Path("impl/windows/Backend.cpp")))
        self.assertEqual("linux", infer_platform(Path("LinuxBackend.cpp")))
        self.assertEqual("macos", infer_platform(Path("AppleClock.hpp")))
        self.assertEqual("posix", infer_platform(Path("PosixSignal.hpp")))
        self.assertEqual("all", infer_platform(Path("Widget.hpp")))

    def test_parses_unique_ninja_dependency_fanout(self) -> None:
        widget = self.write_source("Widget.hpp", "#pragma once\n")
        source = self.write_source("Widget.cpp", '#include "Widget.hpp"\n')
        text = f"first.o: #deps 2\n    {source}\n    {widget}\n    {widget}\n" f"second.o: #deps 1\n    {widget}\n"

        result = parse_ninja_dependencies(text, self.project_dir)

        self.assertEqual(2, result["sample/Widget.hpp"])
        self.assertEqual(1, result["sample/Widget.cpp"])

    def test_counts_transitive_project_dependencies(self) -> None:
        self.write_source("Leaf.hpp", "#pragma once\n")
        self.write_source("Middle.hpp", '#pragma once\n#include "Leaf.hpp"\n')
        self.write_source("Root.hpp", '#pragma once\n#include "Middle.hpp"\n')
        files = collect_audit_files(self.project_dir)

        result = transitive_dependency_counts(files, self.project_dir)

        self.assertEqual(2, result["sample/Root.hpp"])
        self.assertEqual(1, result["sample/Middle.hpp"])
        self.assertEqual(0, result["sample/Leaf.hpp"])

    def test_reads_complete_and_begin_end_trace_events(self) -> None:
        document = {
            "traceEvents": [
                {"name": "Source", "ph": "X", "dur": 17, "args": {"detail": "/one.hpp"}},
                {"name": "Source", "ph": "b", "pid": 1, "tid": 2, "ts": 20, "args": {"detail": "/two.hpp"}},
                {"name": "Source", "ph": "e", "pid": 1, "tid": 2, "ts": 45},
            ]
        }

        self.assertEqual([("/one.hpp", 17), ("/two.hpp", 25)], list(trace_source_events(document)))

    def test_scan_report_orders_by_parse_cost_then_fanout(self) -> None:
        first = self.write_source("First.hpp", "#pragma once\n")
        second = self.write_source("Second.hpp", "#pragma once\n")
        files = collect_audit_files(self.project_dir)
        report = scan_report(
            files,
            {"sample/First.hpp": 20, "sample/Second.hpp": 30},
            {
                "sample/First.hpp": {"duration_us": 100, "count": 2},
                "sample/Second.hpp": {"duration_us": 200, "count": 1},
            },
        )

        self.assertEqual("src/erbsland/sample/Second.hpp", report["files"][0]["path"])
        self.assertTrue(first.is_file() and second.is_file())

    @unittest.skipUnless(shutil.which("c++"), "A C++ compiler is required for the include probe test.")
    def test_syntax_probe_accepts_removed_unused_include(self) -> None:
        dependency = self.write_source("Dependency.hpp", "#pragma once\nstruct Dependency {};\n")
        source = self.write_source("Widget.cpp", '#include "Dependency.hpp"\nint widget() { return 1; }\n')
        entry = {
            "directory": str(self.project_dir),
            "file": str(source),
            "arguments": [shutil.which("c++"), "-std=c++20", "-c", str(source), "-o", "Widget.o"],
        }
        app = BuildPerformanceApp()

        result = app.syntax_check(entry, source.parent, "int widget() { return 1; }\n")

        self.assertEqual(0, result.returncode, result.stderr)
        self.assertTrue(dependency.is_file())


if __name__ == "__main__":
    unittest.main()
