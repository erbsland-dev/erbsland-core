# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import calendar
import re
from dataclasses import dataclass, field
from datetime import date, datetime, timedelta
from enum import IntEnum

from lib.error import UtilityError


MONTHS = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]
DAYS_OF_WEEK = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]
SECONDS_AT_YEAR_ONE = 366 * 24 * 60 * 60


class DayRule(IntEnum):
    EXACT = 0
    FIRST_AFTER = 1
    LAST_BEFORE = 2
    LAST = 3


class TimeReference(IntEnum):
    UTC = 0
    WALL = 1
    STD = 2


@dataclass(frozen=True)
class TimeValue:
    seconds: int
    reference: TimeReference = TimeReference.WALL


@dataclass(frozen=True)
class RuleTransition:
    dt: datetime
    reference: TimeReference
    dst_offset_minutes: int


def parse_time_value(text: str, *, allow_empty: bool = False) -> TimeValue | None:
    if not text:
        if allow_empty:
            return None
        raise UtilityError("Empty time value.")
    reference = TimeReference.WALL
    suffix = text[-1:]
    if suffix in {"u", "g", "z"}:
        reference = TimeReference.UTC
        text = text[:-1]
    elif suffix == "s":
        reference = TimeReference.STD
        text = text[:-1]
    elif suffix == "w":
        text = text[:-1]
    sign = -1 if text.startswith("-") else 1
    if text.startswith(("-", "+")):
        text = text[1:]
    if not text:
        return TimeValue(0, reference)
    parts = [int(part) for part in text.split(":")]
    if len(parts) > 3:
        raise UtilityError(f"Unsupported time value: {text!r}")
    while len(parts) < 3:
        parts.append(0)
    return TimeValue(sign * (parts[0] * 3600 + parts[1] * 60 + parts[2]), reference)


def epoch_seconds(dt: datetime) -> int:
    return int((dt - datetime(1, 1, 1)).total_seconds()) + SECONDS_AT_YEAR_ONE


