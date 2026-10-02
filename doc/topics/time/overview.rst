..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Time; Choosing a Type

***********************
Dates and Time Overview
***********************

A calendar date, a time interval, and a timestamp describe different kinds of values.
The time domain gives each of them its own type so that your code can retain the meaning of a value as it moves between
calculations, display, and storage.

Choosing a Type
===============

The types are available in ``erbsland::time`` and through the flattened ``el`` namespace used in the demos.
Individual headers live under ``<erbsland/time/>``; ``<erbsland/time/all.hpp>`` includes the domain.

.. list-table::
    :header-rows: 1
    :widths: 25 40 35

    * - Value
      - What it expresses
      - Typical task
    * - Calendar and time parts
      - A position such as ``Day{15}`` or ``Hour{9}``
      - Constructing or inspecting fields
    * - Typed time amounts
      - A quantity such as ``Days{15}`` or ``Hours{9}``
      - Making a unit explicit in an API
    * - ``Date`` and ``Time``
      - A calendar day or a reading within a clock day
      - Calendars and daily timetables
    * - ``Duration``
      - A signed fixed interval in whole seconds
      - Whole-second delays and long intervals
    * - ``TimeDelta``
      - A signed fixed interval in nanoseconds
      - Precise latency and interval calculations
    * - ``CalendarDelta``
      - Independent changes from nanoseconds through years
      - Combining fixed and calendar changes
    * - ``TimePoint``
      - A point on a monotonic clock with an unspecified epoch
      - Checkpoints and deadlines
    * - ``Timestamp``
      - A civil instant without display-zone information
      - Recording and exchanging event instants
    * - ``DateTime``
      - A UTC instant with a display zone or offset
      - Viewing an instant in a chosen zone
    * - ``TimeZone``
      - UTC, a fixed offset, or named rules, with optional local origin
      - Interpreting local input and choosing a display zone
    * - ``TimeWithZone``
      - An intended daily clock reading with its zone
      - Resolving local appointments on a chosen date

Constructing Dates and Times
============================

:doc:`working_with_dates` starts with calendar dates, including typed construction, validation of supplied fields,
calendar extraction, and text or day-count conversion.
It also explains why default construction is invalid and how dates compare.

:doc:`working_with_times` covers daily clock readings, fractional seconds, and conversion to counts or intervals
since midnight.
It also explains how to retain the date change when a calculation wraps across midnight.

:doc:`working_with_timestamps` explains recording and exchanging UTC instants, including canonical text,
portable bytes, signed epoch counts, and precise distances.
:doc:`working_with_datetime` covers resolving local input, choosing a display zone, comparing instants,
and applying elapsed or calendar changes.
:doc:`keeping_wall_clock_time_with_zone` explains retaining an undated local reading, resolving each occurrence,
choosing fold occurrences, and checking skipped readings.

Choosing and Resolving Time Zones
=================================

:doc:`working_with_time_zones` covers UTC, fixed offsets, named zones and aliases, and the cached system-local setting.
It explains interpreting local input versus converting an existing instant, daylight-saving gaps and folds, resolved
offsets and labels, transient identifiers, and the bundled rule version.
For recurring local readings, :doc:`keeping_wall_clock_time_with_zone` separates the stored wall-clock intention from
the UTC instant selected on each date.

Expressing Amounts and Intervals
================================

:doc:`working_with_calendar_parts` explains typed positions, contextual validation, calendar navigation, weekday
selection, ordinal conversion, and named result structures.
A calendar part names a position, while an amount names a quantity: ``Day{15}`` selects a day of the month, and
``Days{15}`` represents fifteen days.
:doc:`working_with_time_amounts` explains the units from nanoseconds through years, signed quantities, compatible
conversions, precision loss, and saturating arithmetic.
:doc:`writing_time_literals` shows how the suffixes ``_ns``, ``_us``, ``_ms``, ``_s``, ``_m``, and ``_h`` make common
amounts readable and how to choose a shared representation for an expression; ``_m`` means minutes.

:doc:`working_with_durations` covers whole-second fixed spans, including construction from typed amounts or chrono
values, signed arithmetic, splitting totals into components, and checked conversion to nanosecond precision.
:doc:`working_with_time_deltas` covers nanosecond intervals, typed and checked construction, signed calculations,
integer ratios, saturation, and conversion of precise totals.
Its ``TimeDelta`` preserves nanoseconds over a smaller range.
Choose the required precision before converting: dropping fractional seconds can be intentional for a timeout, but
misleading for a measurement.
Fixed days and weeks have lengths of 86,400 seconds and seven such days; months and years need calendar context.

