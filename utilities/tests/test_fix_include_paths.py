# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from dev.fix_include_paths import FixIncludePathsApp
from lib.error import UtilityError


class TestFixIncludePathsApp(FixIncludePathsApp):
    """Fix-include-paths app bound to an isolated project directory."""

    def __init__(self, project_dir: Path) -> None:
        super().__init__()
        self._project_dir = project_dir

    @property
    def project_directory(self) -> Path:
        """Use the temporary test project as the project root."""
        return self._project_dir

    def config_file_path(self) -> Path:
        """Use the production fix-include-paths configuration."""
        return Path(__file__).resolve().parents[1] / "conf" / "fix_include_paths.elcl"


class FixIncludePathsTest(unittest.TestCase):
    """Tests for fixing C++ include paths."""

    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory(dir="/private/tmp")
        self.project_dir = Path(self.temp_dir.name)
        (self.project_dir / "src" / "erbsland").mkdir(parents=True)
        (self.project_dir / "test" / "unittest" / "src").mkdir(parents=True)
        (self.project_dir / "demos" / "_common" / "src").mkdir(parents=True)

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

    def run_fix_include_paths(self) -> None:
        """Run the include path fixer in the temporary project."""
        TestFixIncludePathsApp(self.project_dir).run([])

    def test_main_sources_use_shortest_relative_path(self) -> None:
        self.write_file(
            "src/erbsland/other/Thing.hpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0
#pragma once
""",
        )
        self.write_file(
            "src/erbsland/sample/Widget.cpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0

#include "erbsland/other/Thing.hpp"

#include <vector>
""",
        )

        self.run_fix_include_paths()

        text = self.read_file("src/erbsland/sample/Widget.cpp")
        self.assertIn('#include "../other/Thing.hpp"', text)
        self.assertNotIn('#include "erbsland/other/Thing.hpp"', text)
        self.assertIn("#include <vector>", text)

    def test_main_sources_report_ambiguous_filename(self) -> None:
        self.write_file(
            "src/erbsland/one/Shared.hpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0
#pragma once
""",
        )
        self.write_file(
            "src/erbsland/two/Shared.hpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0
#pragma once
""",
        )
        self.write_file(
            "src/erbsland/sample/Widget.cpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0

#include "Shared.hpp"
""",
        )

        with self.assertRaisesRegex(UtilityError, "Ambiguous include path"):
            self.run_fix_include_paths()

    def test_main_sources_use_unique_suffix_when_filename_is_ambiguous(self) -> None:
        self.write_file(
            "src/erbsland/one/Shared.hpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0
#pragma once
""",
        )
        self.write_file(
            "src/erbsland/two/Shared.hpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0
#pragma once
""",
        )
        self.write_file(
            "src/erbsland/sample/Widget.cpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0

#include "two/Shared.hpp"
""",
        )

        self.run_fix_include_paths()

        text = self.read_file("src/erbsland/sample/Widget.cpp")
        self.assertIn('#include "../two/Shared.hpp"', text)

    def test_main_sources_normalize_ide_src_include(self) -> None:
        self.write_file(
            "src/erbsland/other/Thing.hpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0
#pragma once
""",
        )
        self.write_file(
            "src/erbsland/sample/Widget.cpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0

#include "src/erbsland/other/Thing.hpp"
""",
        )

        self.run_fix_include_paths()

        text = self.read_file("src/erbsland/sample/Widget.cpp")
        self.assertIn('#include "../other/Thing.hpp"', text)

    def test_src_include_without_erbsland_is_reported(self) -> None:
        self.write_file(
            "src/erbsland/sample/Widget.cpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0

#include "generated/src/Thing.hpp"
""",
        )

        with self.assertRaisesRegex(UtilityError, "must not contain \"src\""):
            self.run_fix_include_paths()

    def test_error_does_not_write_partial_changes(self) -> None:
        self.write_file(
            "src/erbsland/other/Thing.hpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0
#pragma once
""",
        )
        self.write_file(
            "src/erbsland/one/Shared.hpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0
#pragma once
""",
        )
        self.write_file(
            "src/erbsland/two/Shared.hpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0
#pragma once
""",
        )
        original_text = """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0

#include "erbsland/other/Thing.hpp"
"""
        self.write_file("src/erbsland/sample/First.cpp", original_text)
        self.write_file(
            "src/erbsland/sample/Second.cpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0

#include "Shared.hpp"
""",
        )

        with self.assertRaisesRegex(UtilityError, "Ambiguous include path"):
            self.run_fix_include_paths()

        self.assertEqual(original_text, self.read_file("src/erbsland/sample/First.cpp"))

    def test_unit_tests_use_erbsland_global_include(self) -> None:
        self.write_file(
            "src/erbsland/sample/Widget.hpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0
#pragma once
""",
        )
        self.write_file(
            "src/erbsland/sample/Detail.hpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0
#pragma once
""",
        )
        self.write_file(
            "test/unittest/src/sample/LocalHelper.hpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0
#pragma once
""",
        )
        self.write_file(
            "test/unittest/src/sample/WidgetTest.cpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0

#include "Widget.hpp"
#include "../../../../src/erbsland/sample/Detail.hpp"
#include "LocalHelper.hpp"
#include "erbsland/unittest/UnitTest.hpp"

#include <vector>
""",
        )

        self.run_fix_include_paths()

        text = self.read_file("test/unittest/src/sample/WidgetTest.cpp")
        self.assertIn("#include <erbsland/sample/Widget.hpp>", text)
        self.assertIn("#include <erbsland/sample/Detail.hpp>", text)
        self.assertIn('#include "LocalHelper.hpp"', text)
        self.assertIn("#include <erbsland/unittest/UnitTest.hpp>", text)
        self.assertIn("#include <vector>", text)

    def test_demos_use_erbsland_global_include_and_keep_local_helpers(self) -> None:
        self.write_file(
            "src/erbsland/text/String.hpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0
#pragma once
""",
        )
        self.write_file(
            "demos/_common/src/DemoCommon.hpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0
#pragma once
""",
        )
        self.write_file(
            "demos/text/Sample/SampleDemos.hpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0
#pragma once
""",
        )
        self.write_file(
            "demos/text/Sample/main.cpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0

#include "SampleDemos.hpp"
#include "../../../../src/erbsland/text/String.hpp"
#include <DemoCommon.hpp>
""",
        )

        self.run_fix_include_paths()

        text = self.read_file("demos/text/Sample/main.cpp")
        self.assertIn('#include "SampleDemos.hpp"', text)
        self.assertIn("#include <erbsland/text/String.hpp>", text)
        self.assertIn("#include <DemoCommon.hpp>", text)

    def test_unit_tests_report_unknown_quoted_project_include(self) -> None:
        self.write_file(
            "test/unittest/src/sample/WidgetTest.cpp",
            """// Copyright (c) 2026 Tobias Erbsland
// SPDX-License-Identifier: Apache-2.0

#include "Missing.hpp"
""",
        )

        with self.assertRaisesRegex(UtilityError, 'No library source file named "Missing.hpp"'):
            self.run_fix_include_paths()


if __name__ == "__main__":
    unittest.main()
