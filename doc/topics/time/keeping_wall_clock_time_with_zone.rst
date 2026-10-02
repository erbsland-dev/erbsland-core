..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: TimeWithZone; Daily Local Times
    single: Time; Local Recurrences
    single: Daylight Saving; Resolving Local Times

***************************************
Keeping a Wall-Clock Time with Its Zone
***************************************

A daily appointment often starts with a clock reading and a place, while its next date is still undecided.
You might need to retain “09:15 in Europe/Athens” and turn it into a concrete instant only when someone selects a date.
This page explains how to keep that intention in a ``TimeWithZone``, resolve it into a ``DateTime``, and handle the days
when the local clock skips or repeats a reading.

A Daily Reading Is Not Yet an Instant
=====================================

A plain :cpp:class:`Time <erbsland::time::Time>` describes a reading within a day.
It does not say which day or which zone that reading belongs to.
:cpp:class:`TimeWithZone <erbsland::time::TimeWithZone>` adds the intended zone, giving you a useful value for daily
schedules without requiring you to invent a date.
A :cpp:class:`DateTime <erbsland::time::DateTime>` goes further: it identifies a dated UTC instant and retains a zone
for display.

These distinctions matter when you keep a daily appointment for later use.
``09:15`` alone leaves the zone open to interpretation.
``09:15[Europe/Athens]`` retains the intended local reading, but still needs a date to select an instant.
Once you supply that date, ``DateTime`` can resolve the zone's rules and determine the corresponding UTC reading.

When an appointment should follow a particular place's local clock, retaining its named zone keeps that intention clear.
Athens has different offsets in winter and summer, so replacing its name with today's numeric offset would change the
meaning of later appointments.
A fixed offset is appropriate when the intention really is tied to that offset throughout the year.
:doc:`working_with_time_zones` explains how to select UTC, a fixed offset, a named zone, or the system-local zone.

Building and Inspecting the Value
=================================

Start with the clock reading you want to retain, then pair it with the zone in which that reading should occur.
Construction takes a ``Time`` and a ``TimeZone``.
The zone argument defaults to UTC, and default construction creates midnight UTC.
There is no invalid ``TimeWithZone`` sentinel: it holds a daily reading whose range follows ``Time``, together with an
available zone value.
Unknown zone names are handled when you construct the ``TimeZone``, before you create the zoned reading.

:cpp:func:`time() <erbsland::time::TimeWithZone::time>` returns the reading, and
:cpp:func:`timeZone() <erbsland::time::TimeWithZone::timeZone>` returns its zone.
The direct ``hour()``, ``minute()``, and ``second()`` accessors expose the corresponding typed clock parts.
``millisecondFraction()`` returns the millisecond portion of the fractional second, while ``nanosecondFraction()``
returns the full fraction within that second.
Neither changes the stored precision.
See :doc:`working_with_times` for constructing and working with the underlying ``Time``.

.. erbsland-demo::
    :source: time/TimeWithZone/Create.cpp
    :exec: time/time_with_zone --demo Create
    :source-sha256: 13dfaf1f3db90b06652ff3ea4b99d0eb40a9a6f07fedd860b4b9d15698a86697

