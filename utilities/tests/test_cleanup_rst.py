# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from dev.cleanup_rst import CleanupRstConfig, CleanupRstRunner, DoxygenSymbolIndex, RstCleaner


class CleanupRstTest(unittest.TestCase):
    """Tests for the reStructuredText cleanup utility."""

    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory(dir="/private/tmp")
        self.project_dir = Path(self.temp_dir.name)
        self.index_path = self.project_dir / "_build" / "breathe" / "doxygen" / "erbsland-core" / "xml" / "index.xml"
        self.index_path.parent.mkdir(parents=True)
        self.index_path.write_text(
            """<?xml version='1.0' encoding='UTF-8'?>
<doxygenindex>
  <compound kind="namespace"><name>erbsland::text</name>
    <member kind="typedef"><name>StringEditor</name></member>
    <member kind="enum"><name>StringEncoding</name></member>
    <member kind="function"><name>format</name></member>
    <member kind="function"><name>StringConverter</name></member>
    <member kind="function"><name>StringEncoder</name></member>
    <member kind="enum"><name>Mode</name></member>
  </compound>
  <compound kind="namespace"><name>erbsland::math</name>
    <member kind="enum"><name>Mode</name></member>
  </compound>
  <compound kind="class"><name>erbsland::text::U8StringEditor</name>
    <member kind="function"><name>append</name></member>
  </compound>
  <compound kind="class"><name>erbsland::text::StringConverter</name></compound>
  <compound kind="class"><name>erbsland::text::StringEncoder</name></compound>
  <compound kind="struct"><name>erbsland::unit::ByteUnit</name></compound>
  <compound kind="concept"><name>erbsland::math::IntegerType</name></compound>
  <compound kind="class"><name>erbsland::text::impl::PrivateThing</name></compound>
</doxygenindex>
""",
            encoding="utf-8",
        )
        self.config = CleanupRstConfig(
            project_dir=self.project_dir,
            line_width=88,
            doxygen_index=self.index_path,
        )
        self.warnings: list[str] = []
        self.index = DoxygenSymbolIndex.read(self.index_path)
        self.cleaner = RstCleaner(self.config, self.index, self.collect_warning)

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def collect_warning(self, path: Path, line_number: int, message: str) -> None:
        """Collect cleanup warnings."""
        self.warnings.append(f"{path.name}:{line_number}: {message}")

    def cleanup(self, text: str, path: str = "doc/sample.rst") -> str:
        """Clean up a test document."""
        return self.cleaner.cleanup_text(self.project_dir / path, text)

    def test_doxygen_index_parsing_and_role_mapping(self) -> None:
        self.assertEqual(("type", "erbsland::text::StringEditor"), self.symbol_tuple("StringEditor"))
        self.assertEqual(("class", "erbsland::text::U8StringEditor"), self.symbol_tuple("U8StringEditor"))
        self.assertEqual(("struct", "erbsland::unit::ByteUnit"), self.symbol_tuple("ByteUnit"))
        self.assertEqual(("enum", "erbsland::text::StringEncoding"), self.symbol_tuple("StringEncoding"))
        self.assertEqual(("func", "erbsland::text::format"), self.symbol_tuple("format"))
        self.assertEqual(("concept", "erbsland::math::IntegerType"), self.symbol_tuple("IntegerType"))
        self.assertFalse(self.index.resolve("PrivateThing"))

    def test_doxygen_deduction_guides_do_not_shadow_template_classes(self) -> None:
        self.assertEqual(("class", "erbsland::text::StringConverter"), self.symbol_tuple("StringConverter"))
        self.assertEqual(("class", "erbsland::text::StringEncoder"), self.symbol_tuple("StringEncoder"))

        result = self.cleanup("Use `StringConverter` and `StringEncoder` here.\n")

        self.assertEqual(
            "Use :cpp:class:`StringConverter <erbsland::text::StringConverter>` and\n"
            ":cpp:class:`StringEncoder <erbsland::text::StringEncoder>` here.\n",
            result,
        )

    def symbol_tuple(self, query: str) -> tuple[str, str]:
        """Resolve one symbol for compact assertions."""
        symbols = self.index.resolve(query)
        self.assertEqual(1, len(symbols))
        return symbols[0].role, symbols[0].full_name

    def test_inline_links_are_rewritten_with_correct_roles_and_targets(self) -> None:
        self.config = CleanupRstConfig(self.project_dir, 240, self.index_path)
        self.cleaner = RstCleaner(self.config, self.index, self.collect_warning)
        text = (
            "Use `StringEditor`, :cpp:class:`U8StringEditor`, :cpp:any:`format <erbsland::text::format>`, "
            "and :cpp:enum:`Text StringEditor <erbsland::text::StringEditor>`.\n"
        )

        result = self.cleanup(text)

        self.assertEqual(
            "Use :cpp:type:`StringEditor <erbsland::text::StringEditor>`, "
            ":cpp:class:`U8StringEditor <erbsland::text::U8StringEditor>`, "
            ":cpp:func:`format <erbsland::text::format>`, and "
            ":cpp:type:`Text StringEditor <erbsland::text::StringEditor>`.\n",
            result,
        )

    def test_member_suffix_links_are_resolved(self) -> None:
        result = self.cleanup("Use :cpp:any:`U8StringEditor::append` for appending text.\n")

        self.assertEqual(
            "Use :cpp:func:`U8StringEditor::append <erbsland::text::U8StringEditor::append>` for\n"
            "appending text.\n",
            result,
        )

    def test_missing_and_ambiguous_symbols_become_literals_with_warnings(self) -> None:
        result = self.cleanup("Use `Missing` and `Mode` here.\n")

        self.assertEqual("Use ``Missing`` and ``Mode`` here.\n", result)
        self.assertEqual(2, len(self.warnings))
        self.assertIn("No Doxygen symbol found for 'Missing'.", self.warnings[0])
        self.assertIn("Ambiguous Doxygen symbol for 'Mode'.", self.warnings[1])

    def test_reflows_simple_paragraphs_and_keeps_inline_markup_atomic(self) -> None:
        self.config = CleanupRstConfig(self.project_dir, 74, self.index_path)
        self.cleaner = RstCleaner(self.config, self.index, self.collect_warning)
        text = (
            "This paragraph references `StringEditor` and ``std::string`` while it keeps words "
            "wrapped within the configured width. Another sentence follows with ordinary text.\n"
        )

        result = self.cleanup(text)

        self.assertEqual(
            "This paragraph references\n"
            ":cpp:type:`StringEditor <erbsland::text::StringEditor>` and\n"
            "``std::string`` while it keeps words wrapped within the configured width.\n"
            "Another sentence follows with ordinary text.\n",
            result,
        )

    def test_sentence_break_exceptions_for_abbreviations_initials_and_numbers(self) -> None:
        self.config = CleanupRstConfig(self.project_dir, 240, self.index_path)
        self.cleaner = RstCleaner(self.config, self.index, self.collect_warning)
        text = (
            "Use etc. and vs. as abbreviations inside the sentence. "
            "Initials like E. T. A. stay together. "
            "Numbers like 3.14, 2.0 and 1. number stay together. "
            "The final sentence still breaks.\n"
        )

        result = self.cleanup(text)

        self.assertEqual(
            "Use etc. and vs. as abbreviations inside the sentence.\n"
            "Initials like E. T. A. stay together.\n"
            "Numbers like 3.14, 2.0 and 1. number stay together.\n"
            "The final sentence still breaks.\n",
            result,
        )

    def test_long_token_skips_reflow_but_keeps_link_cleanup(self) -> None:
        self.config = CleanupRstConfig(self.project_dir, 30, self.index_path)
        self.cleaner = RstCleaner(self.config, self.index, self.collect_warning)

        result = self.cleanup("Use `StringEditor` with a veryveryveryveryveryveryverylongword here.\n")

        self.assertEqual(
            "Use :cpp:type:`StringEditor <erbsland::text::StringEditor>` with a veryveryveryveryveryveryverylongword here.\n",
            result,
        )

    def test_skips_code_blocks_and_reflow_for_lists_and_tables(self) -> None:
        text = (
            ".. code-block:: cpp\n"
            "\n"
            "    auto value = `StringEditor`{};\n"
            "\n"
            "* Use `StringEditor` in a bullet that must not be joined with anything else.\n"
            "\n"
            ".. list-table::\n"
            "\n"
            "    * - :cpp:type:`StringEditor`\n"
        )

        result = self.cleanup(text)

        self.assertIn("    auto value = `StringEditor`{};\n", result)
        self.assertIn("* Use :cpp:type:`StringEditor <erbsland::text::StringEditor>` in a bullet", result)
        self.assertIn("    * - :cpp:type:`StringEditor <erbsland::text::StringEditor>`\n", result)

    def test_title_adornments_are_fixed(self) -> None:
        result = self.cleanup("*************\n" "Title Is Longer\n" "*************\n" "\n" "Small Title\n" "===\n")

        self.assertEqual(
            "***************\n" "Title Is Longer\n" "***************\n" "\n" "Small Title\n" "===========\n",
            result,
        )

    def test_file_selection_and_dry_run(self) -> None:
        doc_dir = self.project_dir / "doc"
        nested_dir = doc_dir / "nested"
        nested_dir.mkdir(parents=True)
        direct_file = doc_dir / "direct.rst"
        nested_file = nested_dir / "nested.rst"
        direct_original = "Use `StringEditor` here.\n"
        nested_original = "Use `StringEditor` there.\n"
        direct_file.write_text(direct_original, encoding="utf-8")
        nested_file.write_text(nested_original, encoding="utf-8")
        runner = CleanupRstRunner(self.config, dry_run=True, recursive=False)

        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            runner.run(doc_dir)

        self.assertEqual([direct_file], runner.changed_files)
        self.assertIn("would update: doc/direct.rst", output.getvalue())
        self.assertEqual(direct_original, direct_file.read_text(encoding="utf-8"))
        self.assertEqual(nested_original, nested_file.read_text(encoding="utf-8"))

        recursive_runner = CleanupRstRunner(self.config, dry_run=True, recursive=True)
        with contextlib.redirect_stdout(io.StringIO()):
            recursive_runner.run(doc_dir)
        self.assertEqual([direct_file, nested_file], recursive_runner.changed_files)


if __name__ == "__main__":
    unittest.main()