def format_offset(seconds: int) -> str:
    sign = "-" if seconds < 0 else "+"
    seconds = abs(seconds)
    hour = seconds // 3600
    minute = (seconds // 60) % 60
    second = seconds % 60
    if second:
        return f"{sign}{hour:02d}{minute:02d}{second:02d}"
    if minute:
        return f"{sign}{hour:02d}{minute:02d}"
    return f"{sign}{hour:02d}"


def resolve_day(year: int, month: int, day_text: str | None) -> int:
    if day_text is None:
        return 1
    if day_text.startswith("last"):
        weekday = DAYS_OF_WEEK.index(day_text[4:])
        last_day = calendar.monthrange(year, month)[1]
        candidates = [day for day in range(1, last_day + 1) if date(year, month, day).weekday() == weekday]
        return candidates[-1]
    if ">=" in day_text:
        weekday_text, start_text = day_text.split(">=", 1)
        weekday = DAYS_OF_WEEK.index(weekday_text)
        start = int(start_text)
        last_day = calendar.monthrange(year, month)[1]
        candidates = [day for day in range(start, last_day + 1) if date(year, month, day).weekday() == weekday]
        return candidates[0]
    if "<=" in day_text:
        weekday_text, end_text = day_text.split("<=", 1)
        weekday = DAYS_OF_WEEK.index(weekday_text)
        end = int(end_text)
        candidates = [day for day in range(1, end + 1) if date(year, month, day).weekday() == weekday]
        return candidates[-1]
    return int(day_text)


@dataclass
class Rule:
    name: str
    year_from: int
    year_to_text: str
    month_text: str
    day_text: str
    at_text: str
    save_text: str
    letters: str

    @classmethod
    def parse(cls, line: str) -> Rule:
        parts = line.split()
        if len(parts) < 10:
            raise UtilityError(f"Unsupported rule line: {line}")
        return cls(parts[1], int(parts[2]), parts[3], parts[5], parts[6], parts[7], parts[8], parts[9])

    @property
    def year_to(self) -> int:
        if self.year_to_text == "only":
            return self.year_from
        if self.year_to_text == "max":
            return 10_000
        return int(self.year_to_text)

    def parse_month(self) -> int:
        return MONTHS.index(self.month_text) + 1

    def parse_day(self) -> tuple[int, int, DayRule]:
        if self.day_text.startswith("last"):
            return 0, DAYS_OF_WEEK.index(self.day_text[4:]) + 1, DayRule.LAST
        if ">=" in self.day_text:
            weekday, day = self.day_text.split(">=", 1)
            return int(day), DAYS_OF_WEEK.index(weekday) + 1, DayRule.FIRST_AFTER
        if "<=" in self.day_text:
            weekday, day = self.day_text.split("<=", 1)
            return int(day), DAYS_OF_WEEK.index(weekday) + 1, DayRule.LAST_BEFORE
        return int(self.day_text), 0, DayRule.EXACT

    @property
    def save_minutes(self) -> int:
        value = parse_time_value(self.save_text)
        if value is None or value.seconds % 60 != 0:
            raise UtilityError(f"Unsupported non-minute SAVE value in rule {self.name}: {self.save_text}")
        return value.seconds // 60

    def active_for_year(self, year: int) -> bool:
        return self.year_from <= year <= self.year_to

    def transition(self, year: int) -> RuleTransition:
        month = self.parse_month()
        day = resolve_day(year, month, self.day_text)
        at = parse_time_value(self.at_text)
        if at is None:
            raise UtilityError(f"Missing AT value in rule {self.name}.")
        midnight = datetime(year, month, day)
        dt = midnight + timedelta(seconds=at.seconds)
        return RuleTransition(dt, at.reference, self.save_minutes)

    def encoded(self) -> tuple[int, str]:
        year_to = self.year_to
        if self.year_from <= 0 or self.year_from > 9999 or year_to < self.year_from or year_to > 10_000:
            raise UtilityError(f"Rule {self.name} has unsupported year range.")
        year_delta = 0xFF if year_to >= 10_000 else year_to - self.year_from
        if year_delta > 0xFE and year_delta != 0xFF:
            raise UtilityError(f"Rule {self.name} year range is too large.")
        month = self.parse_month() - 1
        day, day_of_week, day_rule = self.parse_day()
        at = parse_time_value(self.at_text)
        if at is None:
            raise UtilityError(f"Rule {self.name} has no AT value.")
        if at.seconds % 60 != 0:
            raise UtilityError(f"Rule {self.name} has non-minute AT value.")
        at_minutes = at.seconds // 60
        save_minutes = self.save_minutes
        value = 0
        value |= (self.year_from & 0x3FFF) << 0
        value |= (year_delta & 0xFF) << 14
        value |= (month & 0x0F) << 22
        value |= (day & 0x1F) << 26
        value |= (day_of_week & 0x07) << 31
        value |= (int(day_rule) & 0x03) << 34
        value |= (at_minutes & 0x07FF) << 36
        value |= (int(at.reference) & 0x03) << 47
        value |= (save_minutes & 0x7FFF) << 49
        comment = (
            f"// y:{self.year_from}-{year_to} m:{month} d:{day} dow:{day_of_week} "
            f"dr:{int(day_rule)} t:{at_minutes} tr:{int(at.reference)} save:{save_minutes}"
        )
        return value, comment


@dataclass
class RuleSet:
    name: str
    rules: list[Rule] = field(default_factory=list)
    id: int = -1

    @property
    def first_year(self) -> int:
        return min(rule.year_from for rule in self.rules)

    def active_rules(self, year: int) -> list[Rule]:
        return [rule for rule in self.rules if rule.active_for_year(year)]

    def last_transition_points(self, year: int) -> list[RuleTransition]:
        result: list[RuleTransition] = []
        while year >= self.first_year and len(result) < 4:
            result.extend(rule.transition(year) for rule in self.active_rules(year))
            year -= 1
        result.sort(key=lambda item: item.dt)
        return result


@dataclass
class ZoneEntry:
    std_offset_text: str
    rule_set_name: str
    format_text: str
    until_text: str = ""
    rule_set: RuleSet | None = None

    @classmethod
    def parse(cls, line: str) -> ZoneEntry:
        parts = line.split()
        if parts[0] == "Zone":
            parts = parts[2:]
        if len(parts) < 3:
            raise UtilityError(f"Unsupported zone line: {line}")
        until_text = " ".join(parts[3:]) if len(parts) > 3 else ""
        return cls(parts[0], parts[1], parts[2], until_text)

    @property
    def std_offset_seconds(self) -> int:
        value = parse_time_value(self.std_offset_text)
        if value is None:
            return 0
        return value.seconds

    @property
    def rule_offset_seconds(self) -> int:
        if self.rule_set_name == "-":
            return 0
        value = parse_time_value(self.rule_set_name)
        if value is None:
            return 0
        return value.seconds

    @property
    def is_rule_set_based(self) -> bool:
        return self.rule_set_name != "-" and not re.match(r"^[+-]?\d", self.rule_set_name)

    def parse_until(self) -> tuple[datetime, TimeReference]:
        if not self.until_text:
            raise UtilityError("Cannot parse an empty UNTIL value.")
        parts = self.until_text.split()
        year = int(parts[0])
        month = MONTHS.index(parts[1]) + 1 if len(parts) >= 2 else 1
        day_text = parts[2] if len(parts) >= 3 else None
        time_text = parts[3] if len(parts) >= 4 else "0"
        time_value = parse_time_value(time_text)
        if time_value is None:
            time_value = TimeValue(0)
        day = resolve_day(year, month, day_text)
        dt = datetime(year, month, day) + timedelta(seconds=time_value.seconds)
        return dt, time_value.reference

    def abbreviation_text(self, letter: str, offset_seconds: int | None = None) -> str:
        if self.format_text in {"", "-"} or self.format_text.startswith(("-", "+")):
            return ""
        result = self.format_text
        uses_numeric_offset = "%z" in result
        if letter == "-":
            letter = ""
        if "%s" in result:
            result = result.replace("%s", letter)
        if "%z" in result:
            if offset_seconds is None:
                offset_seconds = self.std_offset_seconds + self.rule_offset_seconds
            result = result.replace("%z", format_offset(offset_seconds))
        return "" if result.startswith(("-", "+")) and not uses_numeric_offset else result

    def utc_dt(self, dt: datetime, reference: TimeReference, standard_offset: int, dst_offset: int) -> datetime:
        if reference == TimeReference.UTC:
            return dt
        if reference == TimeReference.STD:
            return dt - timedelta(seconds=standard_offset)
        return dt - timedelta(seconds=standard_offset + dst_offset)

    def last_effective_offset(self, transition_time: datetime, transition_reference: TimeReference) -> tuple[datetime, int]:
        standard_offset = self.std_offset_seconds
        if self.rule_set is None:
            dst_offset = self.rule_offset_seconds
            return self.utc_dt(transition_time, transition_reference, standard_offset, dst_offset), standard_offset + dst_offset
        points = self.rule_set.last_transition_points(transition_time.year)
        if len(points) <= 1:
            last_dst_offset = 0
            points = points[1:]
        else:
            last_dst_offset = points[0].dst_offset_minutes * 60
            points = points[1:]
        for point in points:
            utc_point = self.utc_dt(point.dt, point.reference, standard_offset, last_dst_offset)
            if self.utc_dt(transition_time, transition_reference, standard_offset, last_dst_offset) >= utc_point:
                break
            last_dst_offset = point.dst_offset_minutes * 60
        return (
            self.utc_dt(transition_time, transition_reference, standard_offset, last_dst_offset),
            standard_offset + last_dst_offset,
        )


@dataclass
class Zone:
    name: str
    region: str
    entries: list[ZoneEntry] = field(default_factory=list)
    id: int = -1
    function_name: str = ""

    def all_abbreviations(self) -> list[str]:
        result: set[str] = set()
        for entry in self.entries:
            if entry.format_text in {"", "-"} or entry.format_text.startswith(("-", "+")):
                continue
            if "%s" not in entry.format_text and "%z" not in entry.format_text:
                result.add(entry.format_text)
                continue
            if entry.rule_set is None:
                text = entry.abbreviation_text("", entry.std_offset_seconds + entry.rule_offset_seconds)
                if text:
                    result.add(text)
                continue
            letters = {rule.letters for rule in entry.rule_set.rules}
            letters.add("")
            for rule in entry.rule_set.rules:
                letters.add(rule.letters)
                text = entry.abbreviation_text(rule.letters, entry.std_offset_seconds + rule.save_minutes * 60)
                if text:
                    result.add(text)
        return sorted(result)
