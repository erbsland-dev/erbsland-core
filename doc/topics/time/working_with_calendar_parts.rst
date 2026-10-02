..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Time; Calendar and Clock Parts
    single: Calendar Parts; Validation and Navigation
    single: DayOfWeek; Finding an Occurrence
    single: Ordinal Dates; Calendar Conversion

************************************
Working with Calendar and Time Parts
************************************

A date calculation becomes easier to read when its inputs say what they represent.
``Year{2024}``, :cpp:func:`Month::february() <erbsland::time::Month::february>`, and ``Day{29}`` carry more meaning than
three interchangeable integers.
The calendar and clock part types let you keep that meaning while validating input, finding month lengths, navigating
dates, and extracting fields from complete values.
This page explains the part model and develops the calendar helpers you can use in those calculations.

Positions and Quantities
========================

A part identifies a position within a calendar or clock: ``Day{15}`` is a day of a month, and ``Hour{9}`` is a reading
within a clock day.
An amount describes a quantity: ``Days{15}`` is fifteen days, and ``Hours{9}`` is nine hours.
The same distinction separates :cpp:class:`Month <erbsland::time::Month>` from
:cpp:type:`Months <erbsland::time::Months>` and :cpp:class:`Year <erbsland::time::Year>` from
:cpp:type:`Years <erbsland::time::Years>`.
:doc:`working_with_time_amounts` covers the quantity types and their arithmetic.

Typed parameters make the distinction visible at the point of use.
``Date::fromParts(Year{2024}, Month::february(), Day{29})`` expects each field in its own position.
Passing a :cpp:class:`Day <erbsland::time::Day>` where a :cpp:class:`Month <erbsland::time::Month>` is required does not
compile, and neither part is implicitly an unlabelled integer.
When you need its numeric label for output or another API, ``toValue()`` provides it explicitly.

A part guarantees its own numeric range, rather than the validity of every possible combination.
``Day{31}`` is a supported part, but it does not exist in February.
Separating these checks lets you retain a requested day while choosing how to handle shorter months.
For construction and validation of complete values, see :doc:`working_with_dates`, :doc:`working_with_times`, and
:doc:`working_with_datetime`.

Available Parts and Their Numbering
===================================

All the following types are in ``erbsland::time`` and are available through the flattened ``el`` namespace.
Their individual headers live under ``<erbsland/time/>``; ``<erbsland/time/all.hpp>`` includes the domain.
Default construction selects the minimum label for each part.
A default month is therefore January, and a default hour is zero.
These are usable positions; they are not invalid sentinels for missing input.

.. list-table::
    :header-rows: 1
    :widths: 20 25 15 40

    * - Part
      - Label range
      - Default
      - Meaning
    * - :cpp:class:`Year <erbsland::time::Year>`
      - ``0..9999``
      - ``0``
      - A proleptic Gregorian year, including year zero
    * - :cpp:class:`Month <erbsland::time::Month>`
      - ``1..12``
      - ``1``
      - January through December
    * - :cpp:class:`Day <erbsland::time::Day>`
      - ``1..31``
      - ``1``
      - A day within a month
    * - :cpp:class:`DayOfWeek <erbsland::time::DayOfWeek>`
      - ``0..6``
      - ``0``
      - Monday through Sunday
    * - :cpp:class:`DayOfYear <erbsland::time::DayOfYear>`
      - ``1..366``
      - ``1``
      - An ordinal day label; 366 needs a leap year
    * - :cpp:class:`Hour <erbsland::time::Hour>`
      - ``0..23``
      - ``0``
      - An hour within a day
    * - :cpp:class:`Minute <erbsland::time::Minute>`
      - ``0..59``
      - ``0``
      - A minute within an hour
    * - :cpp:class:`Second <erbsland::time::Second>`
      - ``0..59``
      - ``0``
      - A second within a minute

These labels use several numbering conventions.
Month, day, and ordinal-day labels start at one, while weekdays and clock parts start at zero.
The weekday numbering is Monday-based: ``DayOfWeek{0}`` is Monday, rather than Sunday.
Named factories make that convention easy to express without remembering a number.

