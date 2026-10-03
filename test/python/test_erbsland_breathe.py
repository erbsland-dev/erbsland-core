# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

DOC_DIR = Path(__file__).resolve().parents[2] / "doc"
sys.path.insert(0, str(DOC_DIR / "_ext"))

from breathe.finder.factory import FinderFactory
from breathe.parser import DoxygenParserFactory
from breathe.renderer.filter import FilterFactory
from erbsland_breathe import CompoundIndexFinderFactory, SymbolFilterFactory


class IndexedBreatheLookupTest(unittest.TestCase):
    """Compare optimized lookup results against Breathe's original XML traversal."""

    def setUp(self) -> None:
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name).resolve()
        members = [("typedef", "Alias"), ("enum", "State"), ("function", "work"), ("function", "work")]
        index_members = "".join(
            f'<member kind="{kind}" refid="member{i}"><name>{name}</name></member>'
            for i, (kind, name) in enumerate(members)
        )
        self.root.joinpath("index.xml").write_text(
            '<doxygenindex><compound kind="namespace" refid="namespaceExample"><name>example</name>'
            + index_members
            + '</compound><compound kind="class" refid="classThing"><name>example::Thing</name></compound>'
            '<compound kind="class" refid="classOtherThing"><name>other::Thing</name></compound>'
            '<compound kind="group" refid="groupExample"><name>groupExample</name>'
            + index_members
            + '</compound><compound kind="file" refid="unrelated"><name>unrelated.hpp</name></compound>'
            "</doxygenindex>"
        )
        definitions = "".join(
            f'<memberdef kind="{kind}" id="member{i}"><name>{name}</name></memberdef>'
            for i, (kind, name) in enumerate(members)
        )
        self.root.joinpath("namespaceExample.xml").write_text(
            '<doxygen><compounddef kind="namespace" id="namespaceExample"><compoundname>example</compoundname>'
            '<sectiondef kind="func">' + definitions + "</sectiondef></compounddef></doxygen>"
        )
        for refid, kind in (
            ("classThing", "class"),
            ("classOtherThing", "class"),
            ("groupExample", "group"),
            ("unrelated", "file"),
        ):
            self.root.joinpath(f"{refid}.xml").write_text(
                f'<doxygen><compounddef kind="{kind}" id="{refid}"><compoundname>{refid}</compoundname>'
                "</compounddef></doxygen>"
            )
        self.app = SimpleNamespace(
            confdir=self.root,
            env=SimpleNamespace(docname="example"),
            config=SimpleNamespace(breathe_implementation_filename_extensions=[".cpp"]),
        )
        self.project = SimpleNamespace(project_path=lambda: str(self.root))
        self.filters = SymbolFilterFactory(self.app)

    def add_compound(self, kind: str, name: str, refid: str, members=()) -> None:
        """Add an owner with real index and compound XML for comparison tests."""
        index = ET.parse(self.root / "index.xml")
        compound = ET.SubElement(index.getroot(), "compound", kind=kind, refid=refid)
        ET.SubElement(compound, "name").text = name
        definition = ET.Element("doxygen")
        body = ET.SubElement(definition, "compounddef", kind=kind, id=refid)
        ET.SubElement(body, "compoundname").text = name
        section = ET.SubElement(body, "sectiondef", kind="func")
        for number, (member_kind, member_name) in enumerate(members):
            member_id = f"{refid}_{number}"
            member = ET.SubElement(compound, "member", kind=member_kind, refid=member_id)
            ET.SubElement(member, "name").text = member_name
            memberdef = ET.SubElement(section, "memberdef", kind=member_kind, id=member_id)
            ET.SubElement(memberdef, "name").text = member_name
        index.write(self.root / "index.xml")
        ET.ElementTree(definition).write(self.root / f"{refid}.xml")

    def compare_lookup(self, filter_, expected: int):
        parser = DoxygenParserFactory(self.app)
        fast_matches = []
        CompoundIndexFinderFactory(self.app, parser).create_finder(self.project).filter_(filter_, fast_matches)
        self.assertEqual(expected, len(fast_matches))
        self.assertNotIn(self.root / "unrelated.xml", parser.cache)
        self.assertNotIn(self.root / "classOtherThing.xml", parser.cache)
        original_matches = []
        FinderFactory(self.app, parser).create_finder(self.project).filter_(filter_, original_matches)
        # Shared parsed objects verify both identity/order and the complete ancestor stack.
        self.assertEqual([match[:-1] for match in original_matches], [match[:-1] for match in fast_matches])
        return fast_matches

    def test_index_lookups_match_original_including_overloads_and_missing_names(self) -> None:
        for filter_, expected in (
            (self.filters.create_compound_finder_filter("example::Thing", "class"), 1),
            (self.filters.create_compound_finder_filter("missing", "class"), 0),
            (self.filters.create_member_finder_filter("example", "Alias", "typedef"), 1),
            (self.filters.create_member_finder_filter("example", "State", "enum"), 1),
            (self.filters.create_function_and_all_friend_finder_filter("example", "work"), 2),
            (self.filters.create_function_and_all_friend_finder_filter("", "work"), 2),
        ):
            with self.subTest(expected=expected, filter=filter_):
                self.compare_lookup(filter_, expected)

    def test_duplicate_compounds_preserve_xml_order(self) -> None:
        self.add_compound("class", "example::Thing", "duplicateThing")
        matches = self.compare_lookup(self.filters.create_compound_finder_filter("example::Thing", "class"), 2)
        self.assertEqual(["classThing", "duplicateThing"], [match[0].refid for match in matches])

    def test_compound_kind_is_part_of_the_key(self) -> None:
        for kind in ("struct", "interface", "concept"):
            self.add_compound(kind, "example::Thing", f"{kind}Thing")
        for kind in ("class", "struct", "interface", "concept"):
            with self.subTest(kind=kind):
                matches = self.compare_lookup(self.filters.create_compound_finder_filter("example::Thing", kind), 1)
                self.assertEqual(f"{kind}Thing", matches[0][0].refid)

    def test_friends_overloads_groups_and_implementation_files_keep_breathe_semantics(self) -> None:
        # Put friends before functions in a later owner to catch kind-based reordering.
        self.add_compound("class", "example::Worker", "worker", [("friend", "work"), ("function", "work")])
        self.add_compound("group", "workGroup", "workGroup", [("function", "work")])
        self.add_compound("file", "work.cpp", "implementation", [("function", "work")])
        self.add_compound("file", "work.hpp", "header", [("function", "work")])
        self.compare_lookup(self.filters.create_function_and_all_friend_finder_filter("", "work"), 5)
        self.compare_lookup(self.filters.create_function_and_all_friend_finder_filter("example::Worker", "work"), 2)
        self.compare_lookup(self.filters.create_function_and_all_friend_finder_filter("example", "work"), 2)
        self.compare_lookup(self.filters.create_function_and_all_friend_finder_filter("workGroup", "work"), 0)
        self.compare_lookup(self.filters.create_member_finder_filter("missing", "work", "function"), 0)

    def test_reuses_maps_without_rescanning_index_for_each_directive(self) -> None:
        parser = DoxygenParserFactory(self.app)
        first = CompoundIndexFinderFactory(self.app, parser).create_finder(self.project)
        with patch.object(first.root(), "get_compound", side_effect=AssertionError("Index scanned again")):
            self.app.env.docname = "second_page"
            second = CompoundIndexFinderFactory(self.app, parser).create_finder(self.project)
            self.assertIs(first.symbol_index, second.symbol_index)
            matches = []
            second.filter_(self.filters.create_compound_finder_filter("example::Thing", "class"), matches)
            self.assertEqual(1, len(matches))
            matches = []
            second.filter_(self.filters.create_member_finder_filter("example", "Alias", "typedef"), matches)
            self.assertEqual(1, len(matches))
        self.assertEqual({"example", "second_page"}, self.app.env.breathe_file_state[str(self.root / "index.xml")][1])
        self.assertEqual({"second_page"}, self.app.env.breathe_file_state[str(self.root / "namespaceExample.xml")][1])

    def test_new_parsed_index_gets_new_maps(self) -> None:
        parser = DoxygenParserFactory(self.app)
        first = CompoundIndexFinderFactory(self.app, parser).create_finder(self.project)
        self.add_compound("class", "example::New", "newClass")
        new_parser = DoxygenParserFactory(self.app)
        second = CompoundIndexFinderFactory(self.app, new_parser).create_finder(self.project)
        self.assertIsNot(first.symbol_index, second.symbol_index)
        query = self.filters.create_compound_finder_filter("example::New", "class")
        old_matches, new_matches = [], []
        first.filter_(query, old_matches)
        second.filter_(query, new_matches)
        self.assertEqual([], old_matches)
        self.assertEqual(1, len(new_matches))

    def test_projects_with_identical_names_have_separate_maps(self) -> None:
        parser = DoxygenParserFactory(self.app)
        first = CompoundIndexFinderFactory(self.app, parser).create_finder(self.project)
        other_dir = self.root / "other-project"
        other_dir.mkdir()
        (other_dir / "index.xml").write_bytes((self.root / "index.xml").read_bytes())
        other = SimpleNamespace(project_path=lambda: str(other_dir))
        second = CompoundIndexFinderFactory(self.app, parser).create_finder(other)
        self.assertIsNot(first.symbol_index, second.symbol_index)
        query = self.filters.create_compound_finder_filter("example::Thing", "class")
        first_matches, second_matches = [], []
        first.filter_(query, first_matches)
        second.filter_(query, second_matches)
        self.assertIsNot(first_matches[0][0], second_matches[0][0])

    def test_untagged_filter_falls_back_to_original_traversal(self) -> None:
        parser = DoxygenParserFactory(self.app)
        finder = CompoundIndexFinderFactory(self.app, parser).create_finder(self.project)
        matches = []
        finder.filter_(FilterFactory(self.app).create_compound_finder_filter("example::Thing", "class"), matches)
        self.assertEqual(1, len(matches))
        self.assertIn(self.root / "unrelated.xml", parser.cache)

    def test_does_not_need_unrelated_xml_and_tracks_selected_dependencies(self) -> None:
        (self.root / "unrelated.xml").unlink()
        parser = DoxygenParserFactory(self.app)
        finder = CompoundIndexFinderFactory(self.app, parser).create_finder(self.project)
        matches = []
        finder.filter_(self.filters.create_member_finder_filter("example", "Alias", "typedef"), matches)
        self.assertEqual(1, len(matches))
        self.assertEqual(
            {str(self.root / "index.xml"), str(self.root / "namespaceExample.xml")},
            set(self.app.env.breathe_file_state),
        )


if __name__ == "__main__":
    unittest.main()
