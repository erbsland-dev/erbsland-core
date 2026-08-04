# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from dev.generate_unicode_normalization_data import DecompositionMapping, GenerateUnicodeNormalizationDataApp
from lib.error import UtilityError


class UnicodeNormalizationGeneratorTest(unittest.TestCase):
    """Tests for the compact Unicode normalization data generator."""

    def create_app(self) -> GenerateUnicodeNormalizationDataApp:
        app = GenerateUnicodeNormalizationDataApp()
        app.parse_command_line([])
        return app

    @staticmethod
    def decode_unsigned(data: bytes, offset: int) -> tuple[int, int]:
        value = 0
        shift = 0
        while True:
            byte = data[offset]
            offset += 1
            value |= (byte & 0x7F) << shift
            if not byte & 0x80:
                return value, offset
            shift += 7

    def decode_decompositions(
        self, app: GenerateUnicodeNormalizationDataApp, directory: tuple[int, ...], data: bytes
    ) -> list[DecompositionMapping]:
        result: list[DecompositionMapping] = []
        for page_index, directory_entry in enumerate(directory):
            page = directory_entry & app.PAGE_MASK
            offset = directory_entry >> app.PAGE_BITS
            end = directory[page_index + 1] >> app.PAGE_BITS if page_index + 1 < len(directory) else len(data)
            code_point = page << app.PAGE_SHIFT
            while offset < end:
                delta, offset = self.decode_unsigned(data, offset)
                code_point += delta
                header = data[offset]
                offset += 1
                self.assertEqual(header & 0xC0, 0, "decomposition header reserved bits must be zero")
                length = (header & 0x1F) + 1
                mapping: list[int] = []
                for _ in range(length):
                    value, offset = self.decode_unsigned(data, offset)
                    mapping.append(value)
                result.append(DecompositionMapping(code_point, tuple(mapping), bool(header & 0x20)))
            self.assertEqual(offset, end)
        return result

    def decode_combining_classes(
        self, app: GenerateUnicodeNormalizationDataApp, directory: tuple[int, ...], data: bytes
    ) -> dict[int, int]:
        result: dict[int, int] = {}
        for page_index, directory_entry in enumerate(directory):
            page = directory_entry & app.PAGE_MASK
            offset = directory_entry >> app.PAGE_BITS
            end = directory[page_index + 1] >> app.PAGE_BITS if page_index + 1 < len(directory) else len(data)
            code_point = page << app.PAGE_SHIFT
            while offset < end:
                delta, offset = self.decode_unsigned(data, offset)
                code_point += delta
                result[code_point] = data[offset]
                offset += 1
            self.assertEqual(offset, end)
        return result

    def decode_compositions(
        self, app: GenerateUnicodeNormalizationDataApp, directory: tuple[int, ...], data: bytes
    ) -> dict[int, list[tuple[int, int]]]:
        result: dict[int, list[tuple[int, int]]] = {}
        for page_index, directory_entry in enumerate(directory):
            page = directory_entry & app.PAGE_MASK
            offset = directory_entry >> app.PAGE_BITS
            end = directory[page_index + 1] >> app.PAGE_BITS if page_index + 1 < len(directory) else len(data)
            starter = page << app.PAGE_SHIFT
            while offset < end:
                delta, offset = self.decode_unsigned(data, offset)
                starter += delta
                pair_count, offset = self.decode_unsigned(data, offset)
                trailing = 0
                pairs: list[tuple[int, int]] = []
                for _ in range(pair_count):
                    trailing_delta, offset = self.decode_unsigned(data, offset)
                    trailing += trailing_delta
                    composite, offset = self.decode_unsigned(data, offset)
                    pairs.append((trailing, composite))
                result[starter] = pairs
            self.assertEqual(offset, end)
        return result

    def test_uleb128_round_trip_and_canonical_encoding(self) -> None:
        app = self.create_app()
        for value in (0, 1, 0x7F, 0x80, 0x3FFF, 0x4000, 0x10FFFF, app.MAX_STREAM_OFFSET):
            encoded = app.encode_unsigned(value)
            decoded, offset = self.decode_unsigned(encoded, 0)
            self.assertEqual(decoded, value)
            self.assertEqual(offset, len(encoded))
            self.assertFalse(len(encoded) > 1 and encoded[-1] == 0)

    def test_unicode_17_data_shape_and_budgets(self) -> None:
        app = self.create_app()
        mappings, combining_classes = app.read_unicode_data()
        exclusions = app.read_composition_exclusions()
        compositions = app.build_compositions(mappings, combining_classes, exclusions)
        data = app.generate_data()

        self.assertEqual(len(mappings), 5914)
        self.assertEqual(len(combining_classes), 968)
        self.assertEqual(sum(len(values) for values in compositions.values()), 961)
        self.assertLessEqual(len(data.decomposition_data), app.MAX_DECOMPOSITION_SIZE)
        self.assertLessEqual(len(data.combining_class_data), app.MAX_COMBINING_CLASS_SIZE)
        self.assertLessEqual(len(data.composition_data), app.MAX_COMPOSITION_SIZE)
        self.assertLessEqual(data.total_size, app.MAX_TOTAL_SIZE)

    def test_encoded_streams_round_trip_to_source_data(self) -> None:
        app = self.create_app()
        mappings, combining_classes = app.read_unicode_data()
        exclusions = app.read_composition_exclusions()
        compositions = app.build_compositions(mappings, combining_classes, exclusions)
        data = app.generate_data()

        self.assertEqual(self.decode_decompositions(app, data.decomposition_pages, data.decomposition_data), mappings)
        self.assertEqual(
            self.decode_combining_classes(app, data.combining_class_pages, data.combining_class_data),
            combining_classes,
        )
        self.assertEqual(self.decode_compositions(app, data.composition_pages, data.composition_data), compositions)

    def test_unicode_17_runtime_limits(self) -> None:
        app = self.create_app()
        mappings, combining_classes = app.read_unicode_data()
        exclusions = app.read_composition_exclusions()
        compositions = app.build_compositions(mappings, combining_classes, exclusions)
        limits = app.validate_runtime_limits(mappings, combining_classes, compositions)

        self.assertEqual(limits.maximum_decomposition_length, 18)
        self.assertEqual(limits.maximum_decomposition_depth, 3)
        self.assertEqual(limits.maximum_composition_chain, 3)
        self.assertEqual(limits.minimum_window_length, 48)

    def test_runtime_decomposition_length_is_bounded(self) -> None:
        mapping = DecompositionMapping(0x1000, tuple(range(0x20, 0x20 + 19)), False)
        with self.assertRaises(UtilityError):
            GenerateUnicodeNormalizationDataApp.validate_runtime_limits([mapping], {}, {})

    def test_runtime_decomposition_depth_is_bounded(self) -> None:
        mappings = [
            DecompositionMapping(0x1000, (0x1001,), False),
            DecompositionMapping(0x1001, (0x1002,), False),
            DecompositionMapping(0x1002, (0x1003,), False),
            DecompositionMapping(0x1003, (0x0041,), False),
        ]
        with self.assertRaises(UtilityError):
            GenerateUnicodeNormalizationDataApp.validate_runtime_limits(mappings, {}, {})

    def test_runtime_rejects_nonstarter_to_starter_expansion(self) -> None:
        mappings = [DecompositionMapping(0x1000, (0x0301, 0x0041), False)]
        with self.assertRaises(UtilityError):
            GenerateUnicodeNormalizationDataApp.validate_runtime_limits(mappings, {0x0301: 230}, {})

    def test_runtime_composition_chain_is_bounded(self) -> None:
        compositions = {code_point: [(0x0041, code_point + 1)] for code_point in range(0x1000, 0x1022)}
        with self.assertRaises(UtilityError):
            GenerateUnicodeNormalizationDataApp.validate_runtime_limits([], {}, compositions)

    def test_runtime_window_length_is_bounded(self) -> None:
        app_type = GenerateUnicodeNormalizationDataApp
        original_length = app_type.MAX_RUNTIME_WINDOW_LENGTH
        app_type.MAX_RUNTIME_WINDOW_LENGTH = 47
        try:
            mapping = DecompositionMapping(0x1000, tuple(range(0x20, 0x20 + 18)), False)
            with self.assertRaises(UtilityError):
                app_type.validate_runtime_limits([mapping], {}, {})
        finally:
            app_type.MAX_RUNTIME_WINDOW_LENGTH = original_length

    def test_hangul_is_not_stored_in_decomposition_table(self) -> None:
        app = self.create_app()
        mappings, _ = app.read_unicode_data()
        sources = {value.code_point for value in mappings}

        self.assertNotIn(0xAC00, sources)
        self.assertNotIn(0xD7A3, sources)

    def test_compatibility_tags_and_composition_exclusions(self) -> None:
        app = self.create_app()
        mappings, combining_classes = app.read_unicode_data()
        exclusions = app.read_composition_exclusions()
        mapping_by_code_point = {value.code_point: value for value in mappings}
        compositions = app.build_compositions(mappings, combining_classes, exclusions)

        self.assertFalse(mapping_by_code_point[0x00C5].compatibility)
        self.assertTrue(mapping_by_code_point[0xFB03].compatibility)
        self.assertIn((0x030A, 0x00C5), compositions[0x0041])
        self.assertNotIn((0x093C, 0x0958), compositions.get(0x0915, []))

    def test_page_boundaries_are_sparse_and_sorted(self) -> None:
        app = self.create_app()
        data = app.generate_data()
        for directory in (
            data.decomposition_pages,
            data.combining_class_pages,
            data.composition_pages,
        ):
            pages = [entry & app.PAGE_MASK for entry in directory]
            offsets = [entry >> app.PAGE_BITS for entry in directory]
            self.assertEqual(pages, sorted(set(pages)))
            self.assertEqual(offsets, sorted(offsets))
            self.assertTrue(all(page <= 0x10F for page in pages))

        boundary_mappings = [
            DecompositionMapping(0x0FFF, (0x0041,), False),
            DecompositionMapping(0x1000, (0x0042,), True),
        ]
        pages, stream = app.encode_decompositions(boundary_mappings)
        self.assertEqual([entry & app.PAGE_MASK for entry in pages], [0, 1])
        self.assertEqual(self.decode_decompositions(app, pages, stream), boundary_mappings)

    def test_malformed_source_data_is_rejected(self) -> None:
        app = self.create_app()
        with tempfile.TemporaryDirectory() as directory:
            malformed_path = Path(directory) / "UnicodeData.txt"
            malformed_path.write_text("0041;LATIN CAPITAL LETTER A\n", encoding="utf-8")
            app.unicode_data_path = malformed_path
            with self.assertRaises(UtilityError):
                app.read_unicode_data()

    def test_cyclic_decomposition_is_rejected(self) -> None:
        mappings = [
            DecompositionMapping(0x0041, (0x0042,), False),
            DecompositionMapping(0x0042, (0x0041,), False),
        ]
        with self.assertRaises(UtilityError):
            GenerateUnicodeNormalizationDataApp.validate_decomposition_graph(mappings)


if __name__ == "__main__":
    unittest.main()