Constructing and Checking a Part
================================

Integer construction clamps to the part's range.
``Month{13}`` becomes December, and ``Hour{-1}`` becomes midnight's hour zero.
That is useful when a bounded value is intentional, but it loses evidence that the original input was outside the range.
For input validation, call ``contains()`` on the raw integer before constructing the part.
After construction, the value is always in its own range.

Named factories such as :cpp:func:`Month::february() <erbsland::time::Month::february>` and
:cpp:func:`DayOfWeek::monday() <erbsland::time::DayOfWeek::monday>` make fixed choices readable.
There is a factory for each month and each weekday.
``first()`` selects the minimum label; ``last()`` selects the maximum for year, month, weekday, and clock parts.
:cpp:class:`Day <erbsland::time::Day>` and :cpp:class:`DayOfYear <erbsland::time::DayOfYear>` provide context-aware ``last()`` factories and ``lastMinimum()`` and ``lastMaximum()``
alternatives for the shortest and longest possible calendar bounds.
Their ``minimum()`` and ``maximum()`` aliases still refer to the numeric part range.

Parts compare with other values of the same type.
Boundary tests such as ``isFirst()`` and ``isLast()`` are useful when deciding whether a simple field change can
continue.
For :cpp:class:`Day <erbsland::time::Day>` and :cpp:class:`DayOfYear <erbsland::time::DayOfYear>`, ``isLast()`` takes
calendar context, so the test refers to the actual month or year end.

.. erbsland-demo::
    :source: time/CalendarParts/Construct.cpp
    :exec: time/calendar_parts --demo Construct
    :source-sha256: e012d3147a3bf2970b98a74728e770e1e51a8290f1a6142d79a67a1cabb90a40

