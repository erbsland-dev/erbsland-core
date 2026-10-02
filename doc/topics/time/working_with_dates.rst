..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Dates; Working with Dates
    single: Time; Calendar Dates
    single: Date; Validation and Arithmetic

******************
Working with Dates
******************

A calendar date identifies a day without choosing a clock time or a time zone.
:cpp:class:`Date <erbsland::time::Date>` represents that value as a year, a month, and a day in the Gregorian calendar.
It lets you compare dates and move through the calendar while accounting for different month lengths and leap years.

This page explains how to construct and validate dates, access their calendar fields, and convert them for display or
storage.
You will also learn how date calculations handle month ends and the limits of the supported calendar.

A Calendar Date on Its Own
==========================

``Date`` uses the proleptic Gregorian calendar: the familiar Gregorian leap-year rules apply throughout its supported
range, including dates before the calendar was historically adopted.
Years run from ``0`` through ``9999``, so the first date is ``0000-01-01`` and the last is ``9999-12-31``.
There are no negative years, and the library does not model a country's historical change of calendar.

A leap year has a February 29: years divisible by four are leap years, except century years unless they are also
divisible by 400. For example, 2000 is a leap year and 2100 is not; year 0 follows the same rule.

The Core epoch is ``0000-01-01``.
A day count of zero therefore identifies that date, rather than the Unix epoch of ``1970-01-01``.
The last date has day count ``3,652,424``.
Keeping this epoch in mind matters when you exchange numeric dates with another library or file format.

A date does not identify an instant.
It identifies a calendar day without specifying a time within that day or the zone in which it is interpreted.
When you need a clock reading as well, combine it with :doc:`working_with_times` or choose
:cpp:class:`DateTime <erbsland::time::DateTime>` for a dated instant with zone information.

Constructing a Date
===================

Named Parts Make Construction Readable
--------------------------------------

The constructor takes :cpp:class:`Year <erbsland::time::Year>`, :cpp:class:`Month <erbsland::time::Month>`, and
:cpp:class:`Day <erbsland::time::Day>` values.
Because the fields have different types, you can write them in year-month-day, day-month-year, or year-day-month order
without confusing a month with a day.
Ordinary copying and moving preserve the same date; default construction produces an **invalid date**.
That default is useful when a field has not yet been supplied, but it does not capture today's date.
For the date of a current instant, obtain a ``DateTime`` and select its UTC or displayed date according to your task.

:cpp:func:`fromParts() <erbsland::time::Date::fromParts>` is convenient when only part of the date is specified.
Its month defaults to January and its day to the first, so supplying just a year selects January 1. The
:cpp:func:`fromPartsOrThrow() <erbsland::time::Date::fromPartsOrThrow>` variant rejects an impossible combination with
:cpp:class:`OutOfRangeError <erbsland::err::OutOfRangeError>`.

The distinction between a part's range and a complete date is worth understanding before using these constructors.
A ``Month`` clamps its input to ``1..12``; a ``Day`` clamps to ``1..31``; a ``Year`` clamps to ``0..9999``.
After that, ``Date`` checks whether the resulting day exists in the chosen month and year.
Thus month 13 becomes December before construction, while February 31 produces an invalid date.

.. erbsland-demo::
    :source: time/Date/CreateTyped.cpp
    :exec: time/date --demo CreateTyped
    :source-sha256: 38277d5900fb1b0273bb3bfdb62c1a24eab67c84adc36c88287fe574d48ca4aa

