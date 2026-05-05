# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from pathlib import Path

from lib.reference_doc import (
    ApiEntry,
    ApiEntryKind,
    HeaderApi,
    HeaderScanner,
    ReferenceDocConfig,
    ReferenceDocGenerator,
    ReferenceGroup,
    camel_to_snake,
    clean_function_signature,
    has_function_declaration_terminator,
    is_function_signature_complete,
    split_identifier_words,
    strip_line_comment_and_literals,
)
from lib.utility import UtilityApp

__all__ = [
    "ApiEntry",
    "ApiEntryKind",
    "HeaderApi",
    "HeaderScanner",
    "ReferenceDocApp",
    "ReferenceDocConfig",
    "ReferenceDocGenerator",
    "ReferenceGroup",
    "camel_to_snake",
    "clean_function_signature",
    "has_function_declaration_terminator",
    "is_function_signature_complete",
    "split_identifier_words",
    "strip_line_comment_and_literals",
]


class ReferenceDocApp(UtilityApp):
    """Update reference documentation from the public API headers."""

    description = "Update reference documentation pages from public API headers."

    def __init__(self) -> None:
        super().__init__()
        self.project_dir = Path()
        self.config: ReferenceDocConfig | None = None

    def handle_command_line_args(self, args) -> None:
        self.project_dir = self.project_directory

    def read_config(self) -> ReferenceDocConfig:
        """Read the reference documentation configuration."""
        self.print_verbose("Reading the configuration")
        return ReferenceDocConfig.read(self.project_dir, self.config_file_path())

    def run(self, argv=None) -> None:
        """Run this script."""
        super().run(argv)
        self.config = self.read_config()
        self.config.reference_dir.mkdir(parents=True, exist_ok=True)
        generator = ReferenceDocGenerator(self.config, self.print_verbose if self.verbose else None)
        headers = generator.collect_headers()
        generator.warn_uncategorized_headers(headers)
        generator.update_reference_pages(headers)
        generator.update_orphan_pages(headers)
        generator.update_indexes(headers)


def main():
    """Main entry point."""
    raise SystemExit(ReferenceDocApp().main())


if __name__ == "__main__":
    main()
