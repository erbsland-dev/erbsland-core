# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from lib.test_status import normalize_test_status_text, validate_test_status_text


class TestStatusTest(unittest.TestCase):
    """Tests for API test-status marker validation."""

    def test_normalizes_suite_list_and_removes_member_marker(self) -> None:
        source = """/// A documented type.
/// @tested{WidgetTest, WidgetTest::testOperation}
class Widget {
public:
    /// Perform an operation.
    /// @tested{WidgetTest}
    void operate();
};
"""

        normalized = normalize_test_status_text(source)

        self.assertEqual(
            """/// A documented type.
/// @tested{WidgetTest}
class Widget {
public:
    /// Perform an operation.
    void operate();
};
""",
            normalized,
        )
        self.assertEqual([], validate_test_status_text(normalized))

    def test_accepts_free_function_marker(self) -> None:
        source = """/// Build a widget.
/// @tested{WidgetTest}
auto buildWidget() -> Widget;

/// Build another widget.
/// @tested{WidgetTest}
[[nodiscard]] inline auto buildAnotherWidget() -> Widget;
"""

        self.assertEqual([], validate_test_status_text(source))

    def test_accepts_type_alias_marker(self) -> None:
        source = """/// A documented widget alias.
/// @tested{WidgetTest}
using WidgetAlias = Widget;
"""

        self.assertEqual([], validate_test_status_text(source))

    def test_rejects_method_selectors_multiline_values_and_member_markers(self) -> None:
        source = """class Widget {
public:
    /// Perform an operation.
    /// @tested{WidgetTest::testOperation,
    ///     OtherTest}
    void operate();
};
"""

        messages = [issue.message for issue in validate_test_status_text(source)]

        self.assertIn("test-status markers must be single-line.", messages)
        self.assertIn("test-status markers are only allowed on types and free functions.", messages)
        self.assertIn("@tested must contain space-separated test suite names only.", messages)


if __name__ == "__main__":
    unittest.main()
