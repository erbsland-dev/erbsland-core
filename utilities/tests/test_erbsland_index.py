# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import sys
import unittest
from pathlib import Path

from docutils.utils import new_document
from sphinx import addnodes

project_dir = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(project_dir / "doc" / "_ext"))

from erbsland_index import ErbslandIndexTransform, strip_erbsland_cpp_index_value


class ErbslandIndexTest(unittest.TestCase):
    """Tests for Erbsland Core generated C++ index entry rewriting."""

    def test_strips_erbsland_cpp_index_values_to_leaf_names(self) -> None:
        cases = {
            "erbsland::text::StringEditor (C++ type)": "StringEditor (C++ type)",
            "erbsland::time::IsoTimeFormat::Extended (C++ enumerator)": "Extended (C++ enumerator)",
            "erbsland::text::U8StringEditor::append (C++ function)": "append (C++ function)",
            "erbsland::stream::io::writeLine (C++ function)": "writeLine (C++ function)",
        }

        for value, expected in cases.items():
            with self.subTest(value=value):
                self.assertEqual(expected, strip_erbsland_cpp_index_value(value))

    def test_leaves_non_matching_index_values_unchanged(self) -> None:
        cases = [
            "StringEditor Types",
            "erbsland::text::StringEditor",
            "std::string (C++ class)",
            "not_erbsland::text::StringEditor (C++ type)",
        ]

        for value in cases:
            with self.subTest(value=value):
                self.assertEqual(value, strip_erbsland_cpp_index_value(value))

    def test_transform_rewrites_only_generated_cpp_index_entries(self) -> None:
        document = new_document("test.rst")
        index_node = addnodes.index(
            entries=[
                ("single", "erbsland::text::StringEditor (C++ type)", "_CPPv4N8erbsland4text6StringE", "", None),
                (
                    "single",
                    "erbsland::text::U8StringEditor::append (C++ function)",
                    "_CPPv4N8erbsland4text8U8String6appendE4Char",
                    "",
                    None,
                ),
                (
                    "single",
                    "erbsland::bgeo::Alignment::~Alignment (C++ function)",
                    "_CPPv4N8erbsland4bgeo9AlignmentD0Ev",
                    "",
                    None,
                ),
                (
                    "single",
                    "erbsland::bgeo::Alignment::Alignment (C++ function)",
                    "_CPPv4N8erbsland4bgeo9Alignment9AlignmentEv",
                    "",
                    None,
                ),
                (
                    "single",
                    "erbsland::stream::io::writeLine (C++ function)",
                    "_CPPv4N8erbsland6stream2io9writeLineEv",
                    "",
                    None,
                ),
                (
                    "single",
                    "erbsland::time::IsoTimeFormat::Extended (C++ enumerator)",
                    "_CPPv4N8erbsland4time13IsoTimeFormat8ExtendedE",
                    "main",
                    "category",
                ),
                (
                    "single",
                    "erbsland::bgeo::Alignment::Bottom (C++ member)",
                    "_CPPv4N8erbsland4bgeo9Alignment6BottomE",
                    "",
                    None,
                ),
                ("single", "erbsland::text::Manual (C++ type)", "index-0", "", None),
                ("single", "std::string (C++ class)", "_CPPv4NSt6stringE", "", None),
                ("pair", "erbsland::text::Pair (C++ type); pair", "_CPPv4N8erbsland4text4PairE", "", None),
            ]
        )
        document += index_node

        ErbslandIndexTransform(document).apply()

        self.assertEqual(
            [
                ("single", "StringEditor (C++ type)", "_CPPv4N8erbsland4text6StringE", "", None),
                ("single", "append; U8StringEditor", "_CPPv4N8erbsland4text8U8String6appendE4Char", "", None),
                ("single", "writeLine (C++ function)", "_CPPv4N8erbsland6stream2io9writeLineEv", "", None),
                (
                    "single",
                    "Extended; IsoTimeFormat",
                    "_CPPv4N8erbsland4time13IsoTimeFormat8ExtendedE",
                    "main",
                    "category",
                ),
                ("single", "Bottom; Alignment", "_CPPv4N8erbsland4bgeo9Alignment6BottomE", "", None),
                ("single", "erbsland::text::Manual (C++ type)", "index-0", "", None),
                ("single", "std::string (C++ class)", "_CPPv4NSt6stringE", "", None),
                ("pair", "erbsland::text::Pair (C++ type); pair", "_CPPv4N8erbsland4text4PairE", "", None),
            ],
            index_node["entries"],
        )


if __name__ == "__main__":
    unittest.main()
