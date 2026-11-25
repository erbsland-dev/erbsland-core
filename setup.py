#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Bootstrap repository development using only Python's standard library."""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "tool/python"))
from erbsland_setup import SetupError, prepare_development


def setup(root: Path) -> None:
    """Prepare the Core checkout and install its maintenance tool."""
    root = root.resolve()
    marker = root / "CMakeLists.txt"
    if not marker.is_file() or "project(erbsland-core " not in marker.read_text()[:1024]:
        raise SetupError(f"No Erbsland Core project marker in {root}.")
    prepare_development(
        root,
        "erbsland-core-tool",
        ["requirements-dev.txt", "doc/requirements-doc.txt"],
        ["test/erbsland-unittest", "test/erbsland-lang-config-tests"],
        [
            "-DERBSLAND_CORE_ENABLE_TOOLS=ON",
            "-DERBSLAND_CORE_ENABLE_TESTS=OFF",
            "-DERBSLAND_CORE_ENABLE_DEMOS=OFF",
            "-DERBSLAND_CORE_DEVELOPER_BUILD=OFF",
        ],
    )


def main() -> int:
    """Run repository bootstrap from the directory containing this script."""
    try:
        setup(Path(__file__).resolve().parent)
    except (SetupError, OSError) as error:
        print(f"Setup failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