.. code-block:: cpp

    /// Build observation dates from named calendar parts.
    /// @notest{Compiled and executed documentation demo.}
    void createTyped() {
        const auto observation = "Observation de la Lune"_el;
        const auto year = el::Year{2028};
        const auto month = el::Month::february();
        const auto day = el::Day{29};

        // Named parts make all three constructor orders unambiguous.
        const auto date = el::Date{year, month, day};
        const auto dayFirst = el::Date{day, month, year};
        const auto yearDayMonth = el::Date{year, day, month};
        const auto startOfYear = el::Date::fromParts(year);
        const auto startOfMonth = el::Date::fromParts(year, month);
        const auto checked = el::Date::fromPartsOrThrow(year, month, day);
        el::io::printLine(el::StringFormat{"{}: {}"_el}.build(observation, date));
        el::io::printLine(
            el::StringFormat{"Constructor orders agree: {}"_el}.build(date == dayFirst && date == yearDayMonth));
        el::io::printLine(
            el::StringFormat{"Year start: {}; month start: {}; checked: {}"_el}.build(startOfYear, startOfMonth, checked));

        // Typed parts clamp their input before the date checks the combination.
        const auto clampedMonth = el::Date{year, el::Month{13}, el::Day{1}};
        const auto impossible = el::Date{year, month, el::Day{31}};
        el::io::printLine(
            el::StringFormat{"Clamped month: {}; February 31 valid: {}"_el}.build(clampedMonth, impossible.isValid()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Observation de la Lune: 2028-02-29
    Constructor orders agree: true
    Year start: 2028-01-01; month start: 2028-02-01; checked: 2028-02-29
    Clamped month: 2028-12-01; February 31 valid: false

.. erbsland-demo-end::


Checking Supplied Fields
------------------------

When your input consists of raw integers, you usually want to reject an out-of-range field instead of silently clamping
it.
:cpp:func:`fromYearMonthDay() <erbsland::time::Date::fromYearMonthDay>` checks the raw values and the calendar
combination together, returning an invalid date if either check fails.
:cpp:func:`fromYearMonthDayOrThrow() <erbsland::time::Date::fromYearMonthDayOrThrow>` performs the same validation
and throws ``OutOfRangeError`` on failure.
Choose the returning form when a missing or rejected value is part of ordinary input handling, and the throwing form
when your caller needs a clear failure path.

The typed ``fromPartsOrThrow()`` factory checks the combination of already constructed parts.
It cannot recover a raw value that a part has already clamped.
For example, passing ``Month{13}`` to this factory still passes December.

:cpp:func:`fromDaysSinceEpoch() <erbsland::time::Date::fromDaysSinceEpoch>` restores a date from a Core day count.
It accepts ``0..3,652,424``; any count outside that range produces an invalid date, rather than a saturated one.
For calendar boundaries, :cpp:func:`firstDay() <erbsland::time::Date::firstDay>` and
:cpp:func:`lastDay() <erbsland::time::Date::lastDay>` accept a year, or a year and month.
They save you from calculating the final day of February yourself.

:cpp:func:`epoch() <erbsland::time::Date::epoch>` and :cpp:func:`first() <erbsland::time::Date::first>` both return
``0000-01-01``; :cpp:func:`last() <erbsland::time::Date::last>` returns ``9999-12-31``.
These are valid dates, distinct from a default-constructed invalid value.

.. erbsland-demo::
    :source: time/Date/CreateChecked.cpp
    :exec: time/date --demo CreateChecked
    :source-sha256: 7ab427b4760b37ff9580d98fe7c8e2344bc3b2455c674c91524ca197f28a3f03

.. code-block:: cpp

    /// Validate supplied dates and choose calendar boundaries.
    /// @notest{Compiled and executed documentation demo.}
    void createChecked() {
        // Validate raw fields before they become clamped calendar parts.
        const auto supplied = el::Date::fromYearMonthDay(2028, 2, 29);
        const auto rejected = el::Date::fromYearMonthDay(2028, 13, 1);
        el::io::printLine(el::StringFormat{"Supplied date: {}; month 13 valid: {}"_el}.build(supplied, rejected.isValid()));
        try {
            const auto invalid = el::Date::fromYearMonthDayOrThrow(2027, 2, 29);
            el::io::printLine(el::StringFormat{"Accepted: {}"_el}.build(invalid));
        } catch (const el::OutOfRangeError &) {
            el::io::printLine("Rejected February 29 in a common year."_el);
        }
        try {
            const auto invalid = el::Date::fromPartsOrThrow(el::Year{2028}, el::Month::february(), el::Day{31});
            el::io::printLine(el::StringFormat{"Accepted: {}"_el}.build(invalid));
        } catch (const el::OutOfRangeError &) {
            el::io::printLine("Rejected February 31 from typed parts."_el);
        }

        const auto restored = el::Date::fromDaysSinceEpoch(supplied.toDaysSinceEpoch());
        const auto beforeEpoch = el::Date::fromDaysSinceEpoch(el::Days{-1});
        el::io::printLine(
            el::StringFormat{"Restored: {}; negative day count valid: {}"_el}.build(restored, beforeEpoch.isValid()));
        el::io::printLine(
            el::StringFormat{"Epoch: {}; first: {}; last: {}"_el}.build(
                el::Date::epoch(), el::Date::first(), el::Date::last()));
        const auto year = el::Year{2028};
        const auto month = el::Month::february();
        el::io::printLine(
            el::StringFormat{"Year boundaries: {} to {}"_el}.build(el::Date::firstDay(year), el::Date::lastDay(year)));
        el::io::printLine(
            el::StringFormat{"Month boundaries: {} to {}"_el}.build(
                el::Date::firstDay(year, month), el::Date::lastDay(year, month)));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Supplied date: 2028-02-29; month 13 valid: false
    Rejected February 29 in a common year.
    Rejected February 31 from typed parts.
    Restored: 2028-02-29; negative day count valid: false
    Epoch: 0000-01-01; first: 0000-01-01; last: 9999-12-31
    Year boundaries: 2028-01-01 to 2028-12-31
    Month boundaries: 2028-02-01 to 2028-02-29

.. erbsland-demo-end::


Checking and Inspecting a Date
==============================

:cpp:func:`isValid() <erbsland::time::Date::isValid>` tells you whether a date represents an existing calendar day.
For example, February 29 is valid in 2028 and invalid in 2027.
:cpp:func:`exists() <erbsland::time::Date::exists>` checks a combination of typed parts directly when you do not
need to retain the date.

Dates support equality and chronological ordering, so you can sort dates or check whether a new date is later than an
existing one.
All invalid dates compare equal to one another and sort before every valid date, including the epoch.
That ordering makes sorting well-defined, but it does not make an invalid value an earlier calendar day.
Check validity before using a value in a calculation that requires an existing calendar day.

:cpp:func:`isFirst() <erbsland::time::Date::isFirst>` and :cpp:func:`isLast() <erbsland::time::Date::isLast>` identify
the two supported boundaries.
They let you detect the supported boundaries during navigation or after arithmetic that may have saturated.
A boundary result alone does not prove saturation: a calculation can reach the first or last date exactly.
The saturation query in the calculation section distinguishes these cases.

Accessing Calendar Fields
-------------------------

The :cpp:func:`year() <erbsland::time::Date::year>`, :cpp:func:`month() <erbsland::time::Date::month>`, and
:cpp:func:`day() <erbsland::time::Date::day>` accessors return typed parts.
:cpp:func:`dayOfYear() <erbsland::time::Date::dayOfYear>` is one-based, ranging from 1 to 365 or 366.
:cpp:func:`dayOfWeek() <erbsland::time::Date::dayOfWeek>` returns a
:cpp:class:`DayOfWeek <erbsland::time::DayOfWeek>` with Monday numbered zero and Sunday numbered six.
Its named values and text output are often clearer than passing the numeric weekday around.

When you need year, month, and day together, :cpp:func:`parts() <erbsland::time::Date::parts>` returns a
:cpp:struct:`DateParts <erbsland::time::DateParts>` with named fields.
One extraction avoids repeating the calendar conversion performed by the separate date-field accessors.
The parts also make reconstruction straightforward: pass ``parts.year``, ``parts.month``, and ``parts.day`` to the
constructor.

.. erbsland-demo::
    :source: time/Date/Inspect.cpp
    :exec: time/date --demo Inspect
    :source-sha256: 099ffd0374106bcf7129f2b6e81e3371dba6bb50240a984e916623c21a688743

.. code-block:: cpp

    /// Compare dates and extract fields for a calendar view.
    /// @notest{Compiled and executed documentation demo.}
    void inspect() {
        const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
        const auto invalid = el::Date{};
        el::io::printLine(
            el::StringFormat{"Valid: {}; invalid sorts first: {}; invalid dates equal: {}"_el}.build(
                date.isValid(), invalid < el::Date::first(), invalid == el::Date{}));
        el::io::printLine(el::StringFormat{"Next observation is later: {}"_el}.build(date < date.next()));
        el::io::printLine(
            el::StringFormat{"At first boundary: {}; at last boundary: {}"_el}.build(
                el::Date::first().isFirst(), el::Date::last().isLast()));
        el::io::printLine(
            el::StringFormat{"Date exists: {}"_el}.build(
                el::Date::exists(el::Year{2028}, el::Month::february(), el::Day{29})));

        // An invalid date's fallback fields cannot replace its validity check.
        const auto fallback = invalid.parts();
        el::io::printLine(
            el::StringFormat{"Invalid fallback: {}-{}-{}; still invalid: {}"_el}.build(
                fallback.year.toValue(), fallback.month.toValue(), fallback.day.toValue(), !invalid.isValid()));

        // Extract related fields together when preparing a calendar view.
        const auto parts = date.parts();
        el::io::printLine(
            el::StringFormat{"Year: {}; month: {}; day: {}"_el}.build(
                parts.year.toValue(), parts.month.toValue(), parts.day.toValue()));
        el::io::printLine(
            el::StringFormat{"Individual fields: {} / {} / {}"_el}.build(
                date.year().toValue(), date.month().toValue(), date.day().toValue()));
        el::io::printLine(
            el::StringFormat{"Day of year: {}; weekday: {} (index {})"_el}.build(
                date.dayOfYear().toValue(), date.dayOfWeek().toString(), date.dayOfWeek().toValue()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Valid: true; invalid sorts first: true; invalid dates equal: true
    Next observation is later: true
    At first boundary: true; at last boundary: true
    Date exists: true
    Invalid fallback: 0-1-1; still invalid: true
    Year: 2028; month: 2; day: 29
    Individual fields: 2028 / 2 / 29
    Day of year: 60; weekday: Tuesday (index 1)

.. erbsland-demo-end::


For an invalid date, the accessors return fallback fields: year 0, January, day 1, day-of-year 1, and Saturday.
``parts()`` likewise returns the epoch's year, month, and day.
Those fields are not evidence of validity, and reconstructing them would turn an invalid value into a valid epoch date.
Keep the original validity check alongside any extraction from optional input.

Converting a Date for Display or Storage
========================================

:cpp:func:`toString() <erbsland::time::Date::toString>` and the default
:cpp:func:`toIsoString() <erbsland::time::Date::toIsoString>` both produce ``YYYY-MM-DD``.
The zero padding makes the result consistent for display and sorting as text.
Both return an empty string for an invalid date.

For numeric storage, :cpp:func:`toDaysSinceEpoch() <erbsland::time::Date::toDaysSinceEpoch>` returns a typed
:cpp:type:`Days <erbsland::time::Days>` amount, using the Core epoch explained above.
Invalid dates produce ``Days{-1}``.
Retain the typed value while working inside the library, and extract its integer only when an external format requires
one.

You can ask ``toIsoString()`` for a compact date without separators or reduce its
:cpp:enum:`DateTimePrecision <erbsland::time::DateTimePrecision>` to a year or month.
Reduced output is a presentation choice: ``2028-02`` no longer communicates a particular day even though the original
object still stores one.
See :ref:`time-iso-output` for the formatting types and the shared ISO output controls.

.. erbsland-demo::
    :source: time/Date/Convert.cpp
    :exec: time/date --demo Convert
    :source-sha256: 077e56c900a599b4060b1cb0960cbb10a6ca379c1f2727245aca10ebddb465fc

.. code-block:: cpp

    /// Convert observation dates to text and epoch day counts.
    /// @notest{Compiled and executed documentation demo.}
    void convert() {
        const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
        el::io::printLine(el::StringFormat{"Display: {}; default ISO: {}"_el}.build(date.toString(), date.toIsoString()));
        el::io::printLine(el::StringFormat{"Days since Core epoch: {}"_el}.build(date.toDaysSinceEpoch().toRawValue()));
        el::io::printLine(el::StringFormat{"Compact ISO: {}"_el}.build(date.toIsoString(el::IsoTimeFormatFlags{})));
        el::io::printLine(
            el::StringFormat{"Month precision: {}"_el}.build(
                date.toIsoString(el::cDefaultDateFormat, el::DateTimePrecision::Month)));
        const auto invalid = el::Date{};
        el::io::printLine(
            el::StringFormat{"Invalid display: '{}'; invalid day count: {}"_el}.build(
                invalid.toString(), invalid.toDaysSinceEpoch().toRawValue()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Display: 2028-02-29; default ISO: 2028-02-29
    Days since Core epoch: 740771
    Compact ISO: 20280229
    Month precision: 2028-02
    Invalid display: ''; invalid day count: -1

.. erbsland-demo-end::


Moving Through the Calendar
===========================

You can move a date forward or backward by a number of days, months, or years.
:cpp:func:`added() <erbsland::time::Date::added>` accepts ``Days``,
:cpp:type:`Months <erbsland::time::Months>`, or :cpp:type:`Years <erbsland::time::Years>` and returns a new date.
:cpp:func:`add() <erbsland::time::Date::add>` changes the existing date instead.
Negative amounts move backward, so subtraction uses the same methods with a negative amount.

A day count moves by that many calendar days.
Months and years move the calendar fields and then clamp a day that does not exist in the destination month.
January 31, 2028 plus one month becomes February 29; February 29 plus one year becomes February 28, 2029. This is a
successful calendar operation, including for
:cpp:func:`addedOrThrow() <erbsland::time::Date::addedOrThrow>` and
:cpp:func:`addOrThrow() <erbsland::time::Date::addOrThrow>`.
The throwing forms reject leaving the supported range, rather than ordinary month-end adjustment.

Because a shorter month can lose the original day number, adding a month and then subtracting a month need not restore
the original date.
In the calculation demo, adding one month to January 31 and then subtracting it returns January 29. For a recurrence
that must always select the month's final day, calculate that day with ``lastDay()`` for each occurrence instead of
carrying forward an already adjusted day.

:cpp:func:`next() <erbsland::time::Date::next>` and :cpp:func:`previous() <erbsland::time::Date::previous>` move one
day when called without an argument.
Their weekday overloads find the next or previous requested weekday, always excluding the starting date.
If today is Friday, asking for the next Friday advances by seven days.

:cpp:func:`daysTo() <erbsland::time::Date::daysTo>` measures the signed calendar-day distance to another date.
``start.daysTo(finish)`` is positive when ``finish`` is later and negative when it is earlier.
This is a date distance; measuring elapsed time between zoned instants is a separate task.

.. erbsland-demo::
    :source: time/Date/Calculate.cpp
    :exec: time/date --demo Calculate
    :source-sha256: 03eaad91db64d1fac3f9532aeb89b6f53c6adc55161242ab648cfa09739fbc97

.. code-block:: cpp

    /// Move observation dates by days, months, or years.
    /// @notest{Compiled and executed documentation demo.}
    void calculate() {
        const auto observation = el::Date::fromYearMonthDayOrThrow(2028, 1, 31);
        const auto nextMonth = observation.added(el::Months{1});
        el::io::printLine(
            el::StringFormat{"One month later: {}; one month back: {}"_el}.build(
                nextMonth, nextMonth.added(el::Months{-1})));
        el::io::printLine(el::StringFormat{"Next year from leap day: {}"_el}.build(nextMonth.addedOrThrow(el::Years{1})));
        el::io::printLine(el::StringFormat{"Seven days later: {}"_el}.build(observation.addedOrThrow(el::Days{7})));

        auto revised = observation;
        revised.add(el::Months{1});
        revised.addOrThrow(el::Days{2});
        revised.addOrThrow(el::Years{1});
        el::io::printLine(el::StringFormat{"Revised date: {}"_el}.build(revised));
        el::io::printLine(
            el::StringFormat{"Previous day: {}; next day: {}"_el}.build(observation.previous(), observation.next()));
        el::io::printLine(
            el::StringFormat{"Next Friday: {}; previous Friday: {}"_el}.build(
                observation.next(el::DayOfWeek::friday()), observation.previous(el::DayOfWeek::friday())));
        el::io::printLine(
            el::StringFormat{"Days forward: {}; days backward: {}"_el}.build(
                observation.daysTo(nextMonth).toRawValue(), nextMonth.daysTo(observation).toRawValue()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    One month later: 2028-02-29; one month back: 2028-01-29
    Next year from leap day: 2029-02-28
    Seven days later: 2028-02-07
    Revised date: 2029-03-02
    Previous day: 2028-01-30; next day: 2028-02-01
    Next Friday: 2028-02-04; previous Friday: 2028-01-28
    Days forward: 29; days backward: -29

.. erbsland-demo-end::


Choosing What Happens at the Boundaries
---------------------------------------

Ordinary ``added()`` and ``add()`` operations **saturate**: a result before ``0000-01-01`` becomes the first date, and a
result after ``9999-12-31`` becomes the last date.
This keeps a calendar value in range, but it can hide that a requested change was too large.
:cpp:func:`wouldAddSaturate() <erbsland::time::Date::wouldAddSaturate>` checks the proposed day, month, or year
change without modifying the date.
It returns false if the change reaches a boundary exactly or only adjusts the day within a shorter month.

The ``...OrThrow()`` variants throw :cpp:class:`OverflowError <erbsland::err::OverflowError>` when the result would
leave the range.
An in-place throwing operation leaves the date unchanged on that failure.
This lets your caller handle an unsupported date change as an error instead of accepting a boundary date.

There is a deliberate difference between arithmetic and one-day navigation at the boundaries.
``Date::last().added(Days{1})`` stays at the last date, whereas ``Date::last().next()`` is invalid.
Similarly, ``Date::first().previous()`` is invalid.
That lets a day-by-day traversal detect when it has finished.
The weekday overloads instead use saturating arithmetic; near a boundary, their result may therefore be a boundary date
whose weekday is not the requested one.

.. erbsland-demo::
    :source: time/Date/Boundaries.cpp
    :exec: time/date --demo Boundaries
    :source-sha256: e0691d9d036ceec8a296b6bbac94645bfcc862fd9026ef301d90dd821fd46fcb

.. code-block:: cpp

    /// Choose saturation or explicit failure at date boundaries.
    /// @notest{Compiled and executed documentation demo.}
    void boundaries() {
        auto end = el::Date::last();
        el::io::printLine(el::StringFormat{"Adding a day would saturate: {}"_el}.build(end.wouldAddSaturate(el::Days{1})));
        end.add(el::Days{1});
        el::io::printLine(el::StringFormat{"Saturated result: {}"_el}.build(end));
        try {
            end.addOrThrow(el::Months{1});
        } catch (const el::OverflowError &) {
            el::io::printLine(
                el::StringFormat{"Rejected a month beyond the supported range; date remains {}."_el}.build(end));
        }
        el::io::printLine(el::StringFormat{"First minus one year: {}"_el}.build(el::Date::first().added(el::Years{-1})));
        el::io::printLine(
            el::StringFormat{"Next beyond last valid: {}; previous before first valid: {}"_el}.build(
                end.next().isValid(), el::Date::first().previous().isValid()));
        const auto invalid = el::Date{};
        el::io::printLine(
            el::StringFormat{"Invalid remains invalid: {}; invalid saturation query: {}; invalid distance: {}"_el}.build(
                invalid.addedOrThrow(el::Days{1}).isValid(),
                invalid.wouldAddSaturate(el::Days{1}),
                invalid.daysTo(end).toRawValue()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Adding a day would saturate: true
    Saturated result: 9999-12-31
    Rejected a month beyond the supported range; date remains 9999-12-31.
    First minus one year: 0000-01-01
    Next beyond last valid: false; previous before first valid: false
    Invalid remains invalid: false; invalid saturation query: false; invalid distance: 0

.. erbsland-demo-end::


Invalid dates remain invalid through arithmetic and navigation, including the throwing arithmetic forms.
``wouldAddSaturate()`` returns false for an invalid date, and ``daysTo()`` returns zero if either date is invalid.
A zero distance therefore needs a validity check before you interpret it as “the same day.”

Building a Larger Time Model
============================

:doc:`working_with_times` adds a clock reading and shows how to carry crossed days back into a date.
The :doc:`overview` helps you choose the remaining types: ``Duration`` and ``TimeDelta`` for fixed intervals,
:cpp:class:`CalendarDelta <erbsland::time::CalendarDelta>` for combined calendar changes, and ``DateTime`` or
:cpp:class:`Timestamp <erbsland::time::Timestamp>` when an event must identify an instant.
For the exact signatures and individual contracts, see :doc:`../../reference/time/date_and_time`.
