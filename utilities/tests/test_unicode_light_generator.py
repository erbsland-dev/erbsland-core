# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from dev.generate_unicode_light_data import GenerateUnicodeLightDataApp


class UnicodeLightGeneratorTest(unittest.TestCase):
    """Tests for the generated Unicode Light data shape."""

    def create_app(self) -> GenerateUnicodeLightDataApp:
        app = GenerateUnicodeLightDataApp()
        app.parse_command_line([])
        return app

    def test_display_width_data(self) -> None:
        app = self.create_app()

        widths = app.read_display_widths()

        self.assertEqual(widths[ord("A")], 1)
        self.assertEqual(widths[ord("\n")], 0)
        self.assertEqual(widths[0x00AD], 0)
        self.assertEqual(widths[0x0301], 0)
        self.assertEqual(widths[0x2500], 1)
        self.assertEqual(widths[0x4E00], 2)
        self.assertEqual(widths[0x1F600], 2)
        self.assertEqual(widths[0xD800], 1)

    def test_merged_delta_and_width_budget(self) -> None:
        app = self.create_app()

        general_categories, lowercase_deltas, uppercase_deltas = app.read_unicode_data()
        case_fold_deltas = app.read_case_fold_deltas()
        display_widths = app.read_display_widths()
        delta_indices, delta_table = app.unique_delta_table(case_fold_deltas, lowercase_deltas, uppercase_deltas)
        ranges = app.compress_to_ranges(general_categories, delta_indices, display_widths)
        non_ascii_entries = [entry for entry in ranges if entry.code_point >= 0x80]

        self.assertLessEqual(len(delta_table), 256)
        self.assertLessEqual(len(non_ascii_entries), 4451)
        self.assertLessEqual(128 * 8 + len(non_ascii_entries) * 8 + len(delta_table) * 12, 39_000)


if __name__ == "__main__":
    unittest.main()
