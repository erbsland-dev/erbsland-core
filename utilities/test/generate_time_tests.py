#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import math
import random
from pathlib import Path

from lib.copyright import HeaderConfig
from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.path_safety import require_directory
from lib.utility import UtilityApp
from test.time.data import INT64_MAX, INT64_MIN, date_fields, datetime_fields, time_fields, write_data_file
from test.time.model import (
    LAST_VALID_DAY,
    DateTimeValue,
    DateValue,
    TimeValue,
    date_from_days,
    days_from_year,
)


def cxx_divide(first: int, second: int) -> int:
    """Divide like C++ signed integers."""
    quotient = abs(first) // abs(second)
    return -quotient if (first < 0) != (second < 0) else quotient


def clamp_int64(value: int) -> int:
    """Clamp a Python integer to the C++ int64_t range."""
    return min(max(value, INT64_MIN), INT64_MAX)


def converted_value(value: int, source_num: int, source_den: int, target_num: int, target_den: int) -> int:
    """Emulate TimeAmount::converted for seconds-based units."""
    factor = source_num * target_den
    divisor = source_den * target_num
    common = math.gcd(factor, divisor)
    factor //= common
    divisor //= common
    if divisor == 1:
        return clamp_int64(value * factor)
    return clamp_int64(cxx_divide(value * factor, divisor))


def random_date(rng: random.Random) -> DateValue:
    """Create a random supported date."""
    while True:
        year = rng.randint(0, 9999)
        month = rng.randint(1, 12)
        day = rng.randint(1, 31)
        try:
            return DateValue(year, month, day)
        except ValueError:
            continue


def random_time(rng: random.Random) -> TimeValue:
    """Create a random supported time."""
    return TimeValue(
        rng.randint(0, 23),
        rng.randint(0, 59),
        rng.randint(0, 59),
        rng.randint(0, 999_999_999),
    )


