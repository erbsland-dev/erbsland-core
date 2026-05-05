# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from dev.rebuild_doc import DocumentationOutputFilter, RebuildDocApp


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

    def test_filters_doxygen_undocumented_impl_entries(self) -> None:
        self.assertIsNone(
            self.output_filter.filter_line(
                "/workspace/erbsland-core/_doxygen_input/erbsland/text/impl/StringData.hpp:9: "
                "warning: Compound erbsland::text::impl::StringData is not documented."
            )
        )
        self.assertIsNone(
            self.output_filter.filter_line(
                "/workspace/erbsland-core/_doxygen_input/erbsland/text/impl/StringData.hpp:9: "
                "warning: Compound erbsland::text::StringData is not documented."
            )
        )

    def test_filters_doxygen_undocumented_std_specializations(self) -> None:
        self.assertIsNone(
            self.output_filter.filter_line(
                "/workspace/erbsland-core/_doxygen_input/erbsland/text/String.hpp:100: "
                "warning: Compound std::hash<erbsland::text::String> is not documented."
            )
        )
        self.assertIsNone(
            self.output_filter.filter_line(
                "/workspace/erbsland-core/_doxygen_input/erbsland/text/String.hpp:101: "
                "warning: Compound std::format<erbsland::text::String> is not documented."
            )
        )

    def test_filters_doxygen_undocumented_operators_and_copy_move_constructors(
        self,
    ) -> None:
        self.assertIsNone(
            self.output_filter.filter_line(
                "/workspace/erbsland-core/_doxygen_input/erbsland/options/Option.hpp:50: "
                "warning: Member operator==(const Option &) const (function) of class erbsland::options::Option "
                "is not documented."
            )
        )
        self.assertIsNone(
            self.output_filter.filter_line(
                "/workspace/erbsland-core/_doxygen_input/erbsland/err/OptionError.hpp:25: "
                "warning: Member OptionError(const OptionError &)=default (function) of class "
                "erbsland::err::OptionError is not documented."
            )
        )
        self.assertIsNone(
            self.output_filter.filter_line(
                "/workspace/erbsland-core/_doxygen_input/erbsland/err/OptionError.hpp:26: "
                "warning: Member OptionError(OptionError &&)=default (function) of class "
                "erbsland::err::OptionError is not documented."
            )
        )
        self.assertIsNone(
            self.output_filter.filter_line(
                "/workspace/erbsland-core/_doxygen_input/erbsland/time/Date.hpp:54: "
                "warning: Member Date(const Date &) noexcept=default (function) of class erbsland::time::Date "
                "is not documented."
            )
        )
        self.assertIsNone(
            self.output_filter.filter_line(
                "/workspace/erbsland-core/_doxygen_input/erbsland/util/List.hpp:60: "
                "warning: Member List(const List &) noexcept=default (function) of class "
                "erbsland::util::List< tString, StringList< tString > > is not documented."
            )
        )

    def test_shows_doxygen_unexpanded_alias_warning_and_error(self) -> None:
        self.assertEqual(
            "erbsland/text/String.hpp:20: warning: Found unknown command '@tested'",
            self.output_filter.filter_line(
                "/workspace/erbsland-core/_doxygen_input/erbsland/text/String.hpp:20: "
                "warning: Found unknown command '@tested'"
            ),
        )
        self.assertEqual(
            "erbsland/text/String.hpp:21: error: failed to parse declaration",
            self.output_filter.filter_line(
                "/workspace/erbsland-core/_doxygen_input/erbsland/text/String.hpp:21: "
                "error: failed to parse declaration"
            ),
        )

    def test_shows_sphinx_diagnostic_path_lines_only_with_short_path(self) -> None:
        self.assertEqual(
            "reference/text/string.rst:12: WARNING: duplicate object description",
            self.output_filter.filter_line(
                "/workspace/erbsland-core/doc/reference/text/string.rst:12: "
                "WARNING: duplicate object description"
            ),
        )
        self.assertIsNone(
            self.output_filter.filter_line("   more detail about the duplicate")
        )
        self.assertIsNone(
            self.output_filter.filter_line("WARNING: summary without source path")
        )
        self.assertIsNone(
            self.output_filter.filter_line("ERROR: summary without source path")
        )

    def test_rebuild_doc_no_filter_flag_disables_filter(self) -> None:
        app = RebuildDocApp()
        app.parse_command_line(["--no-filter"])

        self.assertFalse(app.filter_output)


if __name__ == "__main__":
    unittest.main()
