#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from pathlib import Path

from lib.copyright import HeaderConfig
from lib.file_update import FileUpdate
from lib.time_zone.generator import TimeZoneGenerator
from lib.utility import UtilityApp


class GenerateTimeZoneDataApp(UtilityApp):
    """Generate the compact generated IANA time-zone database used by the time module."""

    description = "Generate compact time-zone data from the vendored IANA tz database."

    def __init__(self) -> None:
        super().__init__()
        self.project_dir = Path()
        self.data_dir = Path()
        self.output_dir = Path()
        self.file_update = FileUpdate(self.print_verbose)

    def handle_command_line_args(self, args) -> None:
        self.project_dir = self.project_directory
        self.data_dir = self.project_dir / "utilities" / "data" / "tzdb"
        self.output_dir = self.project_dir / "src" / "erbsland" / "time" / "tz" / "impl"

    def run(self, argv=None) -> None:
        self.parse_command_line(argv)
        header_config = HeaderConfig.read(self.config_file_path())
        changed = TimeZoneGenerator(
            self.project_dir,
            self.data_dir,
            self.output_dir,
            self.file_update,
            header_config,
        ).run()
        if changed:
            for path in changed:
                print(f"Updated {path.relative_to(self.project_dir)}")
        else:
            print("Unchanged generated time-zone data.")
