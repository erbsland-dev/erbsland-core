# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from run import REGISTERED_UTILITIES


class CTermGeneratorTest(unittest.TestCase):
    """Tests for cterm generated-data utility integration."""

    def test_utilities_are_registered(self) -> None:
        self.assertIn("generate_common_box_frame_style", REGISTERED_UTILITIES)
        self.assertIn("generate_frame_border_joint_chars", REGISTERED_UTILITIES)


if __name__ == "__main__":
    unittest.main()
