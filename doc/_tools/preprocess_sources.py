# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import tempfile
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

TOOL_VERSION = "1"
MARKER_FILE_NAME = ".preprocess-sources-run"
PROJECT_DIR = Path(__file__).resolve().parents[2]
DOC_DIR = PROJECT_DIR / "doc"
CONF_PATH = DOC_DIR / "conf.py"
SOURCE_DIR = PROJECT_DIR / "src"
OUTPUT_DIR = PROJECT_DIR / "_doxygen_input"


def sha256_file(path: Path) -> str:
    """Create a SHA-256 hash for one file."""
    digest = hashlib.sha256()
    with path.open("rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def swap_tr(match: re.Match[str]) -> str:
    """Swap trailing return type syntax for Doxygen input."""
    return match.group("return_type") + " " + match.group("declarator") + " " + match.group("suffix")


class SourcePreprocessor:
    """Prepare a Doxygen-friendly copy of the public source headers."""

    RE_SWAP_TRAILING_RETURN = re.compile(
        r"auto\s+(?P<declarator>.*?)\s+->\s*"
        r"(?P<return_type>.*?)\s*"
        r"(?P<suffix>(?:(?:override|final)\s+)*(?:override|final)?\s*[;{]|=\s*(?:0|default|delete)\s*;)"
    )
    RE_STD_HELPERS = re.compile(r"(?s)template\s*<>\s*\nstruct.*\{\n.*\n\};\n")
    RE_MACRO_DEFINITIONS = re.compile(r"""(?mx)
        ^\s* \# \s* define \s+ ERBSLAND_CORE_[A-Z_]+ [^\n]* \\\n (?: [^\n]* \\\n )* [^\n]* \n
        | ^\s* \# \s* define \s+ ERBSLAND_CORE_[A-Z_]+ [^\n]* \n
        """)
    RE_MACRO_USES = re.compile(r"""(?mx)
        ^(?P<prefix> \s* (?: /// \s*)? ) \s* ERBSLAND_CORE_[A-Z_]+ [^\n]* \n
        """)
    RE_REQUIRES = re.compile(r"(?s)(template.*?)\s+requires\s*\(.*?\)\s*\n")
    RE_ONE_LINE_SPECIALIZATION = re.compile(r"(?s)template<>\s+\[\[nodiscard\]\]\s+inline.*?\}\s*\n")

    def __init__(self, *, force: bool = False, verbose: bool = False) -> None:
        self.force = force
        self.verbose = verbose
        self.project_dir = PROJECT_DIR
        self.conf_path = CONF_PATH
        self.source_dir = SOURCE_DIR
        self.output_dir = OUTPUT_DIR
        self.marker_path = OUTPUT_DIR / MARKER_FILE_NAME

    def log(self, message: str) -> None:
        """Print a verbose progress message."""
        if self.verbose:
            print(message, flush=True)

    @staticmethod
    def is_aggregate_header(path: Path) -> bool:
        """Test if a header is a generated aggregate include."""
        return path.name == "all.hpp" or (path.name.startswith("all_") and path.suffix == ".hpp")

    def source_files(self) -> list[Path]:
        """Collect all source headers used as Doxygen input."""
        result = []
        for path in self.source_dir.rglob("*.hpp"):
            if path.is_symlink() or not path.is_file():
                continue
            if self.is_aggregate_header(path):
                continue
            result.append(path)
        return sorted(result, key=lambda path: path.relative_to(self.source_dir).as_posix().casefold())

    def source_inventory(self) -> list[dict[str, Any]]:
        """Create a cheap inventory for detecting changed source inputs."""
        inventory = []
        for path in self.source_files():
            stat = path.stat()
            inventory.append(
                {
                    "path": path.relative_to(self.source_dir).as_posix(),
                    "size": stat.st_size,
                    "mtime_ns": stat.st_mtime_ns,
                }
            )
        return inventory

    def current_marker_data(self) -> dict[str, Any]:
        """Create the marker data for the current project state."""
        return {
            "tool": "doc/_tools/preprocess_sources.py",
            "tool_version": TOOL_VERSION,
            "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
            "conf_sha256": sha256_file(self.conf_path),
            "sources": self.source_inventory(),
        }

    def read_marker(self) -> dict[str, Any] | None:
        """Read the existing marker file, or return `None` if it cannot be used."""
        if not self.marker_path.exists() or self.marker_path.is_symlink() or not self.marker_path.is_file():
            return None
        try:
            return json.loads(self.marker_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            return None

    def is_up_to_date(self, current_marker_data: dict[str, Any]) -> bool:
        """Test if the generated source tree is up to date."""
        if self.force or not self.output_dir.is_dir() or self.output_dir.is_symlink():
            return False
        existing = self.read_marker()
        if existing is None:
            return False
        for key in ("tool_version", "conf_sha256", "sources"):
            if existing.get(key) != current_marker_data.get(key):
                return False
        marker_mtime = self.marker_path.stat().st_mtime
        return all(path.stat().st_mtime <= marker_mtime for path in self.source_files())

    def transform_source(self, content: str) -> str:
        """Apply Doxygen compatibility transformations to one header."""
        content = self.RE_SWAP_TRAILING_RETURN.sub(swap_tr, content)
        content = self.RE_STD_HELPERS.sub("", content)
        content = self.RE_MACRO_DEFINITIONS.sub("// macro removed\n", content)
        content = self.RE_MACRO_USES.sub(r"\g<prefix>// macro removed\n", content)
        content = self.RE_REQUIRES.sub("\\1/* requires removed */\n", content)
        return self.RE_ONE_LINE_SPECIALIZATION.sub("", content)

    def write_processed_tree(self, temporary_dir: Path, marker_data: dict[str, Any]) -> int:
        """Write processed headers into a temporary directory."""
        count = 0
        for source_path in self.source_files():
            relative_path = source_path.relative_to(self.source_dir)
            destination_path = temporary_dir / relative_path
            content = source_path.read_text(encoding="utf-8")
            destination_path.parent.mkdir(parents=True, exist_ok=True)
            destination_path.write_text(self.transform_source(content), encoding="utf-8")
            count += 1
        (temporary_dir / MARKER_FILE_NAME).write_text(json.dumps(marker_data, indent=2, sort_keys=True) + "\n")
        return count

    def replace_output_tree(self, temporary_dir: Path) -> None:
        """Replace the generated output directory with a prepared tree."""
        if self.output_dir.is_symlink():
            raise RuntimeError(f"Refusing to replace symbolic link: {self.output_dir}")
        if self.output_dir.exists():
            if not self.output_dir.is_dir():
                raise RuntimeError(f"Refusing to replace non-directory path: {self.output_dir}")
            shutil.rmtree(self.output_dir)
        temporary_dir.rename(self.output_dir)

    def run(self) -> bool:
        """Run preprocessing. Returns `True` when output was regenerated."""
        marker_data = self.current_marker_data()
        if self.is_up_to_date(marker_data):
            print("Doxygen input is up to date.")
            return False
        with tempfile.TemporaryDirectory(prefix="_doxygen_input.", dir=self.project_dir) as temporary_name:
            temporary_dir = Path(temporary_name)
            count = self.write_processed_tree(temporary_dir, marker_data)
            self.replace_output_tree(temporary_dir)
        print(f"Preprocessed {count} source files for Doxygen.")
        return True


def main(argv: list[str] | None = None) -> int:
    """Command line entry point."""
    parser = argparse.ArgumentParser(description="Prepare Doxygen input sources for the documentation build.")
    parser.add_argument("--force", action="store_true", help="Rebuild the Doxygen input directory.")
    parser.add_argument("-v", "--verbose", action="store_true", help="Print progress details.")
    args = parser.parse_args(argv)
    SourcePreprocessor(force=args.force, verbose=args.verbose).run()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