.. code-block:: cpp

    /// Keep a daily wall-clock reading with its intended zone before choosing a date.
    /// Component accessors retain fractional seconds; equality compares the stored time and zone.
    /// @notest{Compiled and executed documentation demo.}
    void create() {
        const auto movement = "Αντάντε"_el;
        const auto reading = el::Time{el::Hour{9}, el::Minute{15}, el::Second{12}, el::Nanoseconds{123456789}};
        const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
        const auto rehearsal = el::TimeWithZone{reading, zone};
        const auto fixed = el::TimeWithZone{reading, el::TimeZone{el::Hours{2}}};
        const auto local = el::TimeWithZone{reading, el::TimeZone::local()};

        // A time alone defaults to UTC; default construction is midnight UTC.
        el::io::printLine(
            el::StringFormat{"{}: {}\nDefault: {}\nTime-only: {}"_el}.build(
                movement, rehearsal, el::TimeWithZone{}, el::TimeWithZone{reading}));
        el::io::printLine(
            el::StringFormat{"Hour: {}\nMinute: {}\nSecond: {}\nMs fraction: {}\nNs fraction: {}"_el}.build(
                rehearsal.hour().toRawValue(),
                rehearsal.minute().toRawValue(),
                rehearsal.second().toRawValue(),
                rehearsal.millisecondFraction().toRawValue(),
                rehearsal.nanosecondFraction().toRawValue()));
        el::io::printLine(
            el::StringFormat{"Reading retained: {}\nNamed zone: {}\nLocal origin: {}"_el}.build(
                rehearsal.time() == reading, rehearsal.timeZone().isNamed(), local.timeZone().isLocalTime()));
        el::io::printLine(
            el::StringFormat{"Equal copy: {}\nEqual to fixed-zone reading: {}"_el}.build(
                rehearsal == el::TimeWithZone{reading, zone}, rehearsal == fixed));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Αντάντε: 09:15:12.123456789[Europe/Athens]
    Default: 00:00:00Z
    Time-only: 09:15:12.123456789Z
    Hour: 9
    Minute: 15
    Second: 12
    Ms fraction: 123
    Ns fraction: 123456789
    Reading retained: true
    Named zone: true
    Local origin: true
    Equal copy: true
    Equal to fixed-zone reading: false

.. erbsland-demo-end::

Equality compares the stored reading and zone, including the zone's local-origin marker.
An equal copy expresses the same stored intention.
A named-zone reading and a fixed-offset reading are different values even if their offsets happen to match on some date.
Without a date, equality cannot establish that two readings represent the same instant, and the type provides no
chronological ordering.
If you need to compare occurrences, resolve their dates first and compare the resulting instants.

Resolving a Chosen Date
=======================

The ``DateTime(Date, TimeWithZone)`` constructor interprets the supplied date and reading as local fields in the stored
zone.
It resolves the offset and stores the resulting UTC date and time.
The display still uses the chosen zone, so you can inspect the local reading alongside ``utcDate()`` and ``utcTime()``,
or use ``toUtc()`` to show the selected instant directly.

.. erbsland-demo::
    :source: time/TimeWithZone/Resolve.cpp
    :exec: time/time_with_zone --demo Resolve
    :source-sha256: 4a92e79286ff120cabc293fd204a1b9377497ccf19e4410fa9b68b652feb8466

.. code-block:: cpp

    /// Resolve a stored wall-clock time separately for each chosen local date.
    /// Named-zone rules can give the same reading different UTC offsets in winter and summer.
    /// @notest{Compiled and executed documentation demo.}
    void resolve() {
        const auto movement = "Αντάντε"_el;
        const auto rehearsal =
            el::TimeWithZone{el::Time{el::Hour{9}, el::Minute{15}}, el::TimeZone::fromNameOrThrow("Europe/Athens"_el)};
        for (
            const auto date :
            {el::Date::fromYearMonthDayOrThrow(2026, 1, 15), el::Date::fromYearMonthDayOrThrow(2026, 7, 15)}) {
            // The Date is local input; DateTime stores the resulting UTC instant.
            const auto occurrence = el::DateTime{date, rehearsal};
            el::io::printLine(
                el::StringFormat{"{}: local {}\nUTC {}\nOffset seconds: {}"_el}.build(
                    movement, occurrence, occurrence.toUtc(), occurrence.timeOffset().toSeconds().toRawValue()));
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Αντάντε: local 2026-01-15 09:15:00+02:00
    UTC 2026-01-15 07:15:00Z
    Offset seconds: 7200
    Αντάντε: local 2026-07-15 09:15:00+03:00
    UTC 2026-07-15 06:15:00Z
    Offset seconds: 10800

.. erbsland-demo-end::

Notice what stays the same in the two examples: both occurrences keep ``09:15`` on their local date.
The winter occurrence uses ``+02:00`` and the summer occurrence uses ``+03:00``, selecting different UTC clock readings.
The ``TimeWithZone`` itself stays the same; each date supplies the missing context.

Resolve each occurrence separately when generating a schedule.
Keeping one occurrence's ``timeOffset()`` and reusing it for later dates would turn a named-zone schedule into a
fixed-offset schedule.
For complete ``DateTime`` construction, field extraction, and UTC range boundaries, continue with
:doc:`working_with_datetime`.

When a Reading Repeats or Does Not Occur
========================================

On most dates, a local reading selects one instant.
At some zone transitions, however, it selects two instants or none.
A *fold* is a repeated interval after the clock moves backward; a *gap* is an interval skipped when it moves forward.
These are properties of the chosen local date and zone, so ``TimeWithZone`` cannot decide them by itself.

During a fold, :cpp:enum:`TimeOccurrenceInFold <erbsland::time::TimeOccurrenceInFold>` chooses which occurrence to use.
``First`` selects the earlier UTC instant and is the default.
``Second`` selects the later one.
The local clock fields are identical, but the resolved offsets and UTC instants differ.
For a daily schedule, you can make the earlier occurrence your usual policy.
For a specific appointment, the person choosing the time may need to distinguish the two occurrences.
``TimeWithZone`` stores the clock reading and zone, so retain that additional choice alongside it when needed.

A gap is resolved using the zone database's transition rule rather than rejected as an invalid date/time.
The constructor retains the requested local display fields with the selected offset, so merely inspecting its ``time()``
can hide the skipped reading.
To check the result, convert the selected instant to UTC and back to the named zone, then compare the refreshed local
date and time with the request.

.. erbsland-demo::
    :source: time/TimeWithZone/Transitions.cpp
    :exec: time/time_with_zone --demo Transitions
    :source-sha256: 7ff2b68e3134b43a677338fcb2f88d4edd3f096865291f556520be3d4fa1a623

.. code-block:: cpp

    /// Choose a fold occurrence and check a gap by resolving the selected instant back into its zone.
    /// A valid DateTime can still originate from a local reading that never occurred.
    /// @notest{Compiled and executed documentation demo.}
    void transitions() {
        const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
        const auto rehearsal = el::TimeWithZone{el::Time{el::Hour{3}, el::Minute{30}}, zone};
        const auto foldDate = el::Date::fromYearMonthDayOrThrow(2026, 10, 25);
        const auto first = el::DateTime{foldDate, rehearsal, el::TimeOccurrenceInFold::First};
        const auto second = el::DateTime{foldDate, rehearsal, el::TimeOccurrenceInFold::Second};
        el::io::printLine(
            el::StringFormat{"Fold first UTC: {}\nSecond UTC: {}\nDefault chooses first: {}"_el}.build(
                first.toUtc(), second.toUtc(), el::DateTime{foldDate, rehearsal} == first));

        // Refresh display metadata from UTC before comparing the requested local fields.
        const auto gapDate = el::Date::fromYearMonthDayOrThrow(2026, 3, 29);
        const auto selected = el::DateTime{gapDate, rehearsal};
        const auto resolved = selected.toUtc().toTimeZone(zone);
        el::io::printLine(
            el::StringFormat{"Gap requested: {} {}\nConstructor: {}\nResolved: {}"_el}.build(
                gapDate, rehearsal.time(), selected, resolved));
        el::io::printLine(
            el::StringFormat{"Gap valid: {}\nRequested fields occurred: {}\nInvalid date accepted: {}"_el}.build(
                selected.isValid(),
                resolved.date() == gapDate && resolved.time() == rehearsal.time(),
                el::DateTime{el::Date{}, rehearsal}.isValid()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Fold first UTC: 2026-10-25 00:30:00Z
    Second UTC: 2026-10-25 01:30:00Z
    Default chooses first: true
    Gap requested: 2026-03-29 03:30:00
    Constructor: 2026-03-29 03:30:00+03:00
    Resolved: 2026-03-29 02:30:00+02:00
    Gap valid: true
    Requested fields occurred: false
    Invalid date accepted: false

.. erbsland-demo-end::

In the Athens fold, ``03:30`` occurs twice, one hour apart.
In the spring gap, the constructor selects an instant for the requested ``03:30``, but refreshing that instant's zone
metadata displays ``02:30``.
That difference is the useful signal: the requested local fields did not occur at the selected instant.
Do not assume every gap shifts a reading forward, or every transition changes the clock by an hour.
:doc:`working_with_time_zones` develops the resolution rules with a half-hour transition as well.

``isValid()`` answers whether a ``DateTime`` has a valid date representation; it does not answer whether a requested
reading occurred in the zone.
An invalid input ``Date`` produces an invalid ``DateTime``, while the gap example remains valid.
Check validity first, then compare both the refreshed date and time if your application must reject skipped local times.
Comparing only the time would miss a resolution that also changes the local date.
A field match does not distinguish the two sides of a fold, since both occurrences have those fields.
That choice remains the responsibility of the occurrence policy.

Displaying the Intended Time
============================

:cpp:func:`toString() <erbsland::time::TimeWithZone::toString>` gives you compact text for the undated intention.
UTC adds ``Z``; a fixed offset adds a signed numeric suffix, including seconds when necessary.
A named zone adds its primary name in brackets instead of pretending that one offset can describe every occurrence.
A zone obtained from ``TimeZone::local()`` omits the zone from this compact output.

.. erbsland-demo::
    :source: time/TimeWithZone/Display.cpp
    :exec: time/time_with_zone --demo Display
    :source-sha256: fbf1567b4562a82e75030049928f9ec6074bffbae28d1fa00397925bee017f7e

.. code-block:: cpp

    /// Display an intended daily time with UTC, fixed, named, or system-local zone information.
    /// Compact output omits local-origin zones and does not identify a dated instant.
    /// @notest{Compiled and executed documentation demo.}
    void display() {
        const auto reading = el::Time{el::Hour{9}, el::Minute{15}};
        for (
            const auto zone :
            {el::TimeZone::utc(),
                el::TimeZone{el::Hours{5}, el::Minutes{30}},
                el::TimeZone::fromNameOrThrow("Europe/Athens"_el),
                el::TimeZone::local()}) {
            const auto rehearsal = el::TimeWithZone{reading, zone};
            el::io::printLine(
                el::StringFormat{"Intended time: {}\nLocal origin: {}"_el}.build(
                    rehearsal.toString(), rehearsal.timeZone().isLocalTime()));
        }
        // Second-level fixed offsets remain visible when needed.
        el::io::printLine(
            el::StringFormat{"Second-level offset: {}"_el}.build(
                el::TimeWithZone{reading, el::TimeZone{el::Hours{-3}, el::Minutes{-30}, el::Seconds{-15}}}));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Intended time: 09:15:00Z
    Local origin: false
    Intended time: 09:15:00+05:30
    Local origin: false
    Intended time: 09:15:00[Europe/Athens]
    Local origin: false
    Intended time: 09:15:00
    Local origin: true
    Second-level offset: 09:15:00-03:30:15

.. erbsland-demo-end::

The plain local-origin output is convenient for a display already understood to use the system's zone.
It does not preserve the zone's identity for a reader on another machine.
Likewise, even the bracketed named-zone output lacks a date and cannot identify a dated instant.
For a stored recurring intention, keep the clock reading and zone identity explicitly; for a resolved event, use the
exchange representations described in :doc:`working_with_timestamps` or :doc:`working_with_datetime`.

Keeping a Local Time Across Dates
=================================

A recurring local appointment has two pieces: the next local date and the stored ``TimeWithZone``.
Advance the date through the calendar, then construct a fresh ``DateTime`` with the same zoned reading.
This preserves the wall-clock intention while allowing the elapsed interval between occurrences to change.

.. erbsland-demo::
    :source: time/TimeWithZone/Recurrence.cpp
    :exec: time/time_with_zone --demo Recurrence
    :source-sha256: 7d06cb6f3a621b354298e0c1865e07cbcd7274c74b0ac0198675dac437212bd7

.. code-block:: cpp

    /// Preserve an intended local time by resolving it anew on each local calendar date.
    /// Adding a fixed elapsed day can change the displayed time across a zone transition.
    /// @notest{Compiled and executed documentation demo.}
    void recurrence() {
        const auto movement = "Αντάντε"_el;
        const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
        const auto rehearsal = el::TimeWithZone{el::Time{el::Hour{9}, el::Minute{15}}, zone};
        auto localDate = el::Date::fromYearMonthDayOrThrow(2026, 3, 28);
        const auto first = el::DateTime{localDate, rehearsal};
        for (auto day = 0; day < 3; ++day) {
            const auto selected = el::DateTime{localDate, rehearsal, el::TimeOccurrenceInFold::First};
            const auto resolved = selected.toUtc().toTimeZone(zone);
            const auto fieldsMatch =
                resolved.isValid() && resolved.date() == localDate && resolved.time() == rehearsal.time();
            el::io::printLine(
                el::StringFormat{"{}: {}\nRequested fields retained: {}"_el}.build(movement, resolved, fieldsMatch));
            if (!fieldsMatch) {
                // This recurrence rejects a skipped reading instead of silently changing its time.
                el::io::printLine("Occurrence needs a gap policy\nStop generating dates."_el);
                return;
            }
            localDate = localDate.next();
        }
        const auto fixedDayLater = first.added(el::Duration{el::Days{1}});
        const auto nextLocalDate = el::DateTime{first.date().next(), rehearsal};
        el::io::printLine(
            el::StringFormat{"Fixed day later: {}\nNext local occurrence: {}\nElapsed seconds: {}"_el}.build(
                fixedDayLater, nextLocalDate, first.durationTo(nextLocalDate).toSeconds().toRawValue()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Αντάντε: 2026-03-28 09:15:00+02:00
    Requested fields retained: true
    Αντάντε: 2026-03-29 09:15:00+03:00
    Requested fields retained: true
    Αντάντε: 2026-03-30 09:15:00+03:00
    Requested fields retained: true
    Fixed day later: 2026-03-29 10:15:00+03:00
    Next local occurrence: 2026-03-29 09:15:00+03:00
    Elapsed seconds: 82800

.. erbsland-demo-end::

Across the spring transition, the next local ``09:15`` is only twenty-three elapsed hours after the previous one.
Adding a fixed ``Duration{Days{1}}`` instead moves the UTC instant by 86,400 seconds and displays ``10:15`` on the next
local date.
Both calculations are useful, but they answer different scheduling questions.
:doc:`understanding_calendar_changes` explains why ``DateTime`` calendar arithmetic also operates on UTC fields rather
than promising to preserve the local clock reading.

The recurrence demo selects the first occurrence of a fold and checks each refreshed reading.
If the fields no longer match, it stops so that a skipped occurrence cannot silently become a different appointment.
This is one reasonable policy when retaining the requested clock reading matters more than producing an occurrence on
every date.
Your application can instead ask for a new time or apply an explicit adjustment policy.
Keep that decision visible where the schedule is generated.

Local date navigation belongs to ``Date``; :doc:`working_with_dates` covers ``next()`` and calendar boundaries, and
:doc:`working_with_calendar_parts` covers weekday and month-based selection.
At the final supported date, stop generating occurrences rather than repeatedly accepting the saturated endpoint.
