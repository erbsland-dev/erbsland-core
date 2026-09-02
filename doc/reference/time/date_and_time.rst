..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Date and Time; Reference
    single: Date and Time Types
    single: Duration and Time Amounts

*************
Date and Time
*************

Date and Time Types
===================

Introduction
------------

The ``time`` namespace provides types for working with civil dates, wall-clock times, and time spans.
The calendar helper structs are small aggregate result types for APIs that return more than one calendar component.
They keep call sites readable by naming the returned values.

.. code-block:: cpp

    const auto parts = date.parts();
    const auto year = parts.year;
    const auto month = parts.month;
    const auto day = parts.day;

    const auto next = el::Month::december().next(el::Year{2026});
    const auto nextMonth = next.month;

Date
~~~~

:cpp:class:`Date <erbsland::time::Date>` stores a civil date in the proleptic Gregorian calendar.
The internal epoch is ``0000-01-01`` and raw day zero is that date.
The supported range is ``0000-01-01`` through ``9999-12-31``.
Use :cpp:func:`parts() <erbsland::time::Date::parts>` when you need named
:cpp:struct:`DateParts <erbsland::time::DateParts>` instead of separate accessor calls.

Date arithmetic saturates at the supported range.
For example, adding a negative day count to the first supported date keeps the result at ``0000-01-01``.
Use ``wouldAddSaturate()`` to test for this condition and ``addedOrThrow()`` or ``addOrThrow()`` to reject it with
:cpp:class:`OverflowError <erbsland::err::OverflowError>`.

Date Time
~~~~~~~~~

:cpp:class:`DateTime <erbsland::time::DateTime>` represents an instant as UTC date and time plus display offset
information.
It converts to and from ISO text, ``std::time_t``, exact typed ticks from the Core, POSIX, Windows, and RFC 868 epochs,
fixed offsets, and supported named time zones.
Tick conversion accepts nanoseconds, microseconds, milliseconds, or seconds; a conversion fails when it would lose
fractional precision, precedes the selected epoch, or exceeds the selected unit's range.
Use the split seconds-and-nanoseconds conversion for external formats that need their own fractional representation,
such as Windows ``FILETIME``.
The value is stored internally as a UTC instant; local accessors and
:cpp:func:`parts() <erbsland::time::DateTime::parts>` use the display offset or named time zone.

Date-time arithmetic is performed on the UTC instant and saturates at ``DateTime::first()`` or ``DateTime::last()`` when
the result would leave the supported range.
Use ``wouldAddSaturate()`` or ``wouldSubtractSaturate()`` to test for this condition, and use the ``...OrThrow()``
variants to reject it with
:cpp:class:`OverflowError <erbsland::err::OverflowError>`.

A ``TimeWithZone`` combines a wall-clock
:cpp:class:`Time <erbsland::time::Time>` with a :cpp:class:`TimeZone <erbsland::time::TimeZone>`.  It does not
represent an instant until it is combined with a date.
Named zones therefore render using their IANA name instead of inventing a numeric offset.
Constructing a ``DateTime`` from a date and ``TimeWithZone`` resolves the exact offset for the civil date and time,
converts the value to internally stored UTC, and retains the display zone.

Time
~~~~

:cpp:class:`Time <erbsland::time::Time>` stores a wall-clock time of day with nanosecond precision.
Arithmetic that crosses midnight returns the day wrap separately.

Use :cpp:struct:`TimeParts <erbsland::time::TimeParts>` and
:cpp:struct:`TimeWrapResult <erbsland::time::TimeWrapResult>` as named aggregate results:

.. code-block:: cpp

    const auto parts = time.parts();
    const auto hour = parts.hour;

    const auto wrapped = time.addedWithWrap(el::Duration{el::Hours{3}});
    const auto dayCarry = wrapped.days;
    const auto newTime = wrapped.time;

Time Zone
~~~~~~~~~

:cpp:class:`TimeZone <erbsland::time::TimeZone>` represents UTC, a fixed offset, or a supported named IANA zone.
Lookup factories return ``std::optional`` for tolerant lookup and ``OrThrow`` variants for explicit failure.
Fixed offsets are normalized only by complete 24-hour rotations, so offsets such as UTC+13 and UTC+14 retain their
direction.

``TimeZone::local()`` identifies and caches the operating system's base time zone for the process lifetime.
POSIX systems use ``TZ`` and canonical zoneinfo paths; Windows names are mapped to IANA names using generated Unicode
CLDR data.
Resolution failures produce UTC with the local-origin marker set.
Combining the zone with a civil date and time always resolves the bundled database again, retaining historical shifts,
daylight-saving state, abbreviations, gaps, and folds.

The local-origin marker records that the current display zone came from the system setting.
Copies, arithmetic, and UTC normalization preserve it.
Explicit zone conversion replaces it with the target zone's marker: conversion to UTC or an explicit zone clears it,
while conversion to ``TimeZone::local()`` sets it.
Default string formatting omits the zone for local-origin values.
``DateTime::toIsoString()`` with ``IsoTimeFormat::TimeShift`` still forces the resolved numeric offset.

Duration and Time Amounts
=========================

Introduction
------------

Duration and time span types store signed time intervals at different resolutions.

:cpp:class:`Duration <erbsland::time::Duration>` stores a signed span with second resolution. Conversions to coarser
parts truncate toward zero.
Conversions to nanosecond precision, such as ``toTimeDelta()``, saturate if the represented nanoseconds exceed the
target type.
Use ``wouldConvertToTimeDeltaSaturate()`` or ``toTimeDeltaOrThrow()`` when saturation must be detected or rejected.

