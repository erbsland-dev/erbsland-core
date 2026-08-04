# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import importlib.util
import os
import tempfile
import time
import unittest
from pathlib import Path


def load_preprocess_module():
    """Load the documentation preprocessing tool as a test module."""
    module_path = Path(__file__).resolve().parents[2] / "doc" / "_tools" / "preprocess_sources.py"
    spec = importlib.util.spec_from_file_location("preprocess_sources", module_path)
    if spec is None or spec.loader is None:
        raise RuntimeError("Could not load preprocess_sources.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class PreprocessSourcesTest(unittest.TestCase):
    """Tests for the standalone Doxygen source preprocessing tool."""

    def setUp(self) -> None:
        self.module = load_preprocess_module()
        self.temp_dir = tempfile.TemporaryDirectory(dir="/private/tmp")
        self.project_dir = Path(self.temp_dir.name)
        self.source_dir = self.project_dir / "src"
        self.output_dir = self.project_dir / "_doxygen_input"
        self.conf_path = self.project_dir / "doc" / "conf.py"
        self.conf_path.parent.mkdir(parents=True)
        self.conf_path.write_text("# config\n", encoding="utf-8")
        (self.source_dir / "erbsland").mkdir(parents=True)
        (self.source_dir / "erbsland" / "Value.hpp").write_text(
            "auto value() -> int;\n" "#define ERBSLAND_CORE_TEST 1\n",
            encoding="utf-8",
        )
        (self.source_dir / "erbsland" / "all.hpp").write_text("// generated\n", encoding="utf-8")

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def processor(self, *, force: bool = False):
        """Create a preprocessor instance scoped to the temp project."""
        processor = self.module.SourcePreprocessor(force=force)
        processor.project_dir = self.project_dir
        processor.conf_path = self.conf_path
        processor.source_dir = self.source_dir
        processor.output_dir = self.output_dir
        processor.marker_path = self.output_dir / self.module.MARKER_FILE_NAME
        return processor

    def test_initial_run_creates_processed_tree_and_skips_aggregate_headers(self) -> None:
        changed = self.processor().run()

        self.assertTrue(changed)
        self.assertTrue((self.output_dir / self.module.MARKER_FILE_NAME).is_file())
        self.assertTrue((self.output_dir / "erbsland" / "Value.hpp").is_file())
        self.assertFalse((self.output_dir / "erbsland" / "all.hpp").exists())
        content = (self.output_dir / "erbsland" / "Value.hpp").read_text(encoding="utf-8")
        self.assertIn("int value() ;", content)
        self.assertIn("// macro removed", content)

    def test_unchanged_run_is_skipped(self) -> None:
        self.assertTrue(self.processor().run())

        self.assertFalse(self.processor().run())

    def test_source_mtime_change_rebuilds(self) -> None:
        self.assertTrue(self.processor().run())
        source_path = self.source_dir / "erbsland" / "Value.hpp"
        new_time = time.time() + 10
        os.utime(source_path, (new_time, new_time))

        self.assertTrue(self.processor().run())

    def test_added_and_deleted_source_changes_inventory(self) -> None:
        self.assertTrue(self.processor().run())
        extra_path = self.source_dir / "erbsland" / "Extra.hpp"
        extra_path.write_text("auto extra() -> int;\n", encoding="utf-8")
        self.assertTrue(self.processor().run())

        extra_path.unlink()
        self.assertTrue(self.processor().run())

    def test_conf_hash_change_rebuilds(self) -> None:
        self.assertTrue(self.processor().run())
        self.conf_path.write_text("# config changed\n", encoding="utf-8")

        self.assertTrue(self.processor().run())

    def test_force_rebuilds_even_when_unchanged(self) -> None:
        self.assertTrue(self.processor().run())

        self.assertTrue(self.processor(force=True).run())

    def test_trailing_return_preserves_override_on_pure_virtual_method(self) -> None:
        result = self.processor().transform_source("auto events() -> Editor & override = 0;\n")

        self.assertEqual(result, "Editor & events() override = 0;\n")

    def test_trailing_return_preserves_method_qualifiers(self) -> None:
        source = "auto first() -> int override;\nauto second() -> int final override;\n"
        result = self.processor().transform_source(source)

        self.assertEqual(result, "int first() override;\nint second() final override;\n")


if __name__ == "__main__":
    unittest.main()
