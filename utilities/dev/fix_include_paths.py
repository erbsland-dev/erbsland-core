# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from pathlib import Path

from lib.fix_include_paths import FixIncludePaths, FixIncludePathsConfig
from lib.utility import UtilityApp


class FixIncludePathsApp(UtilityApp):
    """Fix wrong C++ include paths in library and unit test sources."""

    description = "Fix include paths in C/C++ source files."

    def __init__(self) -> None:
        """Create a new fix-include-paths app."""
        super().__init__()
        self.project_dir = Path()

    def handle_command_line_args(self, args) -> None:
        """Handle the command line arguments."""
        self.project_dir = self.project_directory

    def read_config(self) -> FixIncludePathsConfig:
        """Read the configuration."""
        self.print_verbose("Reading the configuration")
        return FixIncludePathsConfig.read(self.project_dir, self.config_file_path())

    def run(self, argv=None) -> None:
        """Run the include path fixer."""
        super().run(argv)
        FixIncludePaths(self.read_config(), self.print_verbose).run()


def main() -> None:
    raise SystemExit(FixIncludePathsApp().main())


if __name__ == "__main__":
    main()
