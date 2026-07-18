# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from lib.cleanup import CleanupConfig, IncludeCleanup


class IncludeCleanupTest(unittest.TestCase):
    """Tests for sorting include blocks in configured source roots."""

    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory(dir="/private/tmp")
        self.project_dir = Path(self.temp_dir.name)
        (self.project_dir / "src" / "erbsland").mkdir(parents=True)
        (self.project_dir / "test" / "unittest" / "src").mkdir(parents=True)
        (self.project_dir / "demos").mkdir(parents=True)

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def write_file(self, relative_path: str, text: str) -> None:
        """Write a text file below the temporary project."""
        path = self.project_dir / relative_path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def read_file(self, relative_path: str) -> str:
        """Read a text file below the temporary project."""
        return (self.project_dir / relative_path).read_text(encoding="utf-8")

    def run_include_cleanup(self) -> None:
        """Run only the include cleanup pass in the temporary project."""
        config = CleanupConfig.read(self.project_dir, Path(__file__).resolve().parents[1] / "conf" / "cleanup.elcl")
        IncludeCleanup(config, lambda _: None).run()

    def test_unit_test_includes_are_sorted_and_allow_erbsland_globals(self) -> None:
        self.write_file(
            "test/unittest/src/sample/WidgetTest.cpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0

#include <vector>
#include <erbsland/unittest/UnitTest.hpp>
#include "../support/TestHelper.hpp"
""",
        )

        self.run_include_cleanup()

        self.assertEqual(
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0

#include "../support/TestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

#include <vector>

""",
            self.read_file("test/unittest/src/sample/WidgetTest.cpp"),
        )

    def test_demo_includes_are_sorted_and_allow_erbsland_globals(self) -> None:
        self.write_file(
            "demos/text/Sample/main.cpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0

#include <vector>
#include <erbsland/text/StringEditor.hpp>
#include "SampleDemos.hpp"
#include <DemoCommon.hpp>
""",
        )

        self.run_include_cleanup()

        self.assertEqual(
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0

#include "SampleDemos.hpp"

#include <DemoCommon.hpp>
#include <erbsland/text/StringEditor.hpp>

#include <vector>

""",
            self.read_file("demos/text/Sample/main.cpp"),
        )


if __name__ == "__main__":
    unittest.main()
