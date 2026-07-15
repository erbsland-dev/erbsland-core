# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Validate API test-status documentation markers."""

from __future__ import annotations

import argparse
from pathlib import Path

from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.test_status import normalize_test_status_text, validate_test_status_text
from lib.utility import UtilityApp


class TestStatusApp(UtilityApp):
    """Validate or normalize test-status markers in API documentation blocks."""

    description = "Validate API test-status documentation markers."

    def __init__(self) -> None:
        super().__init__()
        self.normalize = False

    def add_command_line_args(self, parser: argparse.ArgumentParser) -> None:
        parser.add_argument("--normalize", action="store_true", help="Normalize valid marker placement and syntax.")

    def handle_command_line_args(self, args) -> None:
        self.normalize = args.normalize

    def run(self, argv=None) -> None:
        super().run(argv)
        source_dir = self.project_directory / "src" / "erbsland"
        file_update = FileUpdate(self.print_verbose if self.verbose else None)
        issues = []
        for path in sorted(source_dir.rglob("*.hpp"), key=lambda item: item.as_posix().casefold()):
            text = path.read_text(encoding="utf-8")
            if self.normalize:
                text = normalize_test_status_text(text)
                file_update.write_if_changed(path, text)
            issues.extend(f"{path.relative_to(self.project_directory)}:{issue.line_number}: {issue.message}" for issue in validate_test_status_text(text))
        if issues:
            raise UtilityError("Invalid test-status markers:\n" + "\n".join(issues))


def main() -> None:
    raise SystemExit(TestStatusApp().main())


if __name__ == "__main__":
    main()