.. code-block:: cpp

    /// Validate raw fields before clamping and keep positions distinct from quantities.
    /// @notest{Compiled and executed documentation demo.}
    void construct() {
        const auto observation = "Tangskov"_el;
        const auto rawMonth = 13;
        el::io::printLine(
            el::StringFormat{"{}: month accepted {}; clamped month {}"_el}.build(
                observation, el::Month::contains(rawMonth), el::Month{rawMonth}.toValue()));
        const auto day = el::Day{31};
        const auto date = el::Date::fromParts(el::Year{2024}, el::Month::february(), day);
        el::io::printLine(
            el::StringFormat{"Day in part range: {}; February date valid: {}"_el}.build(
                el::Day::contains(31), date.isValid()));
        auto hour = el::Hour{23};
        hour += el::Hours{2};
        el::io::printLine(
            el::StringFormat{"January first: {}; December last: {}; June < July: {}; clamped hour: {}"_el}.build(
                el::Month::january().isFirst(),
                el::Month::last().isLast(),
                el::Month::june() < el::Month::july(),
                hour.toValue()));
        el::io::printLine(
            el::StringFormat{"Year first/last: {}/{}; maximum day: {}; default second: {}"_el}.build(
                el::Year::first().toValue(),
                el::Year::last().toValue(),
                el::Day::lastMaximum().toValue(),
                el::Second{}.toValue()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Tangskov: month accepted false; clamped month 12
    Day in part range: true; February date valid: false
    January first: true; December last: true; June < July: true; clamped hour: 23
    Year first/last: 0/9999; maximum day: 31; default second: 0

.. erbsland-demo-end::

Arithmetic on a single part clamps instead of carrying into an adjacent field.
Adding two hours to ``Hour{23}`` leaves it at 23; it does not become hour 1 on another day.
Integer addition/subtraction, matching amount addition/subtraction, and increment/decrement share this bounded model.
``added()`` and ``subtracted()`` return a part, while ``add()`` and ``subtract()`` update it in place.
For midnight carry, calculate with :cpp:class:`Time <erbsland::time::Time>` as explained in :doc:`working_with_times`.
For calendar carry, the context-aware navigation helpers below return all affected fields together.

Moving Between Labels and Amounts
=================================

``toAmount()`` returns the offset from the part's minimum label.
It is not always a copy of the visible number.
``Day{1}`` maps to ``Days{0}``, and ``Month{1}`` maps to ``Months{0}``.
For a zero-based clock part, the label and offset agree: ``Hour{9}`` maps to ``Hours{9}``.
Years and weekdays also start at zero; ``DayOfYear{1}`` has a zero-day offset.

``fromAmount()`` performs the reverse mapping and clamps an offset that is outside the range.
``fromAmountOrThrow()`` throws :cpp:class:`OutOfRangeError <erbsland::err::OutOfRangeError>` instead.
A month offset of five selects month six, while offset twelve is already outside the twelve-month part range.
Keeping the word *offset* in mind helps when moving between human calendar labels and zero-based calculations.

.. erbsland-demo::
    :source: time/CalendarParts/Amounts.cpp
    :exec: time/calendar_parts --demo Amounts
    :source-sha256: 845546923cc26500264662c2b0fb33cae99a5cf809ff73241bd45e05d6174f43

.. code-block:: cpp

    /// Convert positions to offsets from their minimum and handle invalid offsets.
    /// @notest{Compiled and executed documentation demo.}
    void amounts() {
        const auto firstDay = el::Day{1};
        const auto firstMonth = el::Month{1};
        const auto hour = el::Hour{9};
        el::io::printLine(
            el::StringFormat{"Day 1 -> {} days; month 1 -> {} months; hour 9 -> {} hours"_el}.build(
                firstDay.toAmount().toRawValue(), firstMonth.toAmount().toRawValue(), hour.toAmount().toRawValue()));
        const auto day = el::Day::fromAmount(el::Days{14});
        const auto month = el::Month::fromAmountOrThrow(el::Months{5});
        el::io::printLine(
            el::StringFormat{"Offset 14 -> day {}; offset 5 -> month {}; day equals offset 14: {}"_el}.build(
                day.toValue(), month.toValue(), day == el::Days{14}));
        el::io::printLine(
            el::StringFormat{"Day plus 2: {}; negative offset clamped: {}"_el}.build(
                (day + el::Days{2}).toValue(), el::Day::fromAmount(el::Days{-1}).toValue()));
        try {
            const auto invalid = el::Month::fromAmountOrThrow(el::Months{12});
            el::io::printLine(el::StringFormat{"Month: {}"_el}.build(invalid.toValue()));
        } catch (const el::err::OutOfRangeError &) {
            el::io::printLine("Month offsets must be in 0..11."_el);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Day 1 -> 0 days; month 1 -> 0 months; hour 9 -> 9 hours
    Offset 14 -> day 15; offset 5 -> month 6; day equals offset 14: true
    Day plus 2: 17; negative offset clamped: 1
    Month offsets must be in 0..11.

.. erbsland-demo-end::

Comparisons with matching amounts use the same offset convention.
``Day{15} == Days{14}`` is true because the part is fourteen days after its minimum label.
Adding ``Days{2}`` to that part produces ``Day{17}``, still clamping within ``1..31``.
An amount comparison alone cannot establish that day 17 exists in a particular month; it only compares the part's
position within its numeric range.

Finding Lengths and Valid Days
==============================

Calendar lengths need context.
:cpp:func:`Year::isLeapYear() <erbsland::time::Year::isLeapYear>` follows the proleptic Gregorian rule, including year zero, and :cpp:func:`dayCount() <erbsland::time::Year::dayCount>` returns 365 or 366
days accordingly.
:cpp:func:`Month::dayCount(year) <erbsland::time::Month::dayCount>` gives the month's length, while :cpp:func:`lastDay(year) <erbsland::time::Month::lastDay>` gives the corresponding
:cpp:class:`Day <erbsland::time::Day>` label.
February is the only month with a variable length in this calendar.
:cpp:func:`hasFixedLength() <erbsland::time::Month::hasFixedLength>`, :cpp:func:`minimumDayCount() <erbsland::time::Month::minimumDayCount>`, and :cpp:func:`maximumDayCount() <erbsland::time::Month::maximumDayCount>` help describe a month before a year is known.

If you retain a requested day, :cpp:func:`Day::exists(year, month) <erbsland::time::Day::exists>` tells you whether it
can be used unchanged.
:cpp:func:`clamped(year, month) <erbsland::time::Day::clamped>` explicitly adjusts it to the last valid day when necessary.
:cpp:func:`Day::last(year, month) <erbsland::time::Day::last>` constructs that endpoint directly, and :cpp:func:`isLast(year, month) <erbsland::time::Day::isLast>` tests it.
This gives you a clear choice between rejecting a nonexistent day and intentionally accepting a month-end adjustment.

.. erbsland-demo::
    :source: time/CalendarParts/Lengths.cpp
    :exec: time/calendar_parts --demo Lengths
    :source-sha256: f05cb206f5e938532da6a36fc196d1f1571265883ecb9a26ac8ff6d67d7db89f

.. code-block:: cpp

    /// Find leap-year and month lengths and validate a day in its calendar context.
    /// @notest{Compiled and executed documentation demo.}
    void lengths() {
        const auto year = el::Year{2024};
        const auto month = el::Month::february();
        const auto requested = el::Day{31};
        const auto last = el::Day::last(year, month);
        el::io::printLine(
            el::StringFormat{"Leap year: {}; year days: {}; month days: {}; last day: {}"_el}.build(
                year.isLeapYear(),
                year.dayCount().toRawValue(),
                month.dayCount(year).toRawValue(),
                month.lastDay(year).toValue()));
        el::io::printLine(
            el::StringFormat{"Day 31 exists: {}; clamped: {}; contextual last: {}"_el}.build(
                requested.exists(year, month), requested.clamped(year, month).toValue(), last.isLast(year, month)));
        el::io::printLine(
            el::StringFormat{"February fixed length: {}; min/max days: {}/{}; common-year last ordinal: {}"_el}.build(
                month.hasFixedLength(),
                month.minimumDayCount().toRawValue(),
                month.maximumDayCount().toRawValue(),
                el::DayOfYear::last(el::Year{2023}).toValue()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Leap year: true; year days: 366; month days: 29; last day: 29
    Day 31 exists: false; clamped: 29; contextual last: true
    February fixed length: false; min/max days: 28/29; common-year last ordinal: 365

.. erbsland-demo-end::

An ordinal day needs the same care.
``DayOfYear::contains(366)`` is true because 366 fits the part range, but a common year ends at label 365.
:cpp:func:`DayOfYear::last(year) <erbsland::time::DayOfYear::last>` supplies the contextual endpoint, and :cpp:func:`isLast(year) <erbsland::time::DayOfYear::isLast>` tests that endpoint.
Validate an ordinal against the selected year's last day before using it as external input.
The part alone cannot tell you which year the caller intended.
For a supplied ordinal, first check its raw label, then check that the constructed part does not exceed
:cpp:func:`DayOfYear::last(year) <erbsland::time::DayOfYear::last>`.
That preserves the difference between accepting a value and silently adjusting it.

Navigating Across Calendar Boundaries
=====================================

Navigation differs from changing one bounded label.
``Month::december() + Months{1}`` stays December, while ``Month::december().next(Year{2024})`` returns January 2025. The
latter knows the year and returns :cpp:struct:`YearMonthParts <erbsland::time::YearMonthParts>` so that the carry is
retained.
:cpp:func:`previous(year) <erbsland::time::Month::previous>` works in the opposite direction.

A day has both month and year context.
:cpp:func:`Day::next(year, month) <erbsland::time::Day::next>` and :cpp:func:`previous(year, month) <erbsland::time::Day::previous>` return :cpp:struct:`DateParts <erbsland::time::DateParts>`,
carrying all three fields across a month or year boundary.
Start with a day that exists in that month, or explicitly clamp it first; these helpers are for navigating a calendar
position, rather than validating raw input.

.. erbsland-demo::
    :source: time/CalendarParts/Navigate.cpp
    :exec: time/calendar_parts --demo Navigate
    :source-sha256: 268cff3fd87b5ae4f6639f38dc92ddcf497c7892a0f9a1f76f22d1439414dbe4

.. code-block:: cpp

    /// Navigate across calendar boundaries with enough context to carry the year and month.
    /// @notest{Compiled and executed documentation demo.}
    void navigate() {
        const auto year = el::Year{2024};
        const auto month = el::Month::december();
        const auto nextMonth = month.next(year);
        const auto previousMonth = nextMonth.month.previous(nextMonth.year);
        const auto nextDay = el::Day{31}.next(year, month);
        const auto previousDay = nextDay.day.previous(nextDay.year, nextDay.month);
        el::io::printLine(
            el::StringFormat{"Next month: {}-{}; back: {}-{}; next day: {}; back: {}"_el}.build(
                nextMonth.year.toValue(),
                nextMonth.month.toValue(),
                previousMonth.year.toValue(),
                previousMonth.month.toValue(),
                el::Date::fromParts(nextDay.year, nextDay.month, nextDay.day).toString(),
                el::Date::fromParts(previousDay.year, previousDay.month, previousDay.day).toString()));
        el::io::printLine(
            el::StringFormat{"Year next/previous: {}/{}; final year has next: {}; first year has previous: {}"_el}.build(
                year.next().toValue(),
                year.previous().toValue(),
                el::Year::last().hasNext(),
                el::Year::first().hasPrevious()));
        el::io::printLine(
            el::StringFormat{"Month next/previous available: {}/{}; day next/previous available: {}/{}"_el}.build(
                month.hasNext(el::Year::last()),
                el::Month::january().hasPrevious(el::Year::first()),
                el::Day{31}.hasNext(el::Year::last(), month),
                el::Day{1}.hasPrevious(el::Year::first(), el::Month::january())));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Next month: 2025-1; back: 2024-12; next day: 2025-01-01; back: 2024-12-31
    Year next/previous: 2025/2023; final year has next: false; first year has previous: false
    Month next/previous available: false/false; day next/previous available: false/false

.. erbsland-demo-end::

Each navigation family has ``hasNext()`` and ``hasPrevious()`` checks with the same required context.
:cpp:class:`Year <erbsland::time::Year>` also provides parameterless ``next()`` and ``previous()``.
At the supported outer boundaries, navigation retains the endpoint instead of wrapping to the opposite end.
For iteration, the availability checks let you stop without repeatedly receiving the final value.
A month-selection loop can retain the returned year/month pair each time it advances; a day-selection loop retains all
three date fields.
The returned structures make the carry explicit so that updating the month cannot accidentally leave an old year beside
it.
If you already have a complete :cpp:class:`Date <erbsland::time::Date>`, its navigation API often expresses the same
task more directly; see
:doc:`working_with_dates`.

Finding a Weekday Occurrence
============================

A weekday can describe a target in a calendar rule.
:cpp:func:`daysToNext(target) <erbsland::time::DayOfWeek::daysToNext>` returns a nonnegative :cpp:type:`Days <erbsland::time::Days>` offset from the current weekday to the target.
:cpp:func:`daysToPrevious(target) <erbsland::time::DayOfWeek::daysToPrevious>` returns a nonpositive offset going backward.
You can add either result directly to a :cpp:class:`Date <erbsland::time::Date>`; the sign already expresses the
direction.

Both methods return zero when the current weekday equals the target.
They therefore find an occurrence on or after, or on or before, the starting date.
If your rule requires a strictly later occurrence, handle zero by choosing ``Days{7}``; for a strictly earlier one,
choose ``Days{-7}``.

.. erbsland-demo::
    :source: time/CalendarParts/Weekdays.cpp
    :exec: time/calendar_parts --demo Weekdays
    :source-sha256: 07c9bfd252ef5c7ce77d448cc1aaa5daa8149d0b140a4dc866e82cb02d820339

.. code-block:: cpp

    /// Find a weekday on or after a date, or on or before it, with signed day offsets.
    /// @notest{Compiled and executed documentation demo.}
    void weekdays() {
        const auto date = el::Date{el::Year{2024}, el::Month{6}, el::Day{12}};
        const auto weekday = date.dayOfWeek();
        const auto target = el::DayOfWeek::monday();
        const auto forward = weekday.daysToNext(target);
        const auto backward = weekday.daysToPrevious(target);
        el::io::printLine(
            el::StringFormat{"{}: {} / {}; Monday is {}"_el}.build(
                date.toString(), weekday.toString(), weekday.toString(el::DayOfWeekFormat::Short), target.toValue()));
        el::io::printLine(
            el::StringFormat{"Next Monday: {} ({} days); previous Monday: {} ({} days)"_el}.build(
                date.added(forward).toString(),
                forward.toRawValue(),
                date.added(backward).toString(),
                backward.toRawValue()));
        el::io::printLine(
            el::StringFormat{"Equal weekday next/previous: {}/{} days"_el}.build(
                target.daysToNext(target).toRawValue(), target.daysToPrevious(target).toRawValue()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    2024-06-12: Wednesday / Wed; Monday is 0
    Next Monday: 2024-06-17 (5 days); previous Monday: 2024-06-10 (-2 days)
    Equal weekday next/previous: 0/0 days

.. erbsland-demo-end::

:cpp:func:`DayOfWeek::toString() <erbsland::time::DayOfWeek::toString>` returns the English long name by default.
:cpp:enumerator:`DayOfWeekFormat::Short <erbsland::time::DayOfWeekFormat::Short>` selects the three-letter form, such as ``Mon``; :cpp:enumerator:`Long <erbsland::time::DayOfWeekFormat::Long>` selects ``Monday``.
These names are useful display labels, while the typed weekday and day offsets carry the calculation's meaning.

Converting Ordinal Dates
========================

An ordinal date identifies a year and a one-based day within it.
For example, day 60 of leap year 2024 is February 29. :cpp:func:`Year::monthOfDay() <erbsland::time::Year::monthOfDay>`
finds its month, and
:cpp:func:`Month::extractMonthAndDay() <erbsland::time::Month::extractMonthAndDay>` returns :cpp:struct:`MonthDayParts <erbsland::time::MonthDayParts>` when you also need the day label.
That extraction accepts either a :cpp:class:`DayOfYear <erbsland::time::DayOfYear>` or a zero-based
:cpp:type:`Days <erbsland::time::Days>` offset.
Choose the overload that matches your input convention, rather than manually adding or removing one at every call.

:cpp:func:`Year::daysBeforeMonth() <erbsland::time::Year::daysBeforeMonth>` returns a zero-based count of preceding days.
:cpp:func:`Month::firstDayOfYear() <erbsland::time::Month::firstDayOfYear>` and :cpp:func:`lastDayOfYear() <erbsland::time::Month::lastDayOfYear>` instead return one-based ordinal labels.
These helpers are useful when turning a month selection into an ordinal range or checking where an ordinal falls.

.. erbsland-demo::
    :source: time/CalendarParts/Ordinal.cpp
    :exec: time/calendar_parts --demo Ordinal
    :source-sha256: 44cdbb0082ac799155b2d975a3bdd6b73213af6caeaad3ad737ca5e30c081173

.. code-block:: cpp

    /// Resolve ordinal dates and keep one-based day labels separate from zero-based offsets.
    /// @notest{Compiled and executed documentation demo.}
    void ordinal() {
        const auto year = el::Year{2024};
        const auto ordinal = el::DayOfYear{60};
        const auto monthDay = el::Month::extractMonthAndDay(year, ordinal);
        const auto date = el::Date::fromParts(year, monthDay.month, monthDay.day);
        const auto extracted = el::Year::extractFromEpoch(date.toDaysSinceEpoch());
        const auto fromOffset = el::Month::extractMonthAndDay(extracted.year, extracted.dayOfYear);
        el::io::printLine(
            el::StringFormat{"Ordinal {}: {}; month from year: {}; zero-based offset: {}; recovered day: {}"_el}.build(
                ordinal.toValue(),
                date.toString(),
                year.monthOfDay(ordinal).toValue(),
                extracted.dayOfYear.toRawValue(),
                fromOffset.day.toValue()));
        const auto march = el::Month::march();
        el::io::printLine(
            el::StringFormat{"Days before March: {}; March ordinals: {}..{}; year-end ordinal is last: {}"_el}.build(
                year.daysBeforeMonth(march).toRawValue(),
                march.firstDayOfYear(year).toValue(),
                march.lastDayOfYear(year).toValue(),
                el::DayOfYear::last(year).isLast(year)));
        el::io::printLine(
            el::StringFormat{"366 in part range: {}; last ordinal in 2023: {}"_el}.build(
                el::DayOfYear::contains(366), el::DayOfYear::last(el::Year{2023}).toValue()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Ordinal 60: 2024-02-29; month from year: 2; zero-based offset: 59; recovered day: 29
    Days before March: 60; March ordinals: 61..91; year-end ordinal is last: true
    366 in part range: true; last ordinal in 2023: 365

.. erbsland-demo-end::

:cpp:func:`Year::extractFromEpoch() <erbsland::time::Year::extractFromEpoch>` splits a count of days since the Core epoch into :cpp:struct:`YearDayOfYearParts <erbsland::time::YearDayOfYearParts>`.
Its ``year`` field identifies the containing year, and its ``dayOfYear`` field is a zero-based
:cpp:type:`Days <erbsland::time::Days>` amount.
Despite the field name, it is not a :cpp:class:`DayOfYear <erbsland::time::DayOfYear>` label.
Passing that amount to :cpp:func:`Month::extractMonthAndDay() <erbsland::time::Month::extractMonthAndDay>` preserves the
zero-based convention through the conversion.
:cpp:func:`Year::daysSinceEpoch() <erbsland::time::Year::daysSinceEpoch>` supplies the count at the start of a year.
The Core epoch is ``0000-01-01``; see :doc:`working_with_dates` for date/count conversion.

These extraction helpers use clamping for out-of-range inputs rather than reporting a validation error.
Epoch extraction clamps to the supported first or last date.
Month/day extraction first clamps its ordinal input to the part range; that alone does not reject day 366 in a common
year.
Validate the original count or contextual ordinal before extraction when you need to reject unsupported input.

Keeping Related Fields Together
===============================

:cpp:func:`Date::parts() <erbsland::time::Date::parts>` and :cpp:func:`Time::parts() <erbsland::time::Time::parts>` return :cpp:struct:`DateParts <erbsland::time::DateParts>` and :cpp:struct:`TimeParts <erbsland::time::TimeParts>`.
They name the extracted fields and keep your code readable when several fields are needed together.
You can inspect or copy those fields without choosing an arbitrary tuple ordering, then pass them to a constructor whose
typed parameters make their roles clear.
:cpp:func:`DateTime::parts() <erbsland::time::DateTime::parts>` returns :cpp:struct:`DateTimeParts <erbsland::time::DateTimeParts>` containing the local display fields, rather than its UTC fields.
When working with a named zone, extracting all local fields together avoids repeating the same conversion for each
individual accessor.

The date fields are ``year``, ``month``, and ``day``.
The time fields are ``hour``, ``minute``, ``second``, and ``nanosecondFraction``;
:cpp:struct:`DateTimeParts <erbsland::time::DateTimeParts>` combines both groups.
A valid fractional-second value is ``0..999,999,999`` nanoseconds.
The fraction uses :cpp:type:`Nanoseconds <erbsland::time::Nanoseconds>`, which is an amount capable of holding values
beyond that range, so an aggregate itself does not guarantee a valid fraction or complete calendar combination.
:cpp:class:`Time <erbsland::time::Time>` clamps the fraction during reconstruction, while :cpp:func:`Date::fromParts() <erbsland::time::Date::fromParts>` returns an invalid date for a
nonexistent combination and :cpp:func:`fromPartsOrThrow() <erbsland::time::Date::fromPartsOrThrow>` reports that error
explicitly.

.. erbsland-demo::
    :source: time/CalendarParts/Aggregates.cpp
    :exec: time/calendar_parts --demo Aggregates
    :source-sha256: bd4b13a8e44e2dcea9a5c72c39e77a5ec92315706f90a33f9e78fe5f8e929c53

.. code-block:: cpp

    /// Extract named fields and reconstruct complete values with calendar validation and a zone.
    /// @notest{Compiled and executed documentation demo.}
    void aggregates() {
        const auto date = el::Date{el::Year{2024}, el::Month{6}, el::Day{12}};
        const auto time = el::Time{el::Hour{9}, el::Minute{15}, el::Second{30}, el::Nanoseconds{123'456'789}};
        const auto dateParts = date.parts();
        const auto timeParts = time.parts();
        const auto rebuiltDate = el::Date::fromPartsOrThrow(dateParts.year, dateParts.month, dateParts.day);
        const auto rebuiltTime = el::Time{timeParts.hour, timeParts.minute, timeParts.second, timeParts.nanosecondFraction};
        const auto zone = el::TimeZone::fromNameOrThrow("Europe/Copenhagen"_el);
        const auto instant = el::DateTime{rebuiltDate, rebuiltTime, zone};
        const auto local = instant.parts();
        const auto rebuilt = el::DateTime{
            el::Date::fromParts(local.year, local.month, local.day),
            el::Time{local.hour, local.minute, local.second, local.nanosecondFraction},
            zone};
        el::io::printLine(
            el::StringFormat{"Date/time retained: {}/{}; local/UTC hour: {}/{}; reconstructed: {}"_el}.build(
                rebuiltDate == date,
                rebuiltTime == time,
                local.hour.toValue(),
                instant.utcTime().hour().toValue(),
                rebuilt == instant));
        // Aggregates can hold combinations that require checking when reconstructed.
        const auto impossible = el::DateParts{el::Year{2023}, el::Month::february(), el::Day{29}};
        el::io::printLine(
            el::StringFormat{"Aggregate date valid: {}; fraction retained: {} ns"_el}.build(
                el::Date::fromParts(impossible.year, impossible.month, impossible.day).isValid(),
                local.nanosecondFraction.toRawValue()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Date/time retained: true/true; local/UTC hour: 9/7; reconstructed: true
    Aggregate date valid: false; fraction retained: 123456789 ns

.. erbsland-demo-end::

Reconstruction uses the named fields with the complete value's constructors or factories.
A :cpp:struct:`DateTimeParts <erbsland::time::DateTimeParts>` has no zone or fold choice, so keep those alongside the
fields when reconstructing a dated instant.
During an ambiguous local hour, the fields and a named zone still need the appropriate
:cpp:enum:`TimeOccurrenceInFold <erbsland::time::TimeOccurrenceInFold>` to identify the original occurrence.
If your purpose is to preserve an exact instant, the timestamp representations in :doc:`working_with_timestamps` provide
a more direct path.

The smaller result structures follow the same approach: :cpp:struct:`YearMonthParts <erbsland::time::YearMonthParts>`
names a carried year/month pair,
:cpp:struct:`MonthDayParts <erbsland::time::MonthDayParts>` names an extracted month/day pair, and :cpp:struct:`YearDayOfYearParts <erbsland::time::YearDayOfYearParts>` retains a year with its zero-based day
offset.
They are ordinary value structures with equality and named fields, useful for passing related calculation results
without losing their meaning.
:doc:`understanding_calendar_changes` builds on these calendar rules when several changes belong in one operation.
