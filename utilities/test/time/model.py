# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from dataclasses import dataclass


DAYS_PER_400_YEARS = 146_097
DAYS_PER_100_YEARS = 36_524
DAYS_PER_4_YEARS = 1_461
COVERED_YEARS = 10_000
LAST_VALID_DAY = 3_652_424
SECONDS_PER_DAY = 86_400
NANOSECONDS_PER_SECOND = 1_000_000_000
NANOSECONDS_PER_DAY = SECONDS_PER_DAY * NANOSECONDS_PER_SECOND
LAST_VALID_SECOND = LAST_VALID_DAY * SECONDS_PER_DAY + SECONDS_PER_DAY - 1

MONTH_DAYS = (0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31)
REGULAR_DAYS_BEFORE_MONTH = (0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334)
LEAP_DAYS_BEFORE_MONTH = (0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335)


def is_leap_year(year: int) -> bool:
    """Test if a year is a leap year in the proleptic Gregorian calendar."""
    return year % 4 == 0 and (year % 100 != 0 or year % 400 == 0)


def days_in_month(year: int, month: int) -> int:
    """Get the number of days in a month."""
    return MONTH_DAYS[month] + int(month == 2 and is_leap_year(year))


def days_before_month(year: int, month: int) -> int:
    """Get the zero-based day offset for the first day of a month."""
    return (LEAP_DAYS_BEFORE_MONTH if is_leap_year(year) else REGULAR_DAYS_BEFORE_MONTH)[month]


def days_from_year(year: int) -> int:
    """Calculate days before a year with year zero included as a leap year."""
    return 365 * year + (year + 3) // 4 - (year + 99) // 100 + (year + 399) // 400


def date_from_days(days: int) -> "DateValue":
    """Convert a supported day index into a date."""
    if days < 0 or days > LAST_VALID_DAY:
        raise ValueError(f"Day index outside supported range: {days}")
    low = 0
    high = COVERED_YEARS - 1
    while low <= high:
        mid = (low + high) // 2
        start = days_from_year(mid)
        next_start = days_from_year(mid + 1) if mid + 1 < COVERED_YEARS else LAST_VALID_DAY + 1
        if days < start:
            high = mid - 1
        elif days >= next_start:
            low = mid + 1
        else:
            day_of_year = days - start
            month = 1
            while month < 12 and day_of_year >= days_before_month(mid, month + 1):
                month += 1
            day = day_of_year - days_before_month(mid, month) + 1
            return DateValue(mid, month, day)
    raise ValueError(f"Could not resolve day index: {days}")


def clamp_day(days: int) -> int:
    """Clamp a day index to the supported date range."""
    return min(max(days, 0), LAST_VALID_DAY)


def clamp_second(seconds: int) -> int:
    """Clamp a second index to the supported date-time range."""
    return min(max(seconds, 0), LAST_VALID_SECOND)


@dataclass(frozen=True, order=True)
class DateValue:
    """Independent date value for generated time tests."""

    year: int
    month: int
    day: int

    def __post_init__(self) -> None:
        if not (0 <= self.year <= 9999):
            raise ValueError(f"Invalid year: {self.year}")
        if not (1 <= self.month <= 12):
            raise ValueError(f"Invalid month: {self.month}")
        if not (1 <= self.day <= days_in_month(self.year, self.month)):
            raise ValueError(f"Invalid day: {self.day}")

    @property
    def days(self) -> int:
        return days_from_year(self.year) + days_before_month(self.year, self.month) + self.day - 1

    @property
    def day_of_year(self) -> int:
        return days_before_month(self.year, self.month) + self.day

    @property
    def day_of_week_index(self) -> int:
        return (self.days + 5) % 7

    def add_days(self, amount: int) -> "DateValue":
        return date_from_days(clamp_day(self.days + amount))

    def add_months(self, amount: int) -> "DateValue":
        month_index = self.year * 12 + (self.month - 1) + amount
        if month_index < 0:
            return DateValue(0, 1, 1)
        if month_index > 9999 * 12 + 11:
            return DateValue(9999, 12, 31)
        year = month_index // 12
        month = month_index % 12 + 1
        return DateValue(year, month, min(self.day, days_in_month(year, month)))

    def add_years(self, amount: int) -> "DateValue":
        return self.add_months(amount * 12)


@dataclass(frozen=True, order=True)
class TimeValue:
    """Independent wall-clock time value for generated time tests."""

    hour: int
    minute: int
    second: int
    nanosecond: int = 0

    def __post_init__(self) -> None:
        if not (0 <= self.hour <= 23):
            raise ValueError(f"Invalid hour: {self.hour}")
        if not (0 <= self.minute <= 59):
            raise ValueError(f"Invalid minute: {self.minute}")
        if not (0 <= self.second <= 59):
            raise ValueError(f"Invalid second: {self.second}")
        if not (0 <= self.nanosecond < NANOSECONDS_PER_SECOND):
            raise ValueError(f"Invalid nanosecond: {self.nanosecond}")

    @property
    def nanoseconds_since_midnight(self) -> int:
        return (
            ((self.hour * 60 + self.minute) * 60 + self.second) * NANOSECONDS_PER_SECOND + self.nanosecond
        )

    @classmethod
    def from_nanoseconds_since_midnight(cls, nanoseconds: int) -> "TimeValue":
        if not (0 <= nanoseconds < NANOSECONDS_PER_DAY):
            raise ValueError(f"Time outside a single day: {nanoseconds}")
        fraction = nanoseconds % NANOSECONDS_PER_SECOND
        seconds = nanoseconds // NANOSECONDS_PER_SECOND
        second = seconds % 60
        minutes = seconds // 60
        minute = minutes % 60
        hour = minutes // 60
        return cls(hour, minute, second, fraction)

    def added_nanoseconds(self, amount: int) -> "TimeValue":
        return self.from_nanoseconds_since_midnight(self.nanoseconds_since_midnight + amount)


@dataclass(frozen=True, order=True)
class DateTimeValue:
    """Independent UTC date-time value for generated time tests."""

    date: DateValue
    time: TimeValue

    @property
    def seconds_since_epoch(self) -> int:
        return self.date.days * SECONDS_PER_DAY + self.time.nanoseconds_since_midnight // NANOSECONDS_PER_SECOND

    @classmethod
    def from_seconds_since_epoch(cls, seconds: int) -> "DateTimeValue":
        if not (0 <= seconds <= LAST_VALID_SECOND):
            raise ValueError(f"Date-time seconds outside supported range: {seconds}")
        days = seconds // SECONDS_PER_DAY
        second_of_day = seconds % SECONDS_PER_DAY
        return cls(date_from_days(days), TimeValue.from_nanoseconds_since_midnight(second_of_day * NANOSECONDS_PER_SECOND))

    def add_seconds(self, seconds: int) -> "DateTimeValue":
        target_seconds = self.seconds_since_epoch + seconds
        if target_seconds < 0:
            return DateTimeValue(DateValue(0, 1, 1), TimeValue(0, 0, 0, 0))
        if target_seconds > LAST_VALID_SECOND:
            return DateTimeValue(DateValue(9999, 12, 31), TimeValue(23, 59, 59, 999_999_999))
        result = self.from_seconds_since_epoch(target_seconds)
        return DateTimeValue(
            result.date,
            TimeValue(result.time.hour, result.time.minute, result.time.second, self.time.nanosecond),
        )