:cpp:class:`TimeDelta <erbsland::time::TimeDelta>` stores a signed span with nanosecond resolution. Conversion to
:cpp:class:`Duration <erbsland::time::Duration>` truncates sub-second nanoseconds toward zero.
``toSecondsWithFractions()`` and ``toDaysWithFractions()`` return approximate floating-point values and do not treat
rounding as an error.
The unit factories from ``nanoseconds()`` through ``weeks()`` saturate when conversion exceeds the stored nanosecond
range.
Use the corresponding ``...OrThrow()`` factory, including ``weeksOrThrow()``, when overflow must be rejected.

:cpp:class:`CalendarDelta <erbsland::time::CalendarDelta>` stores nanoseconds through years as independent signed
components.
It deliberately does not normalize its stored parts: one month remains one month, and mixed positive and negative
components remain visible through the typed accessors.
Conversion to ``TimeDelta`` is available only when the month and year components are zero and the exact fixed-unit sum
fits the nanosecond range.

Applying a ``CalendarDelta`` to a ``DateTime`` processes nanoseconds, microseconds, milliseconds, seconds, minutes,
hours, days, weeks, months, and years in that order.
Month and year steps use the same end-of-month clamping semantics as ``Date``.
Arithmetic is performed on the UTC representation; fixed display offsets are retained and named-zone metadata is
refreshed for the final instant.

:cpp:class:`TimeDeltaFormat <erbsland::time::TimeDeltaFormat>` controls short or long names, separators, the smallest
fixed unit, and fractional output.
``TimeDeltaFormat::elcl()`` selects the aliases and separators required for ELCL serialization.
Calendar-delta formatting always emits non-zero years and months independently and uses exact signed normalization for
the fixed units without first forcing the total into ``TimeDelta``.

Interface
=========

.. doxygenclass:: erbsland::time::CalendarDelta
    :members:
.. doxygenstruct:: erbsland::time::CalendarDeltaParts
    :members:
.. doxygenstruct:: erbsland::time::YearDayOfYearParts
    :members:

.. doxygenstruct:: erbsland::time::YearMonthParts
    :members:

.. doxygenstruct:: erbsland::time::MonthDayParts
    :members:

.. doxygenstruct:: erbsland::time::DateParts
    :members:
.. doxygenclass:: erbsland::time::Date
    :members:
.. doxygenclass:: erbsland::time::DateTime
    :members:
.. doxygenstruct:: erbsland::time::DateTimeParts
    :members:
.. doxygenenum:: erbsland::time::DateTimePrecision
.. doxygenclass:: erbsland::time::Day
    :members:
.. doxygenclass:: erbsland::time::DayOfWeek
    :members:
.. doxygenenum:: erbsland::time::DayOfWeekFormat
.. doxygenclass:: erbsland::time::DayOfYear
    :members:
.. doxygenclass:: erbsland::time::Duration
    :members:
.. doxygenenum:: erbsland::time::DurationPart
.. doxygenclass:: erbsland::time::ElapsedTimer
    :members:
.. doxygenclass:: erbsland::time::Hour
    :members:
.. doxygenenum:: erbsland::time::IsoTimeFormat

.. doxygentypedef:: erbsland::time::IsoTimeFormatFlags
.. doxygenfunction:: erbsland::time::literals::operator""_ns(const unsigned long long value) -> Nanoseconds

.. doxygenfunction:: erbsland::time::literals::operator""_us(const unsigned long long value) -> Microseconds

.. doxygenfunction:: erbsland::time::literals::operator""_ms(const unsigned long long value) -> Milliseconds

.. doxygenfunction:: erbsland::time::literals::operator""_s(const unsigned long long value) -> Seconds

.. doxygenfunction:: erbsland::time::literals::operator""_m(const unsigned long long value) -> Minutes

.. doxygenfunction:: erbsland::time::literals::operator""_h(const unsigned long long value) -> Hours
.. doxygenclass:: erbsland::time::Minute
    :members:
.. doxygenclass:: erbsland::time::Month
    :members:
.. doxygenclass:: erbsland::time::Second
    :members:
.. doxygenclass:: erbsland::time::Time
    :members:
.. doxygentypedef:: erbsland::time::Nanoseconds

.. doxygentypedef:: erbsland::time::Microseconds

.. doxygentypedef:: erbsland::time::Milliseconds

.. doxygentypedef:: erbsland::time::Seconds

.. doxygentypedef:: erbsland::time::Minutes

.. doxygentypedef:: erbsland::time::Hours

.. doxygentypedef:: erbsland::time::Days

.. doxygentypedef:: erbsland::time::Weeks

.. doxygentypedef:: erbsland::time::Months

.. doxygentypedef:: erbsland::time::Years
.. doxygenclass:: erbsland::time::TimeDelta
    :members:
.. doxygenclass:: erbsland::time::TimeDeltaFormat
    :members:
.. doxygenenum:: erbsland::time::TimeDeltaUnit
.. doxygenenum:: erbsland::time::TimeEpoch
.. doxygenenum:: erbsland::time::TimeOccurrenceInFold
.. doxygenstruct:: erbsland::time::TimeParts
    :members:
.. doxygenclass:: erbsland::time::TimePoint
    :members:
.. doxygenstruct:: erbsland::time::SecondsUnitTag
    :members:

.. doxygenstruct:: erbsland::time::MonthsUnitTag
    :members:

.. doxygenstruct:: erbsland::time::YearsUnitTag
    :members:
.. doxygenclass:: erbsland::time::TimeWithZone
    :members:
.. doxygenstruct:: erbsland::time::TimeWrapResult
    :members:
.. doxygenclass:: erbsland::time::TimeZone
    :members:
.. doxygenclass:: erbsland::time::TimeZoneId
    :members:
.. doxygenclass:: erbsland::time::tz::TimeOffset
    :members:
.. doxygenclass:: erbsland::time::Year
    :members:
