# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import io
import sys
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from dev.rebuild_doc import DocumentationOutputFilter, DoxygenWarningSuppression, RebuildDocApp
from lib.error import UtilityError


class DocumentationOutputFilterTest(unittest.TestCase):
    """Tests for the rebuild_doc output filter."""

    def setUp(self) -> None:
        self.project_root = Path("/workspace/erbsland-core")
        self.output_filter = DocumentationOutputFilter(self.project_root)

    def test_shows_relevant_doxygen_undocumented_warning_with_short_path(self) -> None:
        line = (
            "/workspace/erbsland-core/_doxygen_input/erbsland/options/Option.hpp:42: "
            "warning: Member parse() (function) of class erbsland::options::Option is not documented."
        )

        self.assertEqual(
            "erbsland/options/Option.hpp:42: "
            "warning: Member parse() (function) of class erbsland::options::Option is not documented.",
            self.output_filter.filter_line(line),
        )

    def test_shows_unmatched_impl_and_operator_warnings(self) -> None:
        for line in (
            "/workspace/erbsland-core/_doxygen_input/erbsland/text/impl/StringData.hpp:9: "
            "warning: Compound erbsland::text::impl::StringData is not documented.",
            "/workspace/erbsland-core/_doxygen_input/erbsland/options/Option.hpp:50: "
            "warning: Member operator==(const Option &) const (function) of class erbsland::options::Option "
            "is not documented.",
        ):
            self.assertIsNotNone(self.output_filter.filter_line(line))

    def test_suppresses_only_matching_configured_warning(self) -> None:
        rule = DoxygenWarningSuppression(
            source_glob="erbsland/**/impl/**",
            message_contains="is not documented.",
            message_regex="",
            reason="Internal implementation declarations.",
        )
        self.output_filter.suppression_rules = (rule,)
        self.assertIsNone(
            self.output_filter.filter_line(
                "/workspace/erbsland-core/_doxygen_input/erbsland/text/impl/StringData.hpp:9: "
                "warning: Compound erbsland::text::impl::StringData is not documented."
            )
        )
        self.assertIsNotNone(
            self.output_filter.filter_line(
                "/workspace/erbsland-core/_doxygen_input/erbsland/text/impl/StringData.hpp:10: "
                "warning: Found unknown command '@broken'"
            )
        )
        self.assertEqual(
            ["Suppressed 1 Doxygen warning(s): Internal implementation declarations."],
            self.output_filter.suppressed_warning_summary(),
        )

    def test_shows_doxygen_unexpanded_alias_warning_and_error(self) -> None:
        self.assertEqual(
            "erbsland/text/StringEditor.hpp:20: warning: Found unknown command '@tested'",
            self.output_filter.filter_line(
                "/workspace/erbsland-core/_doxygen_input/erbsland/text/StringEditor.hpp:20: "
                "warning: Found unknown command '@tested'"
            ),
        )
        self.assertEqual(
            "erbsland/text/StringEditor.hpp:21: error: failed to parse declaration",
            self.output_filter.filter_line(
                "/workspace/erbsland-core/_doxygen_input/erbsland/text/StringEditor.hpp:21: "
                "error: failed to parse declaration"
            ),
        )

    def test_shows_sphinx_diagnostics_with_short_paths(self) -> None:
        self.assertEqual(
            "reference/text/string.rst:12: WARNING: duplicate object description",
            self.output_filter.filter_line(
                "/workspace/erbsland-core/doc/reference/text/string.rst:12: " "WARNING: duplicate object description"
            ),
        )
        self.assertIsNone(self.output_filter.filter_line("   more detail about the duplicate"))
        self.assertEqual(
            "WARNING: summary without source path",
            self.output_filter.filter_line("WARNING: summary without source path"),
        )
        self.assertEqual(
            "ERROR: summary without source path", self.output_filter.filter_line("ERROR: summary without source path")
        )

    def test_rebuild_doc_no_filter_flag_disables_filter(self) -> None:
        app = RebuildDocApp()
        app.parse_command_line(["--no-filter"])

        self.assertFalse(app.filter_output)

    def test_rebuild_doc_show_suppressed_flag(self) -> None:
        app = RebuildDocApp()
        app.parse_command_line(["--show-suppressed"])

        self.assertTrue(app.show_suppressed)

    def test_rebuild_doc_runs_sphinx_with_eight_workers(self) -> None:
        app = RebuildDocApp()
        app.project_root = self.project_root
        executable = self.project_root / ".venv/bin/sphinx-build"

        with (
            patch.object(app, "sphinx_build_executable", return_value=executable),
            patch.object(app, "run_sphinx_build_filtered", return_value=0) as run_sphinx_build,
        ):
            app.rebuild_documentation()

        run_sphinx_build.assert_called_once_with([str(executable), "-j", "8", "doc", "_build"])

    def test_rebuild_doc_reports_parallel_sphinx_failure(self) -> None:
        app = RebuildDocApp()
        app.project_root = self.project_root
        executable = self.project_root / ".venv/bin/sphinx-build"

        with (
            patch.object(app, "sphinx_build_executable", return_value=executable),
            patch.object(app, "run_sphinx_build_filtered", return_value=2) as run_sphinx_build,
            self.assertRaisesRegex(UtilityError, "sphinx-build failed with exit code 2"),
        ):
            app.rebuild_documentation()

        run_sphinx_build.assert_called_once_with([str(executable), "-j", "8", "doc", "_build"])

    def test_filtered_parallel_failure_replays_complete_output(self) -> None:
        app = RebuildDocApp()
        app.project_root = self.project_root
        process = Mock()
        process.stdout = iter(
            [
                "reading sources... [100%] reference/text/string\n",
                "Sphinx parallel build error:\n",
                "Traceback detail without a diagnostic prefix\n",
            ]
        )
        process.wait.return_value = 2
        output = io.StringIO()

        with (
            patch("dev.rebuild_doc.subprocess.Popen", return_value=process),
            patch.object(app, "sphinx_environment", return_value={}),
            redirect_stdout(output),
        ):
            return_code = app.run_sphinx_build_filtered(["sphinx-build", "-j", "8", "doc", "_build"])

        self.assertEqual(2, return_code)
        self.assertIn("Complete sphinx-build output after failure:", output.getvalue())
        self.assertIn("Traceback detail without a diagnostic prefix", output.getvalue())


if __name__ == "__main__":
    unittest.main()
