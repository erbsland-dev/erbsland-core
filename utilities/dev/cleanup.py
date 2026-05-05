# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from pathlib import Path

from lib.cleanup import CMakeCleanup, CleanupConfig, IncludeCleanup
from lib.utility import UtilityApp


class CleanupApp(UtilityApp):
    """Clean up generated and convention-managed development files."""

    description = "Cleanup/sort #include statements and simple CMakeLists.txt files."

    def __init__(self):
        """Create a new cleanup app."""
        super().__init__()
        self.project_dir = Path()
        self.config: CleanupConfig | None = None

    def handle_command_line_args(self, args) -> None:
        """
        Handle the command line arguments.
        """
        self.project_dir = self.project_directory

    def read_config(self) -> CleanupConfig:
        """
        Read the cleanup configuration.

        :return: The cleanup configuration.
        """
        self.print_verbose("Reading the configuration")
        return CleanupConfig.read(self.project_dir, self.config_file_path())

    def run(self, argv=None):
        """
        Run this script.
        """
        super().run(argv)
        self.config = self.read_config()
        IncludeCleanup(self.config, self.print_verbose).run()
        CMakeCleanup(self.config, self.print_verbose).run()


def main():
    raise SystemExit(CleanupApp().main())


if __name__ == "__main__":
    main()
