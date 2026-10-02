..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Date and Time; Reference
    single: Date and Time Types
    single: Duration and Time Amounts
    single: ISO Date and Time; Formatting

*************
Date and Time
*************

Date and Time Types
===================

Introduction
------------

The ``time`` namespace provides types for working with civil dates, wall-clock times, and time spans.
Calendar and clock parts are bounded labels; the aggregate result structures name related fields.
Part/amount conversion uses offsets from the part's minimum label, and numeric range checks do not validate calendar
combinations.
See :doc:`../../topics/time/working_with_calendar_parts` for construction, numbering, contextual validation, navigation,
weekday selection, ordinal conversion, and field reconstruction.

Date
~~~~

:cpp:class:`Date <erbsland::time::Date>` represents a proleptic Gregorian calendar date in
``0000-01-01..9999-12-31``, with Core epoch ``0000-01-01``.
Default construction is invalid; invalid dates compare equal and sort before valid dates.
Construction validates calendar combinations, and day, month, and year arithmetic offers saturating and throwing forms.
See :doc:`../../topics/time/working_with_dates` for construction choices, validation, field extraction, conversions,
month-end adjustment, navigation, and boundary handling.

Date Time
~~~~~~~~~

:cpp:class:`DateTime <erbsland::time::DateTime>` retains a UTC instant plus resolved display-offset or named-zone metadata.
It shares the date range ``0000-01-01..9999-12-31`` and retains nanosecond fractions.
Ordering compares UTC instants; equality also compares display metadata.
Tick conversion accepts nonnegative seconds, milliseconds, microseconds, or nanoseconds relative to ``TimeEpoch`` and
truncates smaller fractions; split seconds/fractions retain full precision.
Arithmetic applies ``Duration`` or ``CalendarDelta`` in UTC, with saturating, throwing, and overflow-test variants.
The distance APIs use whole-second epoch readings; convert to ``Timestamp`` for precise checked distances.
See :doc:`../../topics/time/working_with_datetime` for construction, field extraction, zone resolution, conversion, and
calculations, including folds, gaps, and local recurrences.

:cpp:class:`TimeWithZone <erbsland::time::TimeWithZone>` retains a daily wall-clock reading with its intended zone.
Default construction is midnight UTC; equality compares the stored reading and zone.
It identifies an instant only after ``DateTime`` resolves it with a local date and, when needed, a fold occurrence.
See :doc:`../../topics/time/keeping_wall_clock_time_with_zone` for construction, accessors, compact output, transition
handling, and local recurrences.

Time
~~~~

:cpp:class:`Time <erbsland::time::Time>` represents a daily clock reading in
``00:00:00..23:59:59.999999999``.
Default construction is midnight; there is no invalid sentinel, date, or zone.
:cpp:struct:`TimeParts <erbsland::time::TimeParts>` names its component fields.
Wrapping addition accepts ``Duration`` or ``TimeDelta`` and either returns a signed day carry or a
:cpp:struct:`TimeWrapResult <erbsland::time::TimeWrapResult>` containing the new time and carry.
See :doc:`../../topics/time/working_with_times` for construction, comparisons, precision-preserving conversions, and
calculations across midnight.

Time Zone
~~~~~~~~~

:cpp:class:`TimeZone <erbsland::time::TimeZone>` represents UTC, a normalized fixed offset, or a supported named IANA zone.
Name lookup returns ``std::optional`` or throws ``err::ParseError``; aliases resolve to the primary zone.
Component construction clamps each signed component; total-duration construction removes complete 24-hour rotations.
``staticOffset()`` returns zero for named zones; ``DateTime::timeOffset()`` supplies an instant's resolved shift.
Equality includes zone identity and local origin.

``local()`` caches the system's base zone for the process lifetime, falling back to local-marked UTC.
Named-zone offsets still resolve per date using the bundled IANA rules identified by ``databaseVersion()``.
Local origin is preserved by copies/arithmetic and replaced by the target marker on conversion.
Default local-origin display omits the zone; explicit ISO ``TimeShift`` output includes the resolved shift.

:cpp:enum:`TimeOccurrenceInFold <erbsland::time::TimeOccurrenceInFold>` selects the earlier (default ``First``) or later
(``Second``) UTC match for repeated local readings.
Gap construction follows the database's transition rule and can retain display metadata that differs from the selected
instant's refreshed zone metadata; ``isValid()`` does not detect skipped readings.

:cpp:class:`TimeZoneId <erbsland::time::TimeZoneId>` and ``tz::TimeOffset::abbreviationId()`` are transient database indexes.
:cpp:class:`tz::TimeOffset <erbsland::time::tz::TimeOffset>` holds offset seconds, zone/DST/abbreviation metadata,
and local origin; its constructors do not resolve named-zone rules.
See :doc:`../../topics/time/working_with_time_zones` for zone selection, lookup, conversion, transition resolution,
system-local behavior, persistence choices, detail values, and rule-version queries.

