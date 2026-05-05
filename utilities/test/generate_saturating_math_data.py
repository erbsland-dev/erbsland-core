#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from lib.copyright import HeaderConfig
from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.path_safety import require_directory
from lib.utility import UtilityApp


@dataclass(frozen=True)
class IntegerType:
    name: str
    bits: int
    signed: bool

    @property
    def minimum(self) -> int:
        if self.signed:
            return -(1 << (self.bits - 1))
        return 0

    @property
    def maximum(self) -> int:
        if self.signed:
            return (1 << (self.bits - 1)) - 1
        return (1 << self.bits) - 1

    @property
    def values(self) -> list[int]:
        candidates = {
            self.minimum,
            self.maximum,
            self.minimum + 1,
            self.maximum - 1,
        }
        positive_boundaries = [
            0,
            1,
            2,
            9,
            23,
            64,
            126,
            127,
            128,
            129,
            254,
            255,
            256,
            32766,
            32767,
            32768,
            32769,
            65534,
            65535,
            65536,
            2147483646,
            2147483647,
            2147483648,
            2147483649,
            4294967294,
            4294967295,
            4294967296,
            9223372036854775806,
            9223372036854775807,
            9223372036854775808,
            18446744073709551614,
            18446744073709551615,
        ]
        for value in positive_boundaries:
            candidates.add(value)
            if self.signed:
                candidates.add(-value)
        return sorted(value for value in candidates if self.minimum <= value <= self.maximum)


TYPES = [
    IntegerType("i8", 8, True),
    IntegerType("u8", 8, False),
    IntegerType("i16", 16, True),
    IntegerType("u16", 16, False),
    IntegerType("i32", 32, True),
    IntegerType("u32", 32, False),
    IntegerType("i64", 64, True),
    IntegerType("u64", 64, False),
]


def clamp(value: int, target: IntegerType) -> int:
    return min(max(value, target.minimum), target.maximum)


def overflow(value: int, target: IntegerType) -> int:
    return int(value != clamp(value, target))


def cxx_divide(first: int, second: int) -> int:
    quotient = abs(first) // abs(second)
    if (first < 0) != (second < 0):
        quotient = -quotient
    return quotient


def cxx_modulo(first: int, second: int) -> int:
    return first - cxx_divide(first, second) * second


def operation_result(operation: str, first: int, second: int) -> int:
    if operation == "add":
        return first + second
    if operation == "subtract":
        return first - second
    if operation == "multiply":
        return first * second
    if operation == "divide":
        return cxx_divide(first, second)
    if operation == "modulo":
        return cxx_modulo(first, second)
    raise UtilityError(f"Unknown operation: {operation}")


class GenerateSaturatingMathDataApp(UtilityApp):
    """Generate saturating math unit-test data."""

    description = "Generate saturating math unit-test data files."

    def __init__(self) -> None:
        super().__init__()
        self.output_dir = Path()
        self.header_config: HeaderConfig | None = None
        self.file_update = FileUpdate(self.print_verbose)

    def handle_command_line_args(self, args) -> None:
        self.output_dir = self.project_directory / "test" / "unittest" / "data" / "saturating_math"

    def write_file(self, name: str, header: str, lines: list[str]) -> None:
        if self.header_config is None:
            raise UtilityError("Header configuration was not loaded.")
        text = "\n".join(
            [
                self.header_config.source_header("py", tool="generate_saturating_math_data.py"),
                "",
                f"# {header}",
                *lines,
                "",
            ]
        )
        if self.file_update.write_if_changed(self.output_dir / name, text):
            self.print_verbose(f"Generated {name} with {len(lines)} rows.")
        else:
            self.print_verbose(f"Kept unchanged {name} with {len(lines)} rows.")

    def generate_cast(self) -> None:
        lines = []
        for target in TYPES:
            for source in TYPES:
                for value in source.values:
                    expected = clamp(value, target)
                    lines.append(f"{target.name} {source.name} {value} {expected} {int(value != expected)}")
        self.write_file("cast.txt", "target_type source_type value expected overflow", lines)

    def generate_operation(self, operation: str) -> None:
        lines = []
        for first_type in TYPES:
            for second_type in TYPES:
                for first in first_type.values:
                    for second in second_type.values:
                        if operation in {"divide", "modulo"} and second == 0:
                            continue
                        result = operation_result(operation, first, second)
                        expected = clamp(result, first_type)
                        overflow_flag = 0 if operation == "modulo" else overflow(result, first_type)
                        lines.append(
                            f"{first_type.name} {second_type.name} {first} {second} {expected} {overflow_flag}"
                        )
        self.write_file(f"{operation}.txt", "first_type second_type first second expected overflow", lines)

    def run(self, argv=None) -> None:
        super().run(argv)
        self.header_config = HeaderConfig.read(self.config_file_path())
        self.output_dir.mkdir(parents=True, exist_ok=True)
        require_directory(self.output_dir, "Saturating math data output directory")
        self.generate_cast()
        for operation in ["add", "subtract", "multiply", "divide", "modulo"]:
            self.generate_operation(operation)


def main() -> None:
    raise SystemExit(GenerateSaturatingMathDataApp().main())


if __name__ == "__main__":
    main()
