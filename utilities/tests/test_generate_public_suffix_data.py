# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from dev.generate_public_suffix_data import GeneratePublicSuffixDataApp


class PublicSuffixDataGeneratorTest(unittest.TestCase):
    """Tests for the pinned complete Public Suffix List generator."""

    @staticmethod
    def make_app() -> GeneratePublicSuffixDataApp:
        """Create a generator targeting the checked-in source and generated table."""
        project_directory = Path(__file__).resolve().parents[2]
        app = GeneratePublicSuffixDataApp()
        app.input_path = project_directory / "utilities" / "data" / "public_suffix" / "public_suffix_list.dat"
        app.output_path = (
            project_directory / "src" / "erbsland" / "network" / "impl" / "http" / "cookie" / "PublicSuffixData.cpp"
        )
        return app

    def test_complete_snapshot_contains_all_rule_families(self) -> None:
        """The pinned list includes its version plus large ICANN/private and special-rule sets."""
        version, exact, wildcard, exception = self.make_app().parse()
        self.assertEqual(version, "2026-08-19_19-18-48_UTC")
        self.assertGreater(len(exact), 9000)
        self.assertGreater(len(wildcard), 200)
        self.assertGreater(len(exception), 5)
        self.assertIn("blogspot.com", exact)
        self.assertIn("ck", wildcard)
        self.assertIn("www.ck", exception)

    def test_generated_table_is_fresh(self) -> None:
        """Regenerating the table from the pinned source produces the checked-in file exactly."""
        app = self.make_app()
        version, exact, wildcard, exception = app.parse()
        data = app.encode(exact, wildcard, exception)
        self.assertEqual(app.output_path.read_text(encoding="utf-8"), app.render(version, data))

    def test_compact_encoding_round_trips_every_rule(self) -> None:
        """The compressed stream preserves every rule and its family exactly."""
        app = self.make_app()
        _, exact, wildcard, exception = app.parse()
        data = app.encode(exact, wildcard, exception)
        decoded = {(".".join(reversed(rule.split("."))), token) for rule, token in data.rules}
        expected = {(rule, app.exact_token) for rule in exact}
        expected |= {(rule, app.wildcard_token) for rule in wildcard}
        expected |= {(rule, app.exception_token) for rule in exception}
        self.assertEqual(decoded, expected)
        self.assertEqual(len(data.rules), len(expected))

    def test_compact_encoding_reduces_storage_by_at_least_75_percent(self) -> None:
        """The persistent payload meets the footprint target on 32-bit and 64-bit systems."""
        app = self.make_app()
        _, exact, wildcard, exception = app.parse()
        data = app.encode(exact, wildcard, exception)
        rule_count = len(exact) + len(wildcard) + len(exception)
        legacy_32_size = data.original_rule_text_byte_count + rule_count * 8
        legacy_64_size = data.original_rule_text_byte_count + rule_count * 16
        self.assertLessEqual(data.encoded_storage_byte_count * 4, legacy_32_size)
        self.assertLessEqual(data.encoded_storage_byte_count * 4, legacy_64_size)
        self.assertLess(data.encoded_storage_byte_count, 45_000)


if __name__ == "__main__":
    unittest.main()