ISO Date and Time Output
========================

:cpp:type:`IsoTimeFormatFlags <erbsland::time::IsoTimeFormatFlags>` combines
:cpp:enum:`IsoTimeFormat <erbsland::time::IsoTimeFormat>` flags for ``Date``, ``Time``, and ``DateTime`` ISO output.
:cpp:enum:`DateTimePrecision <erbsland::time::DateTimePrecision>` controls the last emitted field or fractional digit.
Defaults use extended fields, whole seconds for time output, and a space without an offset for combined output.
See :doc:`../../topics/time/customizing_iso_output` for every flag, precision level, and option interaction.
``Timestamp`` uses its own fixed canonical UTC output; see :doc:`timestamp`.

Duration and Time Amounts
=========================

Introduction
------------

Duration and time span types store signed time intervals at different resolutions.

``<erbsland/time/TimeUnitTags.hpp>`` declares :cpp:struct:`SecondsUnitTag <erbsland::time::SecondsUnitTag>`,
:cpp:struct:`MonthsUnitTag <erbsland::time::MonthsUnitTag>`, and
:cpp:struct:`YearsUnitTag <erbsland::time::YearsUnitTag>` for the dimensions of time amount types.

Time amounts from :cpp:type:`Nanoseconds <erbsland::time::Nanoseconds>` through
:cpp:type:`Years <erbsland::time::Years>` are signed 64-bit ``IntegerAmount`` aliases.
Fixed units through weeks share a seconds dimension; months and years each have their own unit tag.
Compatible conversions truncate toward zero and offer saturating, overflow-test, and throwing forms.
See :doc:`../../topics/time/working_with_time_amounts` for unit selection, construction, conversion, arithmetic, and
calendar application.

The integer suffixes in ``<erbsland/time/Literals.hpp>`` produce typed amounts in ``erbsland::time::literals`` and
saturate oversized unsigned literal inputs to the signed maximum.
See :doc:`../../topics/time/writing_time_literals` for scope, suffix types, expressions, and precision limits.

:cpp:class:`Duration <erbsland::time::Duration>` stores a signed 64-bit span in whole seconds, with saturating arithmetic
and signed component extraction controlled by :cpp:enum:`DurationPart <erbsland::time::DurationPart>`.
Sub-second inputs truncate toward zero; conversion to ``TimeDelta`` offers saturating, overflow-test, and throwing
forms.
See :doc:`../../topics/time/working_with_durations` for construction, comparisons, both split-result structures, chrono
interoperability, and precision/range tradeoffs.

:cpp:class:`TimeDelta <erbsland::time::TimeDelta>` stores a signed nanosecond interval with saturating arithmetic.
Whole-unit conversions truncate toward zero; division by zero terminates.
See :doc:`../../topics/time/working_with_time_deltas` for construction, checked factories, ratios, endpoint behavior,
chrono interoperability, and exact versus approximate conversions.

:cpp:class:`CalendarDelta <erbsland::time::CalendarDelta>` stores nanoseconds through years as independent signed
components, with component equality and saturating component arithmetic.
Application to ``DateTime`` runs from nanoseconds through years in UTC and retains the display zone.
Fixed conversion requires zero month/year parts and representable components and intermediate sums;
``isValidTimeDelta()`` tests conversion success.
See :doc:`../../topics/time/understanding_calendar_changes` for composition, application order, month-end clamping,
local recurrences, boundary handling, and fixed conversion.

:cpp:class:`TimeDeltaFormat <erbsland::time::TimeDeltaFormat>` controls fixed and calendar interval presentation.
It provides short, long, and ELCL presets, custom separators, a smallest fixed unit, and truncating fractional output.
See :doc:`../../topics/time/customizing_time_interval_output` for every option, precision interactions, zero and sign
handling, calendar normalization, and ELCL serialization.

Measuring Elapsed Time
======================

:cpp:class:`ElapsedTimer <erbsland::time::ElapsedTimer>` starts a monotonic measurement on construction.
``elapsed()`` returns a cumulative ``TimeDelta``; ``restart()`` replaces the starting point and returns no interval.
Copies retain the starting point and can be restarted independently.
See :doc:`../../topics/time/measuring_elapsed_time` for operation timing, phase measurements, time budgets, and
interpretation of clock precision and measurement noise.

:cpp:class:`TimePoint <erbsland::time::TimePoint>` wraps a ``std::chrono::steady_clock::time_point``.
Default construction selects the clock epoch; ``now()`` captures a reading and ``inFuture()`` offsets one from now.
Subtraction returns signed intervals, comparison orders points, and ``toStdTimePoint()`` preserves the clock domain.
Point offsets and differences must stay within the underlying clock's representable range; point arithmetic does not
saturate.
See :doc:`../../topics/time/working_with_monotonic_time_points` for checkpoints, interval direction, deadlines,
clock-domain boundaries, and standard library interoperability.

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