class GenerateTimeTestsApp(UtilityApp):
    """Generate deterministic date/time unit-test data files."""

    description = "Generate deterministic date/time unit-test data files."

    def __init__(self) -> None:
        super().__init__()
        self.output_dir = Path()
        self.header_config: HeaderConfig | None = None
        self.file_update = FileUpdate(self.print_verbose)

    def handle_command_line_args(self, args) -> None:
        self.output_dir = self.project_directory / "test" / "unittest" / "data" / "time"

    def validate_model(self) -> None:
        """Validate the independent model over the full supported date range."""
        for year in range(0, 10_000):
            expected = 365 * year + (year + 3) // 4 - (year + 99) // 100 + (year + 399) // 400
            if days_from_year(year) != expected:
                raise UtilityError(f"Date model year conversion failed for {year}.")
        for days in range(0, LAST_VALID_DAY + 1):
            date = date_from_days(days)
            if date.days != days:
                raise UtilityError(f"Date model roundtrip failed for day {days}: {date}.")
        if DateValue(1970, 1, 1).days != 719_528:
            raise UtilityError("Date model POSIX epoch validation failed.")

    def write_file(self, name: str, schema: str, rows: list[str]) -> None:
        """Write one generated fixture file."""
        if self.header_config is None:
            raise UtilityError("Header configuration was not loaded.")
        if write_data_file(self.file_update, self.output_dir / name, self.header_config, schema, rows):
            self.print_verbose(f"Generated {name} with {len(rows)} rows.")
        else:
            self.print_verbose(f"Kept unchanged {name} with {len(rows)} rows.")

    def generate_date_values(self) -> None:
        rng = random.Random(29839)
        dates = {DateValue(0, 1, 1), DateValue(0, 12, 31), DateValue(1970, 1, 1), DateValue(9999, 12, 31)}
        while len(dates) < 124:
            dates.add(random_date(rng))
        for _ in range(40):
            year = rng.randint(1, 9999)
            dates.add(DateValue(year, 1, 1))
            dates.add(DateValue(year - 1, 12, 31))
        sorted_dates = sorted(dates, key=lambda value: value.days)
        self.write_file(
            "date_epoch.txt",
            "days year month day day_of_year day_of_week",
            [
                f"{value.days} {value.year} {value.month} {value.day} "
                f"{value.day_of_year} {value.day_of_week_index}"
                for value in sorted_dates
            ],
        )

        rng = random.Random(29809)
        manipulations: list[str] = []
        for _ in range(48):
            base = random_date(rng)
            delta = rng.randint(-base.days, LAST_VALID_DAY - base.days)
            manipulations.append(f"{date_fields(base)} days {delta} {date_fields(base.add_days(delta))}")
        for _ in range(48):
            base = random_date(rng)
            month_index = base.year * 12 + base.month - 1
            delta = rng.randint(-month_index, 9999 * 12 + 11 - month_index)
            manipulations.append(f"{date_fields(base)} months {delta} {date_fields(base.add_months(delta))}")
        for _ in range(48):
            base = random_date(rng)
            delta = rng.randint(-base.year, 9999 - base.year)
            manipulations.append(f"{date_fields(base)} years {delta} {date_fields(base.add_years(delta))}")
        self.write_file(
            "date_manipulation.txt",
            "base_year base_month base_day unit amount expected_year expected_month expected_day",
            manipulations,
        )

        rng = random.Random(21809)
        days_to: list[str] = []
        for _ in range(140):
            source = random_date(rng)
            delta = rng.randint(-source.days, LAST_VALID_DAY - source.days)
            target = source.add_days(delta)
            days_to.append(f"{date_fields(source)} {date_fields(target)} {delta}")
        self.write_file(
            "date_days_to.txt",
            "source_year source_month source_day target_year target_month target_day days",
            days_to,
        )

        comparisons = [
            "0 0 0 0 -1",
            f"1 {date_fields(DateValue(0, 1, 1))} 0",
            f"1 {date_fields(DateValue(2022, 10, 2))} 1",
            f"1 {date_fields(DateValue(9999, 12, 31))} 2",
        ]
        self.write_file("date_comparison.txt", "valid year month day order", comparisons)

    def generate_time_values(self) -> None:
        rng = random.Random(29839)
        times = {TimeValue(0, 0, 0), TimeValue(23, 59, 59, 999_999_999)}
        for _ in range(10):
            times.add(random_time(rng))
        for _ in range(10):
            hour = rng.randint(0, 23)
            minute = rng.randint(0, 59)
            second = rng.randint(0, 59)
            fraction = rng.randint(0, 999_999_999)
            candidates = [
                TimeValue(hour, minute, second, 0),
                TimeValue(hour, minute, 0, 0),
                TimeValue(hour, minute, 0, fraction),
                TimeValue(hour, 0, 0, 0),
            ]
            for value in candidates:
                times.add(value)
                if value.nanoseconds_since_midnight > 0:
                    times.add(value.added_nanoseconds(-1))
        self.write_file(
            "time_midnight.txt",
            "hour minute second nanosecond nanoseconds_since_midnight",
            [
                f"{time_fields(value)} {value.nanoseconds_since_midnight}"
                for value in sorted(times, key=lambda value: value.nanoseconds_since_midnight)
            ],
        )
        comparisons = [
            (TimeValue(0, 0, 0, 0), 0),
            (TimeValue(0, 0, 0, 1), 1),
            (TimeValue(0, 0, 1, 0), 2),
            (TimeValue(0, 1, 0, 0), 3),
            (TimeValue(1, 0, 0, 0), 4),
            (TimeValue(18, 22, 59, 108_938_182), 5),
            (TimeValue(23, 59, 59, 999_999_999), 6),
        ]
        self.write_file(
            "time_comparison.txt",
            "hour minute second nanosecond order",
            [f"{time_fields(value)} {order}" for value, order in comparisons],
        )

    def generate_time_units(self) -> None:
        second_units = [
            ("Nanoseconds", 1, 1_000_000_000),
            ("Microseconds", 1, 1_000_000),
            ("Milliseconds", 1, 1_000),
            ("Seconds", 1, 1),
            ("Minutes", 60, 1),
            ("Hours", 3600, 1),
            ("Days", 86_400, 1),
            ("Weeks", 604_800, 1),
        ]
        comparison_rows: list[str] = []
        for name in [unit[0] for unit in second_units] + ["Months", "Years"]:
            for order, value in enumerate([INT64_MIN, -2_903_234, 0, 16_039_287, INT64_MAX]):
                comparison_rows.append(f"{name} {value} {order}")
        for name, minimum, maximum in [
            ("Second", 0, 59),
            ("Minute", 0, 59),
            ("Hour", 0, 23),
            ("Day", 1, 31),
            ("DayOfYear", 1, 366),
            ("Month", 1, 12),
            ("Year", 0, 9999),
        ]:
            for order, value in enumerate([minimum, maximum // 2, maximum]):
                comparison_rows.append(f"{name} {value} {order}")
        for order, value in enumerate([INT64_MIN, -123_456, 0, 987_654, INT64_MAX]):
            comparison_rows.append(f"TimeDelta {value} {order}")
        for order, value in enumerate([INT64_MIN, -123_456, 0, 987_654, INT64_MAX]):
            comparison_rows.append(f"Duration {value} {order}")
        self.write_file("time_amount_comparison.txt", "type value order", comparison_rows)

        values = [
            INT64_MIN,
            -129_219_392_392_193,
            -291_291_198,
            -793_291,
            -378,
            0,
            529,
            10_290,
            828_926_009,
            302_000_928_948_093,
            INT64_MAX,
        ]
        conversion_rows = []
        for source_type, source_num, source_den in second_units:
            for value in values:
                for target_type, target_num, target_den in second_units:
                    expected = converted_value(value, source_num, source_den, target_num, target_den)
                    conversion_rows.append(f"{source_type} {value} {target_type} {expected}")
        self.write_file("time_amount_conversion.txt", "source_type value target_type expected", conversion_rows)

    def generate_date_time_values(self) -> None:
        rng = random.Random(29839)
        values: set[DateTimeValue] = {
            DateTimeValue(DateValue(0, 1, 1), TimeValue(0, 0, 0)),
            DateTimeValue(DateValue(1970, 1, 1), TimeValue(0, 0, 0)),
            DateTimeValue(DateValue(9999, 12, 31), TimeValue(23, 59, 59, 999_999_999)),
        }
        while len(values) < 124:
            values.add(DateTimeValue(random_date(rng), random_time(rng)))
        self.write_file(
            "datetime_epoch.txt",
            "year month day hour minute second nanosecond millisecond seconds_since_epoch",
            [
                f"{datetime_fields(value)} {value.time.nanosecond // 1_000_000} {value.seconds_since_epoch}"
                for value in sorted(values, key=lambda value: (value.seconds_since_epoch, value.time.nanosecond))
            ],
        )

        base_values = [
            DateTimeValue(DateValue(0, 1, 1), TimeValue(0, 0, 0)),
            DateTimeValue(DateValue(1970, 12, 31), TimeValue(4, 17, 0)),
            DateTimeValue(DateValue(2000, 2, 29), TimeValue(17, 59, 13, 100_200_300)),
            DateTimeValue(DateValue(7022, 8, 1), TimeValue(23, 1, 4, 728_349)),
            DateTimeValue(DateValue(9999, 12, 31), TimeValue(23, 59, 59, 999_999_999)),
        ]
        deltas = [0, -1, 1, -60, 60, -3600, 3600, -86400, 86400, INT64_MIN, INT64_MAX]
        for exponent in range(1, 19):
            deltas.extend([10**exponent, -(10**exponent)])
        add_rows = []
        for base in base_values:
            for delta in sorted(set(deltas)):
                add_rows.append(
                    f"{datetime_fields(base)} {delta} "
                    f"{datetime_fields(base.add_seconds(delta))} {datetime_fields(base.add_seconds(-delta))}"
                )
        self.write_file(
            "datetime_add_duration.txt",
            "base_year base_month base_day base_hour base_minute base_second base_nanosecond delta_seconds "
            "add_year add_month add_day add_hour add_minute add_second add_nanosecond "
            "subtract_year subtract_month subtract_day subtract_hour subtract_minute subtract_second "
            "subtract_nanosecond",
            add_rows,
        )

        comparisons = [
            "0 0 0 0 0 0 0 0 -1",
            f"1 {datetime_fields(DateTimeValue(DateValue(0, 1, 1), TimeValue(0, 0, 0)))} 0",
            f"1 {datetime_fields(DateTimeValue(DateValue(2022, 10, 1), TimeValue(17, 40, 12, 128_837_237)))} 1",
            f"1 {datetime_fields(DateTimeValue(DateValue(2022, 10, 2), TimeValue(17, 40, 12, 128_837_236)))} 2",
            f"1 {datetime_fields(DateTimeValue(DateValue(2022, 10, 2), TimeValue(17, 40, 12, 128_837_237)))} 3",
            f"1 {datetime_fields(DateTimeValue(DateValue(9999, 12, 31), TimeValue(23, 59, 59, 999_999_999)))} 4",
        ]
        self.write_file(
            "datetime_comparison.txt",
            "valid year month day hour minute second nanosecond order",
            comparisons,
        )

    def run(self, argv=None) -> None:
        super().run(argv)
        self.header_config = HeaderConfig.read(self.config_file_path())
        self.output_dir.mkdir(parents=True, exist_ok=True)
        require_directory(self.output_dir, "time unittest data directory")
        self.validate_model()
        self.generate_date_values()
        self.generate_time_values()
        self.generate_time_units()
        self.generate_date_time_values()


def main() -> None:
    raise SystemExit(GenerateTimeTestsApp().main())


if __name__ == "__main__":
    main()
