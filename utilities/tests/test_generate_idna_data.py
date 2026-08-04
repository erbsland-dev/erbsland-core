# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from idna import idnadata

from dev.generate_idna_data import GenerateIdnaDataApp


def decode_ranges(values: tuple[int, ...], property_value: int) -> set[tuple[int, int, int]]:
    """Decode idna-package ranges for direct generator comparison."""
    return {(value >> 32, (value & 0xFFFFFFFF) - 1, property_value) for value in values}


def generated_set(values) -> set[tuple[int, int, int]]:
    """Convert generator ranges into comparable tuples."""
    return {(value.first, value.last, value.value) for value in values}


class IdnaDataGeneratorTest(unittest.TestCase):
    """Tests for the Unicode 17 IDNA2008 data generator."""

    @staticmethod
    def make_app() -> GenerateIdnaDataApp:
        """Create a generator using the checked-in Unicode source directory."""
        app = GenerateIdnaDataApp()
        app.data_dir = Path(__file__).resolve().parents[1] / "data"
        return app

    def test_unicode_17_status_ranges_match_independent_idna_package(self) -> None:
        """The complete derived-status partition matches idna 3.18 / Unicode 17."""
        app = self.make_app()
        expected: set[tuple[int, int, int]] = set()
        for name, value in (("PVALID", 1), ("CONTEXTJ", 2), ("CONTEXTO", 3)):
            expected |= decode_ranges(idnadata.codepoint_classes[name], value)
        self.assertEqual(generated_set(app.status_ranges()), expected)

    def test_joining_types_match_independent_idna_package(self) -> None:
        """The locally derived Joining_Type ranges match the independent tables."""
        app = self.make_app()
        categories, _, _ = app.read_unicode_data()
        expected: set[tuple[int, int, int]] = set()
        for name, value in (("L", 1), ("R", 2), ("D", 3), ("T", 4)):
            expected |= decode_ranges(idnadata.joining_types[name], value)
        self.assertEqual(generated_set(app.read_joining_types(categories)), expected)

    def test_contextual_scripts_match_independent_idna_package(self) -> None:
        """Every script range required by CONTEXTO matches the independent tables."""
        app = self.make_app()
        scripts = app.read_scripts()
        values = {"Greek": 1, "Han": 2, "Hebrew": 3, "Hiragana": 4, "Katakana": 5}
        for name, value in values.items():
            self.assertEqual(
                generated_set(app.merge_ranges(scripts[name])), decode_ranges(idnadata.scripts[name], value)
            )

    def test_runtime_ranges_are_compact_and_complete(self) -> None:
        """The merged runtime representation retains every attribute required for valid code points."""
        app = self.make_app()
        data = app.property_ranges()
        ranges = app.runtime_ranges(data)
        page_offsets = app.page_offsets(ranges)

        self.assertEqual(len(ranges), 1840)
        self.assertEqual(page_offsets, [0, 1312, 1833, 1838, 1840])
        self.assertEqual(len(ranges) * 6 + len(page_offsets) * 2, 11050)

        expected: dict[int, int] = {}
        for value in data["idnaStatusRanges"]:
            for code_point in range(value.first, value.last + 1):
                expected[code_point] = value.value
        properties = (
            ("idnaBidiRanges", app.BIDI_SHIFT),
            ("idnaJoiningRanges", app.JOINING_SHIFT),
            ("idnaViramaRanges", app.VIRAMA_SHIFT),
            ("idnaGreekRanges", app.SCRIPT_SHIFT),
            ("idnaHanRanges", app.SCRIPT_SHIFT),
            ("idnaHebrewRanges", app.SCRIPT_SHIFT),
            ("idnaHiraganaRanges", app.SCRIPT_SHIFT),
            ("idnaKatakanaRanges", app.SCRIPT_SHIFT),
        )
        for name, shift in properties:
            for value in data[name]:
                for code_point in range(value.first, value.last + 1):
                    if code_point in expected:
                        expected[code_point] |= value.value << shift

        actual: dict[int, int] = {}
        previous_last = -1
        for value in ranges:
            self.assertGreater(value.first, previous_last)
            self.assertEqual(value.first >> 16, value.last >> 16)
            self.assertNotEqual(value.value & 0x0003, 0)
            for code_point in range(value.first, value.last + 1):
                actual[code_point] = value.value
            previous_last = value.last
        self.assertEqual(actual, expected)


if __name__ == "__main__":
    unittest.main()