Calendar Calculations
=====================

The calculations in :doc:`working_with_dates` apply individual day, month, or year amounts to a date.
They explain month-end adjustment and the choice between saturation and an explicit overflow error.

:doc:`understanding_calendar_changes` explains composing independent components, applying them in order, month-end
clamping, checked conversion, and intermediate overflow.
It also distinguishes UTC calendar arithmetic from preserving an intended local wall-clock time across zone transitions.

Customizing ISO Date and Time Output
====================================

:doc:`customizing_iso_output` explains every ``IsoTimeFormat`` flag and ``DateTimePrecision`` level with matching
examples for dates, standalone times, and combined readings.
It covers separators, numeric UTC, second-level offsets, fixed-width fractions, and truncating displayed precision.
:doc:`working_with_timestamps` uses a fixed canonical UTC format for precise exchange.

For intervals, :doc:`customizing_time_interval_output` explains format presets, unit names, separators, every
smallest-unit choice, and the interaction of fractional visibility with digit limits.
It also covers signed calendar-component display and ELCL serialization for ``TimeDelta`` and ``CalendarDelta``.
Formatting changes the text you present, rather than the value stored in the interval.

Measuring Elapsed Time
======================

:cpp:class:`ElapsedTimer <erbsland::time::ElapsedTimer>` measures an operation from its starting point and returns
a ``TimeDelta``.
:cpp:class:`TimePoint <erbsland::time::TimePoint>` exposes monotonic checkpoints and deadlines when you need to
retain or compare explicit points.
Both use a monotonic clock, so changing the system's civil clock does not change the meaning of your elapsed
measurement.
:doc:`measuring_elapsed_time` covers cumulative readings, restarting between phases, timer copies, and cooperative
processing budgets.
:doc:`working_with_monotonic_time_points` covers explicit checkpoints, signed distances, shared deadlines, clock
boundaries, and steady-clock interoperability.

Running the Topic Demos
=======================

The examples on these pages use the common demo framework.
Their sources live in ``demos/time/Date``, ``demos/time/Time``, ``demos/time/Timestamp``, ``demos/time/DateTime``,
``demos/time/IsoTimeOutput``, ``demos/time/Duration``, ``demos/time/TimeLiterals``, ``demos/time/TimeAmounts``,
``demos/time/TimeDelta``, ``demos/time/TimeDeltaOutput``, ``demos/time/CalendarDelta``, ``demos/time/CalendarParts``,
``demos/time/ElapsedTimer``, ``demos/time/TimePoint``, ``demos/time/TimeWithZone``, and ``demos/time/TimeZone``.
The documentation utility synchronizes both source and output into each topic.
From the repository root, build and select an individual example:

.. code-block:: console

    cmake -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug .
    cmake --build cmake-build-debug --target date time timestamp date_time iso_time_output duration
    cmake --build cmake-build-debug --target time_literals time_amounts time_delta time_delta_output
    cmake --build cmake-build-debug --target calendar_delta calendar_parts elapsed_timer time_point
    cmake --build cmake-build-debug --target time_with_zone time_zone
    cmake-build-debug/demo-apps/time/date --demo Calculate
    cmake-build-debug/demo-apps/time/time --demo Wrap
    cmake-build-debug/demo-apps/time/timestamp --demo Ticks
    cmake-build-debug/demo-apps/time/date_time --demo Zones
    cmake-build-debug/demo-apps/time/iso_time_output --demo Precision
    cmake-build-debug/demo-apps/time/duration --demo Parts
    cmake-build-debug/demo-apps/time/time_literals --demo Expressions
    cmake-build-debug/demo-apps/time/time_amounts --demo Convert
    cmake-build-debug/demo-apps/time/time_delta --demo Calculate
    cmake-build-debug/demo-apps/time/time_delta_output --demo Precision
    cmake-build-debug/demo-apps/time/calendar_delta --demo Apply
    cmake-build-debug/demo-apps/time/calendar_parts --demo Navigate
    cmake-build-debug/demo-apps/time/elapsed_timer --demo Phases
    cmake-build-debug/demo-apps/time/time_point --demo Deadline
    cmake-build-debug/demo-apps/time/time_with_zone --demo Recurrence
    cmake-build-debug/demo-apps/time/time_zone --demo Transitions

Each executable also accepts ``--help`` and runs all its examples when no ``--demo`` is supplied.
