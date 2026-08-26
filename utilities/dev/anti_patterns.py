# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Scan first-party C++ sources for documented anti-patterns."""

from __future__ import annotations

import argparse
from pathlib import Path

from lib.anti_patterns import AntiPatternConfig, AntiPatternScanner, create_report
from lib.utility import UtilityApp


def report_limit(value: str) -> int:
    """Validate the bounded report limit."""
    try:
        result = int(value)
    except ValueError:
        raise argparse.ArgumentTypeError("the report limit must be an integer") from None
    if not 1 <= result <= 200:
        raise argparse.ArgumentTypeError("the report limit must be between 1 and 200")
    return result


class AntiPatternsApp(UtilityApp):
    """Scan first-party C++ sources for documented anti-patterns."""

    description = "Scan first-party C++ sources for documented anti-patterns."

    def __init__(self) -> None:
        super().__init__()
        self.requested_paths: list[Path] = []
        self.limit = 20
        self.show_suppressed = False
        self.active_findings = 0

    def add_command_line_args(self, parser: argparse.ArgumentParser) -> None:
        parser.add_argument("paths", nargs="*", type=Path, help="Optional project-relative files or directories.")
        parser.add_argument("--limit", type=report_limit, default=20, help="Number of findings to show (1-200).")
        parser.add_argument(
            "--show-suppressed", action="store_true", help="Include inline and centrally suppressed findings."
        )

    def handle_command_line_args(self, args: argparse.Namespace) -> None:
        self.requested_paths = args.paths
        self.limit = args.limit
        self.show_suppressed = args.show_suppressed

    def run(self, argv=None) -> None:
        super().run(argv)
        config = AntiPatternConfig.read(self.project_directory, self.config_file_path())
        scanner = AntiPatternScanner(config)
        findings = scanner.scan(self.requested_paths or None)
        self.print_verbose(
            f"Anti-pattern cache: {scanner.cache_hits} unchanged file(s) reused, "
            f"{scanner.cache_misses} file(s) scanned."
        )
        self.active_findings = sum(not finding.suppressed for finding in findings)
        print(create_report(findings, limit=self.limit, show_suppressed=self.show_suppressed), end="")

    def main(self, argv=None) -> int:
        result = super().main(argv)
        if result == 130:
            return result
        if result != 0:
            return 2
        return 1 if self.active_findings else 0


def main() -> None:
    raise SystemExit(AntiPatternsApp().main())


if __name__ == "__main__":
    main()
