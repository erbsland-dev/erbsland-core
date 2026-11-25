#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Prepare a standalone working copy of the sample extension."""

import re
import subprocess
import sys
import tempfile
from pathlib import Path

CORE_URL = "https://github.com/erbsland-dev/erbsland-core.git"


def select_core(root: Path) -> Path:
    local = root / "test/erbsland-core"
    for candidate in (root.parent / "erbsland-core", local):
        if (candidate / "CMakeLists.txt").is_file():
            return candidate
    if local.exists():
        raise RuntimeError(f"Expected an Erbsland Core checkout at {local}.")

    refs = subprocess.check_output(["git", "ls-remote", "--tags", "--refs", CORE_URL], text=True)
    releases = re.findall(r"^[0-9a-f]+\s+refs/tags/v(\d+)\.(\d+)\.(\d+)$", refs, re.MULTILINE)
    if not releases:
        raise RuntimeError("No Core release tags found.")
    version = max(releases, key=lambda fields: tuple(map(int, fields)))
    tag = "v" + ".".join(version)
    local.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=local.parent) as temporary:
        checkout = Path(temporary) / "core"
        subprocess.run(["git", "clone", "--branch", tag, "--depth", "1", CORE_URL, str(checkout)], check=True)
        checkout.rename(local)
    return local


def setup(root: Path) -> None:
    core = select_core(root)
    sys.path.insert(0, str(core / "tool/python"))
    from erbsland_setup import prepare_development

    prepare_development(
        root,
        "erbsland-sample-tool",
        [str(core / "doc/requirements-doc.txt")],
        ["test/erbsland-unittest"],
        ["-DERBSLAND_SAMPLE_ENABLE_TOOLS=ON", "-DERBSLAND_SAMPLE_ENABLE_TESTS=OFF"],
    )
    (root / ".venv/erbsland-sphinx.path").write_text(str(core / "doc/_ext") + "\n", encoding="utf-8")


if __name__ == "__main__":
    try:
        setup(Path(__file__).resolve().parent)
    except (OSError, RuntimeError, ImportError, subprocess.CalledProcessError) as error:
        sys.exit(f"Setup failed: {error}")
