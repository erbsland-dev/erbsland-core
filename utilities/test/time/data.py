# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from pathlib import Path

from lib.copyright import HeaderConfig
from lib.file_update import FileUpdate
from test.time.model import DateTimeValue, DateValue, TimeValue


INT64_MIN = -(1 << 63)
INT64_MAX = (1 << 63) - 1


def date_fields(value: DateValue) -> str:
    """Create date fields for line-based fixture data."""
    return f"{value.year} {value.month} {value.day}"


def time_fields(value: TimeValue) -> str:
    """Create time fields for line-based fixture data."""
    return f"{value.hour} {value.minute} {value.second} {value.nanosecond}"


def datetime_fields(value: DateTimeValue) -> str:
    """Create date-time fields for line-based fixture data."""
    return f"{date_fields(value.date)} {time_fields(value.time)}"


def data_file_text(header_config: HeaderConfig, schema: str, rows: list[str]) -> str:
    """Create the full contents for a generated time fixture file."""
    return "\n".join(
        [
            header_config.source_header("py", tool="generate_time_tests.py"),
            "",
            f"# {schema}",
            *rows,
            "",
        ]
    )


def write_data_file(
    file_update: FileUpdate,
    path: Path,
    header_config: HeaderConfig,
    schema: str,
    rows: list[str],
) -> bool:
    """Write generated fixture data if the contents changed."""
    return file_update.write_if_changed(path, data_file_text(header_config, schema, rows))
