..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: TimeZone; Choosing and Resolving Zones
    single: Daylight Saving; Gaps and Folds
    single: Time; System-Local Zone
    single: TimeZoneId; Persistence

***********************
Working with Time Zones
***********************

A date and clock reading become useful across locations only when you know how they relate to UTC.
Sometimes you are interpreting a local appointment; sometimes you already know the instant and only want to display it
in another zone.
This page explains both directions, how to choose and inspect a ``TimeZone``, and how to handle the transitions that
make some local readings ambiguous or impossible.

A Zone Describes Rules; an Offset Describes a Shift
===================================================

A :cpp:class:`TimeZone <erbsland::time::TimeZone>` represents UTC, a fixed offset, or a named IANA zone.
UTC needs no shift.
A fixed offset such as ``+05:30`` always moves the displayed clock by the same amount.
A named zone such as ``Europe/Athens`` describes rules whose offset can depend on the date, including historical changes
and daylight-saving transitions.

For an individual instant, its *UTC offset* is the signed shift from UTC to the local display.
An offset of ``+03:00`` displays UTC noon as ``15:00`` locally; interpreting local ``15:00`` subtracts that offset to
select UTC noon.
The offset can also move the date across midnight, so local and UTC dates must stay paired with their corresponding
times.

An abbreviation such as ``EET`` or ``EEST`` is a helpful display label for the resolved reading.
It is not a reliable zone identifier: abbreviations can be shared by unrelated zones and can change seasonally.
Keep a named zone when you want its rules, and an explicit offset when the relationship to UTC is intentionally fixed.

``TimeZone::local()`` adds another property: it records that the zone came from the system-local setting.
This is an origin marker alongside the UTC, fixed, or named category.
A local-origin named zone still resolves its rules for the date you supply.

Choosing a Zone
===============

Default construction and ``TimeZone::utc()`` create UTC.
The component constructor takes ``Hours`` with optional ``Minutes`` and ``Seconds`` amounts, while construction from a
``Duration`` supplies the total fixed offset.
A named zone comes from a supported IANA name or alias.
You can therefore choose the representation from the intent of your input: a constant shift belongs to a fixed offset,
while a place's civil clock belongs to a named zone.

:cpp:func:`fromName() <erbsland::time::TimeZone::fromName>` returns ``std::nullopt`` for an unknown name, allowing you to
keep a failed lookup separate from an intentional choice of UTC.
``fromNameOrThrow()`` reports the same failure with ``err::ParseError``.
``isValidName()`` checks whether a name is accepted; when you need the value as well, one ``fromName()`` lookup is
enough.

.. erbsland-demo::
    :source: time/TimeZone/Create.cpp
    :exec: time/time_zone --demo Create
    :source-sha256: 039881f8cabef0bac1e2b5be85ac7dda577da1b3bd295ec9f8b6628227915980

.. code-block:: cpp

    /// Choose UTC, a fixed offset, or a supported named zone with explicit lookup failure handling.
    /// Fixed-offset components have independent signs; total durations discard complete 24-hour rotations.
    /// @notest{Compiled and executed documentation demo.}
    void create() {
        const auto movement = "Αντάντε"_el;
        const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
        el::io::printLine(
            el::StringFormat{"{}: named zone {}\nDefault UTC: {}\nUtc() UTC: {}"_el}.build(
                movement, zone.name(), el::TimeZone{}.isUtc(), el::TimeZone::utc().isUtc()));
        const auto input = "Unknown/Composition"_el;
        if (const auto parsed = el::TimeZone::fromName(input); parsed.has_value()) {
            el::io::printLine(el::StringFormat{"Chosen zone: {}"_el}.build(parsed->name()));
        } else {
            el::io::printLine("Unknown zone: choose a supported name before resolving local input."_el);
        }
        try {
            [[maybe_unused]] const auto rejected = el::TimeZone::fromNameOrThrow(input);
        } catch (const el::err::ParseError &) {
            el::io::printLine("Throwing lookup reports ParseError."_el);
        }
        el::io::printLine(
            el::StringFormat{"Athens supported: {}\nUnknown supported: {}"_el}.build(
                el::TimeZone::isValidName("Europe/Athens"_el), el::TimeZone::isValidName(input)));

        // Apply the sign to each nonzero component of a negative offset.
        const auto negative = el::TimeZone{el::Hours{-3}, el::Minutes{-30}, el::Seconds{-15}};
        const auto mixed = el::TimeZone{el::Hours{-3}, el::Minutes{30}};
        const auto clamped = el::TimeZone{el::Hours{30}, el::Minutes{90}, el::Seconds{90}};
        const auto normalized = el::TimeZone{el::Duration{el::Hours{27}}};
        el::io::printLine(
            el::StringFormat{"Negative seconds: {}\nMixed signs: {}\nClamped parts: {}\nNormalized total: {}"_el}.build(
                negative.staticOffset().toSeconds().toRawValue(),
                mixed.staticOffset().toSeconds().toRawValue(),
                clamped.staticOffset().toSeconds().toRawValue(),
                normalized.staticOffset().toSeconds().toRawValue()));
        for (const auto name : {"Z"_el, "GMT"_el, "+0530"_el, "UTC+05:00"_el, "Etc/GMT+5"_el}) {
            const auto parsed = el::TimeZone::fromNameOrThrow(name);
            const auto instant =
                el::DateTime{el::Date::fromYearMonthDayOrThrow(2026, 7, 15), el::Time{}}.toTimeZone(parsed);
            el::io::printLine(
                el::StringFormat{"{}: offset seconds {}"_el}.build(name, instant.timeOffset().toSeconds().toRawValue()));
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Αντάντε: named zone Europe/Athens
    Default UTC: true
    Utc() UTC: true
    Unknown zone: choose a supported name before resolving local input.
    Throwing lookup reports ParseError.
    Athens supported: true
    Unknown supported: false
    Negative seconds: -12615
    Mixed signs: -9000
    Clamped parts: 86399
    Normalized total: 10800
    Z: offset seconds 0
    GMT: offset seconds 0
    +0530: offset seconds 19800
    UTC+05:00: offset seconds 18000
    Etc/GMT+5: offset seconds -18000

.. erbsland-demo-end::

Fixed-offset components carry independent signs.
``Hours{-3}, Minutes{-30}`` means ``-03:30``, whereas ``Hours{-3}, Minutes{30}`` means ``-02:30``.
The component constructor clamps hours to ``-23..23`` and minutes and seconds to ``-59..59`` before adding them.
The total-duration constructor removes complete 24-hour rotations with a signed remainder: ``+27`` hours becomes ``+03``
hours, and ``-27`` hours becomes ``-03`` hours.
Zero becomes UTC; offsets such as ``+13:00`` and ``+14:00`` retain their direction.
These constructors normalize rather than reject out-of-range values, so validate supplied components first if your input
must be accepted exactly as written.

Recognized Text and Aliases
---------------------------

Alongside IANA names and aliases, lookup accepts the special UTC names ``UTC``, ``GMT``, ``Z``, ``Universal``, ``Zulu``,
and ``Factory``.
The special-name parser accepts ASCII case variations.
Use the spelling supplied by ``names()`` for named-zone choices.
Fixed-offset text accepts a signed hour, an hour/minute pair, or a colon-separated hour/minute/second triple, optionally
prefixed by ``UTC`` or ``GMT``.
Examples include ``+5``, ``+0530``, ``-03:30``, ``UTC+05:30``, and ``GMT-03:30:15``.
In a colon-separated form, minutes and seconds have two digits; the hour has one or two digits.
Only the colon-separated form accepts seconds.
Text offsets reject hours above 23 or minutes/seconds above 59 rather than applying the component constructor's
clamping.

The demo contrasts ``UTC+05:00`` with the IANA name ``Etc/GMT+5``.
The former means five hours ahead of UTC; the latter uses the IANA ``Etc/GMT`` sign convention and means five hours
behind UTC.
Treat ``Etc/GMT`` names as database identifiers rather than rewriting their suffix as a numeric ISO offset.

``TimeZone::names()`` provides supported IANA names and aliases for a selection interface.
After lookup, ``name()`` returns a named zone's primary name, so an alias may produce a different canonical spelling.
UTC and fixed-offset values return an empty name.
The database demo later on this page shows how to query the list and inspect an alias without hard-coding its size.

Inspecting and Comparing Zones
==============================

``isUtc()``, ``isStaticOffset()``, and ``isNamed()`` distinguish the three categories.
``isStaticOffset()`` means a fixed *nonzero* offset; zero is UTC.
``isLocalTime()`` independently tells you whether the value originated from the system-local setting.
``id()`` identifies a named zone within the bundled database and returns the UTC identifier for UTC and fixed offsets.

:cpp:func:`staticOffset() <erbsland::time::TimeZone::staticOffset>` gives you the fixed offset as a ``Duration``.
For a named zone it returns zero, because the offset depends on the date.
That zero is not a claim about the zone's current resolved offset.
Resolve or convert a ``DateTime`` and read its ``timeOffset()`` when you need the shift at an instant.

.. erbsland-demo::
    :source: time/TimeZone/Inspect.cpp
    :exec: time/time_zone --demo Inspect
    :source-sha256: 1e88397154ac1a5ab1794a041479fa135fc054929f8ed84ca0e0b474e45feba5

.. code-block:: cpp

    /// Inspect a zone's category separately from its system-local origin and resolved offset.
    /// Zone equality compares the stored identity and origin, rather than a seasonal offset.
    /// @notest{Compiled and executed documentation demo.}
    void inspect() {
        const auto named = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
        const auto fixed = el::TimeZone{el::Hours{2}};
        for (const auto zone : {el::TimeZone::utc(), fixed, named, el::TimeZone::local()}) {
            el::io::printLine(
                el::StringFormat{"UTC: {}\nFixed: {}\nNamed: {}\nLocal: {}\nName: {}\nUTC id: {}\nStatic seconds: {}"_el}
                    .build(
                        zone.isUtc(),
                        zone.isStaticOffset(),
                        zone.isNamed(),
                        zone.isLocalTime(),
                        zone.name(),
                        zone.id().isUtc(),
                        zone.staticOffset().toSeconds().toRawValue()));
        }
        el::io::printLine(
            el::StringFormat{"Named equals fixed: {}\nAlias equals primary: {}"_el}.build(
                named == fixed,
                el::TimeZone::fromNameOrThrow("US/Eastern"_el) == el::TimeZone::fromNameOrThrow("America/New_York"_el)));
        for (
            const auto date :
            {el::Date::fromYearMonthDayOrThrow(2026, 1, 15), el::Date::fromYearMonthDayOrThrow(2026, 7, 15)}) {
            const auto namedValue = el::DateTime{date, el::Time{}, named};
            const auto fixedValue = el::DateTime{date, el::Time{}, fixed};
            el::io::printLine(
                el::StringFormat{"{}: named seconds {}\nFixed seconds {}\nOffsets equal: {}"_el}.build(
                    date,
                    namedValue.timeOffset().toSeconds().toRawValue(),
                    fixedValue.timeOffset().toSeconds().toRawValue(),
                    namedValue.timeOffset() == fixedValue.timeOffset()));
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    UTC: true
    Fixed: false
    Named: false
    Local: false
    Name:
    UTC id: true
    Static seconds: 0
    UTC: false
    Fixed: true
    Named: false
    Local: false
    Name:
    UTC id: true
    Static seconds: 7200
    UTC: false
    Fixed: false
    Named: true
    Local: false
    Name: Europe/Athens
    UTC id: false
    Static seconds: 0
    UTC: false
    Fixed: false
    Named: true
    Local: true
    Name: Europe/Zurich
    UTC id: false
    Static seconds: 0
    Named equals fixed: false
    Alias equals primary: true
    2026-01-15: named seconds 7200
    Fixed seconds 7200
    Offsets equal: true
    2026-07-15: named seconds 10800
    Fixed seconds 7200
    Offsets equal: false

.. erbsland-demo-end::

It is tempting to compare zones by their current offset, but that loses the rules that distinguish them.
Zone equality compares stored zone values and local origin, rather than offsets on one particular date.
An alias resolving to the same primary zone compares equal to that explicit primary zone.
A named zone and a fixed offset remain different even when they share an offset in winter.
The summer example shows why: their offsets can later diverge without either zone value changing.

Displaying an Existing Instant
==============================

When you already have an instant, :cpp:func:`DateTime::toTimeZone() <erbsland::time::DateTime::toTimeZone>` returns a
value with that same UTC instant and a display resolved for the target zone.
Targets can be UTC, fixed-offset, named, or system-local values.
The local ``date()`` and ``time()`` may change, while ``utcDate()`` and ``utcTime()`` remain the same.

``timeOffset()`` returns the resolved shift as a ``Duration``, ``timeZone()`` returns the retained zone, and
``timeZoneAbbreviation()`` returns the named-zone label.
UTC and fixed offsets have no named-zone abbreviation, so an empty label is expected there.

.. erbsland-demo::
    :source: time/TimeZone/Convert.cpp
    :exec: time/time_zone --demo Convert
    :source-sha256: e62dc75bed197825b5fb444ff298a768a52ad328c2c9459a07ab265c10acd696

.. code-block:: cpp

    /// Display one UTC instant in different zones without changing its chronological position.
    /// Local fields, resolved offsets, and abbreviations belong to the selected display zone.
    /// @notest{Compiled and executed documentation demo.}
    void convert() {
        const auto instant =
            el::DateTime{el::Date::fromYearMonthDayOrThrow(2026, 7, 15), el::Time{el::Hour{23}, el::Minute{30}}};
        for (
            const auto zone :
            {el::TimeZone::utc(),
                el::TimeZone{el::Hours{5}, el::Minutes{30}},
                el::TimeZone::fromNameOrThrow("Europe/Athens"_el),
                el::TimeZone::local()}) {
            const auto displayed = instant.toTimeZone(zone);
            el::io::printLine(
                el::StringFormat{"Local: {} {}\nUTC: {} {}\nOffset seconds: {}\nAbbreviation: {}"_el}.build(
                    displayed.date(),
                    displayed.time(),
                    displayed.utcDate(),
                    displayed.utcTime(),
                    displayed.timeOffset().toSeconds().toRawValue(),
                    displayed.timeZoneAbbreviation()));
            el::io::printLine(
                el::StringFormat{"Same ordering position: {}\nSame value: {}\nLocal origin: {}\nNamed zone: {}"_el}.build(
                    (displayed <=> instant) == std::strong_ordering::equal,
                    displayed == instant,
                    displayed.timeZone().isLocalTime(),
                    displayed.timeZone().isNamed()));
        }
        const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
        for (
            const auto date :
            {el::Date::fromYearMonthDayOrThrow(2026, 1, 15), el::Date::fromYearMonthDayOrThrow(2026, 7, 15)}) {
            const auto displayed = el::DateTime{date, el::Time{el::Hour{12}, el::Minute{}}}.toTimeZone(zone);
            el::io::printLine(
                el::StringFormat{"Seasonal display: {}\nOffset seconds: {}\nLabel: {}"_el}.build(
                    displayed, displayed.timeOffset().toSeconds().toRawValue(), displayed.timeZoneAbbreviation()));
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Local: 2026-07-15 23:30:00
    UTC: 2026-07-15 23:30:00
    Offset seconds: 0
    Abbreviation:
    Same ordering position: true
    Same value: true
    Local origin: false
    Named zone: false
    Local: 2026-07-16 05:00:00
    UTC: 2026-07-15 23:30:00
    Offset seconds: 19800
    Abbreviation:
    Same ordering position: true
    Same value: false
    Local origin: false
    Named zone: false
    Local: 2026-07-16 02:30:00
    UTC: 2026-07-15 23:30:00
    Offset seconds: 10800
    Abbreviation: EEST
    Same ordering position: true
    Same value: false
    Local origin: false
    Named zone: true
    Local: 2026-07-16 01:30:00
    UTC: 2026-07-15 23:30:00
    Offset seconds: 7200
    Abbreviation: CEST
    Same ordering position: true
    Same value: false
    Local origin: true
    Named zone: true
    Seasonal display: 2026-01-15 14:00:00+02:00
    Offset seconds: 7200
    Label: EET
    Seasonal display: 2026-07-15 15:00:00+03:00
    Offset seconds: 10800
    Label: EEST

.. erbsland-demo-end::

The UTC reading near midnight displays on the next date in the positive-offset zones.
This date change is a display conversion; it does not move the instant.
Chronological comparison therefore gives the same ordering position before and after conversion.
``DateTime`` equality also includes display metadata, so the converted value can compare unequal with ``==`` while
occupying the same chronological position.
Convert both values to UTC if you want equality of their instants.
:doc:`working_with_datetime` explains these comparison semantics in more detail.

The seasonal examples resolve the named zone for each instant and show both its offset and abbreviation changing.
Retain the zone identity instead of carrying a resolved winter offset into summer conversions.
Invalid ``DateTime`` values stay invalid on zone conversion; conversion does not supply a missing date.

Interpreting Local Input
========================

Suppose the date and clock reading came from a local appointment form.
Those fields are not yet a UTC instant: the chosen zone is part of their meaning.
Local-to-UTC resolution starts from this local intention.
The ``DateTime(Date, Time, TimeZone)`` constructor treats the supplied fields as a local reading in the chosen zone,
then resolves and stores the corresponding UTC instant.
The two-argument ``DateTime(Date, Time)`` constructor treats the same fields as UTC.
Converting that second value to the zone changes only its display, so it generally selects a different instant from the
local-input constructor.

.. erbsland-demo::
    :source: time/TimeZone/Resolve.cpp
    :exec: time/time_zone --demo Resolve
    :source-sha256: 5dd3e6a2f4cb64e52e97e0d3bd88d1fc7cc17cb90da89fcfe1c01a2a96a4721e

.. code-block:: cpp

    /// Interpret local calendar fields in a zone, or convert an already-known UTC instant to that zone.
    /// These operations start with different information and need not select the same instant.
    /// @notest{Compiled and executed documentation demo.}
    void resolve() {
        const auto date = el::Date::fromYearMonthDayOrThrow(2026, 7, 15);
        const auto reading = el::Time{el::Hour{9}, el::Minute{15}};
        const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
        const auto localInput = el::DateTime{date, reading, zone};
        const auto utcInput = el::DateTime{date, reading};
        const auto converted = utcInput.toTimeZone(zone);
        el::io::printLine(el::StringFormat{"Local input: {}\nIts UTC: {}"_el}.build(localInput, localInput.toUtc()));
        el::io::printLine(
            el::StringFormat{"UTC input: {}\nIts zoned display: {}\nSame instant as local input: {}"_el}.build(
                utcInput, converted, converted.toUtc() == localInput.toUtc()));
        el::io::printLine(
            el::StringFormat{"Invalid local date accepted: {}"_el}.build(
                el::DateTime{el::Date{}, reading, zone}.isValid()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Local input: 2026-07-15 09:15:00+03:00
    Its UTC: 2026-07-15 06:15:00Z
    UTC input: 2026-07-15 09:15:00Z
    Its zoned display: 2026-07-15 12:15:00+03:00
    Same instant as local input: false
    Invalid local date accepted: false

.. erbsland-demo-end::

For the summer Athens reading, local ``09:15`` selects UTC ``06:15``.
Starting instead from UTC ``09:15`` and converting to Athens displays ``12:15``.
Choose the operation from the meaning of your input: local fields need interpretation, while an already-recorded instant
needs display conversion.

An invalid ``Date`` produces an invalid ``DateTime``.
For typed construction, date validation, precision, and supported UTC boundaries, see :doc:`working_with_datetime`.
If the date is not known yet, :doc:`keeping_wall_clock_time_with_zone` shows how ``TimeWithZone`` retains the intended
reading and zone until they can be resolved together.

Understanding Clock Transitions
===============================

When the local clock advances, some readings are skipped: the skipped interval is a *gap*.
When it retreats, some readings repeat: the repeated interval is a *fold*.
A local date and time inside a fold can correspond to two UTC instants; a reading inside a gap corresponds to none.
UTC-to-local conversion remains unambiguous because it starts from an instant rather than an incomplete local reading.

Choosing an Occurrence in a Fold
--------------------------------

:cpp:enum:`TimeOccurrenceInFold <erbsland::time::TimeOccurrenceInFold>` disambiguates local construction.
``First`` selects the earlier matching UTC instant and is the default; ``Second`` selects the later one.
Outside a fold, the choices select the same instant.
Inside a fold, comparing local fields cannot distinguish the occurrences, because both have the requested fields.
Select an occurrence explicitly when an input or scheduling policy depends on that distinction.

Checking a Reading in a Gap
---------------------------

For a gap, the constructor selects an offset using the bundled database's local transition rule.
It does not reject the reading and does not promise to shift every skipped time forward.
It retains the requested local fields with that selected offset, which can differ from the offset resolved for the
resulting UTC instant.
Refresh the zone metadata through ``value.toUtc().toTimeZone(zone)`` before comparing the local fields with the request.
Calling ``toTimeZone(zone)`` also resolves the offset from UTC directly; the two-step form makes that direction
explicit.

.. erbsland-demo::
    :source: time/TimeZone/Transitions.cpp
    :exec: time/time_zone --demo Transitions
    :source-sha256: e829bca43091194d6ca76a18070f82fd0244888bde865c1019e88b9aa4c4bb3b

.. code-block:: cpp

    /// Resolve folds and gaps using the named zone's rules, including half-hour transitions.
    /// Refresh the constructor's display from UTC to check whether requested local fields actually occurred.
    /// @notest{Compiled and executed documentation demo.}
    void transitions() {
        const auto zone = el::TimeZone::fromNameOrThrow("Australia/Lord_Howe"_el);
        const auto foldDate = el::Date::fromYearMonthDayOrThrow(2026, 4, 5);
        const auto repeated = el::Time{el::Hour{1}, el::Minute{45}};
        const auto first = el::DateTime{foldDate, repeated, zone, el::TimeOccurrenceInFold::First};
        const auto second = el::DateTime{foldDate, repeated, zone, el::TimeOccurrenceInFold::Second};
        el::io::printLine(
            el::StringFormat{"Fold first: {}\nSecond: {}\nElapsed seconds: {}"_el}.build(
                first, second, first.durationTo(second).toSeconds().toRawValue()));
        el::io::printLine(
            el::StringFormat{"UTC first: {}\nUTC second: {}\nDefault chooses first: {}"_el}.build(
                first.toUtc(), second.toUtc(), el::DateTime{foldDate, repeated, zone} == first));

        const auto gapDate = el::Date::fromYearMonthDayOrThrow(2026, 10, 4);
        const auto skipped = el::Time{el::Hour{2}, el::Minute{15}};
        const auto selected = el::DateTime{gapDate, skipped, zone};
        const auto resolved = selected.toUtc().toTimeZone(zone);
        el::io::printLine(
            el::StringFormat{"Gap requested: {} {}\nConstructor: {}\nResolved: {}\nUTC: {}"_el}.build(
                gapDate, skipped, selected, resolved, resolved.toUtc()));
        el::io::printLine(
            el::StringFormat{"Valid: {}\nRequested fields occurred: {}\nGap occurrence choice changes instant: {}"_el}
                .build(
                    selected.isValid(),
                    resolved.date() == gapDate && resolved.time() == skipped,
                    selected.toUtc() != el::DateTime{gapDate, skipped, zone, el::TimeOccurrenceInFold::Second}.toUtc()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Fold first: 2026-04-05 01:45:00+11:00
    Second: 2026-04-05 01:45:00+10:30
    Elapsed seconds: 1800
    UTC first: 2026-04-04 14:45:00Z
    UTC second: 2026-04-04 15:15:00Z
    Default chooses first: true
    Gap requested: 2026-10-04 02:15:00
    Constructor: 2026-10-04 02:15:00+11:00
    Resolved: 2026-10-04 01:45:00+10:30
    UTC: 2026-10-03 15:15:00Z
    Valid: true
    Requested fields occurred: false
    Gap occurrence choice changes instant: false

.. erbsland-demo-end::

On Lord Howe Island, the demo's fold repeats ``01:45`` at offsets ``+11:00`` and ``+10:30``.
The two UTC instants are thirty minutes apart.
The spring transition skips ``02:15``; the constructor's selected instant, once refreshed, displays ``01:45``.
Its gap adjustment is thirty minutes, demonstrating why a general policy must not assume one-hour transitions.

The gap reading remains a valid ``DateTime``.
``isValid()`` detects an invalid date representation, not a clock reading that was skipped by a zone transition.
If your application promises to accept the clock reading exactly as entered, a valid result alone is not enough.
Check validity and compare both refreshed local fields with the requested fields.
If they differ, reject or explicitly adjust the appointment; a field match alone does not resolve a fold policy.
``TimeOccurrenceInFold`` selects repeated occurrences and does not turn a gap into a valid local reading.

Adding an elapsed interval across a transition can change the local clock reading.
:doc:`understanding_calendar_changes` explains UTC arithmetic, and :doc:`keeping_wall_clock_time_with_zone` explains
resolving each date of a local recurrence independently.

Using the System-Local Zone
===========================

:cpp:func:`local() <erbsland::time::TimeZone::local>` identifies the system's base zone once and caches it for the process
lifetime.
On POSIX systems, detection uses ``TZ`` when supplied, otherwise a canonical zoneinfo path from the system setting or
``/etc/timezone``.
Windows time-zone names are mapped to IANA zones.
An unknown or unavailable setting falls back to UTC with the local-origin marker still set.

The cache retains the base zone, not today's numeric offset.
A named local zone still resolves winter, summer, and historical dates using the bundled rules.
Changing the system setting after the first query does not change the cached value; use an explicitly chosen zone when
your application needs to select a different one during its lifetime.

.. erbsland-demo::
    :source: time/TimeZone/Local.cpp
    :exec: time/time_zone --demo Local
    :source-sha256: 4be108162812170451610dec32b4d1013832cd30b03fd248255f1d6d0cf2722a

.. code-block:: cpp

    /// Use the cached system-local base zone while resolving its offset separately for each date.
    /// Copies and arithmetic preserve local origin; conversion adopts the target zone's origin.
    /// @notest{Compiled and executed documentation demo.}
    void local() {
        const auto zone = el::TimeZone::local();
        const auto date = el::Date::fromYearMonthDayOrThrow(2026, 7, 15);
        const auto value = el::DateTime{date, el::Time{el::Hour{9}, el::Minute{15}}, zone};
        // Reconstruct the same base zone explicitly to distinguish identity from local origin.
        const auto explicitZone = zone.isNamed() ? el::TimeZone{zone.id()} : el::TimeZone{zone.staticOffset()};
        el::io::printLine(el::StringFormat{"Local zone equals explicit base zone: {}"_el}.build(zone == explicitZone));
        const auto copy = value;
        const auto later = value.added(el::Duration{el::Days{1}});
        const auto utc = value.toTimeZone(el::TimeZone::utc());
        const auto returned = utc.toTimeZone(zone);
        el::io::printLine(
            el::StringFormat{"Cached zone equal: {}\nLocal origin: {}\nDefault display: {}"_el}.build(
                zone == el::TimeZone::local(), value.isLocalTime(), value.toString()));
        const auto format = el::IsoTimeFormatFlags{el::IsoTimeFormat::Extended, el::IsoTimeFormat::TimeShift};
        el::io::printLine(el::StringFormat{"Explicit numeric shift: {}"_el}.build(value.toIsoString(format)));
        el::io::printLine(
            el::StringFormat{"Copy local: {}\nArithmetic local: {}\nUTC local: {}\nReturned local: {}"_el}.build(
                copy.isLocalTime(), later.isLocalTime(), utc.isLocalTime(), returned.isLocalTime()));

        // The same cached zone resolves each occurrence using its date, not today's offset.
        for (const auto season : {el::Date::fromYearMonthDayOrThrow(2026, 1, 15), date}) {
            const auto occurrence = el::DateTime{season, el::Time{el::Hour{9}, el::Minute{}}, zone};
            el::io::printLine(
                el::StringFormat{"Local occurrence: {}\nOffset seconds: {}\nAbbreviation: {}"_el}.build(
                    occurrence, occurrence.timeOffset().toSeconds().toRawValue(), occurrence.timeZoneAbbreviation()));
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Local zone equals explicit base zone: false
    Cached zone equal: true
    Local origin: true
    Default display: 2026-07-15 09:15:00
    Explicit numeric shift: 2026-07-15 09:15:00+02:00
    Copy local: true
    Arithmetic local: true
    UTC local: false
    Returned local: true
    Local occurrence: 2026-01-15 09:00:00
    Offset seconds: 3600
    Abbreviation: CET
    Local occurrence: 2026-07-15 09:00:00
    Offset seconds: 7200
    Abbreviation: CEST

.. erbsland-demo-end::

The exact dates, offsets, and labels shown by this demo depend on the machine's setting.
Its behavior does not require that setting to be Athens or that it observe daylight saving.
A UTC fallback is still local-origin, so ``isLocalTime()`` reports origin rather than successful detection or a specific
geographic location.

Copies and date/time arithmetic preserve local origin.
Conversion adopts the target zone's marker: explicit UTC or named-zone conversion clears it, while conversion to
``TimeZone::local()`` sets it.
An explicit named zone may therefore compare unequal to the system-local value naming the same base zone.

Default ``DateTime`` and ``TimeWithZone`` string output omits the suffix for local-origin values.
For ISO output, ``IsoTimeFormat::TimeShift`` requests the resolved numeric shift even when the display is local-origin.
Zero is written as ``Z`` unless ``TimeShiftAlwaysComplete`` requests a numeric zero offset.
:doc:`customizing_iso_output` explains the formatting flags and precision choices.
Include the shift when text needs to retain an instant for readers outside the local display context.

Keeping Identity and Offset Details Distinct
============================================

:cpp:class:`TimeZoneId <erbsland::time::TimeZoneId>` is a transient identifier within the bundled database context.
You can obtain it through ``TimeZone::id()`` and construct a ``TimeZone`` from that identifier in the same context.
``toRawValue()`` exposes its underlying number, but the number is not a persistent zone identity across database
versions.
Default construction creates the UTC identifier; UTC and fixed offsets both return that identifier from ``id()``.
An ID round trip consequently cannot preserve a fixed offset or the local-origin marker.

For persistence, keep the primary IANA name of a named zone or the explicit signed offset of a fixed zone.
If the intention is to follow the system-local zone of the process that later reads the value, retain that choice
explicitly too.
An abbreviation is a display label and cannot substitute for any of these identities.

.. erbsland-demo::
    :source: time/TimeZone/Details.cpp
    :exec: time/time_zone --demo Details
    :source-sha256: 7fa40ee74781acafd8d7556126bda5b62a8e52605c88194727d1a39e2545b205

.. code-block:: cpp

    /// Keep a transient zone identifier within its database context and inspect low-level offset values.
    /// Persist the named zone's primary name rather than its numeric identifier or abbreviation index.
    /// @notest{Compiled and executed documentation demo.}
    void details() {
        const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
        const auto restored = el::TimeZone{zone.id()};
        const auto utcId = el::TimeZoneId{};
        const auto copiedId = el::TimeZoneId{zone.id().toRawValue()};
        el::io::printLine(
            el::StringFormat{"Stored name: {}\nId round trip: {}\nCopied id: {}\nDefault id UTC: {}"_el}.build(
                zone.name(), restored == zone, copiedId == zone.id(), utcId.isUtc()));

        // Public UTC and fixed constructors do not require database metadata.
        const auto utc = el::time::tz::TimeOffset{};
        const auto fixed = el::time::tz::TimeOffset{el::Seconds{19800}, true};
        for (const auto offset : {utc, fixed}) {
            el::io::printLine(
                el::StringFormat{
                    "Seconds: {}\nUTC: {}\nFixed: {}\nNamed: {}\nDST: {}\nLocal: {}\nUTC id: {}\nAbbreviation index: {}"_el}
                    .build(
                        offset.offset().toRawValue(),
                        offset.isUtc(),
                        offset.isStaticOffset(),
                        offset.isZone(),
                        offset.isDst(),
                        offset.isLocalTime(),
                        offset.zoneId().isUtc(),
                        offset.abbreviationId()));
        }
        const auto occurrence = el::DateTime{el::Date::fromYearMonthDayOrThrow(2026, 7, 15), el::Time{}, zone};
        el::io::printLine(
            el::StringFormat{"Application-level resolved offset seconds: {}\nDisplay abbreviation: {}"_el}.build(
                occurrence.timeOffset().toSeconds().toRawValue(), occurrence.timeZoneAbbreviation()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Stored name: Europe/Athens
    Id round trip: true
    Copied id: true
    Default id UTC: true
    Seconds: 0
    UTC: true
    Fixed: false
    Named: false
    DST: false
    Local: false
    UTC id: true
    Abbreviation index: 0
    Seconds: 19800
    UTC: false
    Fixed: true
    Named: false
    DST: false
    Local: true
    UTC id: true
    Abbreviation index: 0
    Application-level resolved offset seconds: 10800
    Display abbreviation: EEST

.. erbsland-demo-end::

:cpp:class:`tz::TimeOffset <erbsland::time::tz::TimeOffset>` is a lower-level detail value for an offset and its associated
metadata.
``offset()`` returns ``Seconds``, ``zoneId()`` identifies the associated named zone or UTC, and ``isDst()`` reports the
stored daylight-saving state.
``isUtc()``, ``isStaticOffset()``, and ``isZone()`` test the category; ``isLocalTime()`` reports origin.
``abbreviationId()`` is a transient abbreviation-table index rather than abbreviation text.
Its meaning also belongs to the same zone/database context.

The default constructor creates UTC details, and the fixed constructor accepts ``Seconds`` with an optional local-origin
flag.
These values have no named-zone or daylight-saving metadata.
The metadata constructor is documented in the :doc:`date and time reference <../../reference/time/date_and_time>` for
code that already has valid metadata; it does not resolve rules for a supplied offset.
In ordinary application code, obtain resolved offsets and abbreviation text through ``DateTime``.
``TimeZone`` provides no public standalone offset-at-date query.

Understanding the Bundled Rules
===============================

:cpp:func:`databaseVersion() <erbsland::time::TimeZone::databaseVersion>` identifies the bundled IANA rule version used by
named-zone resolution, including historical dates.
The rules travel with the library rather than being replaced by the operating system's zone database at conversion time.
The system setting chooses a local zone, while the bundled rules describe its behavior.

.. erbsland-demo::
    :source: time/TimeZone/Database.cpp
    :exec: time/time_zone --demo Database
    :source-sha256: 8a66e20d636501b06d58a5862cfe467e2080c811bdd4dba2f02b377feb6c5e17

.. code-block:: cpp

    /// Query the bundled rule version and supported names when offering time-zone choices.
    /// Database versions and zone lists are runtime results rather than permanent application constants.
    /// @notest{Compiled and executed documentation demo.}
    void database() {
        const auto names = el::TimeZone::names();
        el::io::printLine(
            el::StringFormat{"Bundled rule version: {}\nSupported names: {}"_el}.build(
                el::TimeZone::databaseVersion(), names.count().toRawValue()));
        const auto alias = el::TimeZone::fromNameOrThrow("US/Eastern"_el);
        el::io::printLine(el::StringFormat{"Alias US/Eastern has primary name: {}"_el}.build(alias.name()));
        const auto requested = "Europe/Athens"_el;
        el::io::printLine(el::StringFormat{"Requested name listed: {}"_el}.build(!names.findFirst(requested).isNoIndex()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Bundled rule version: 1.2026.2
    Supported names: 597
    Alias US/Eastern has primary name: America/New_York
    Requested name listed: true

.. erbsland-demo-end::

Time-zone rules can change, particularly for future civil dates.
A persisted local intention may therefore resolve differently when an application updates its bundled database.
Record the rule version when reproducing a conversion matters; the version identifies the required rules, but does not
by itself preserve a copy of them.
Keep that library and rule version available when exact reproduction is required.
Keep the original local fields and zone name when the intention must remain inspectable.
A stored UTC ``Timestamp`` preserves an already-selected instant, while a local date/time with a zone preserves the
information needed to resolve an appointment under a chosen set of rules.

The supported name list and version are queried values, not permanent counts or constants to copy from the demo output.
``names()`` helps you offer supported choices, and ``name()`` gives you the primary spelling after alias lookup.
:doc:`working_with_timestamps` covers persisting instants; :doc:`working_with_datetime` develops complete dated workflows
and conversions.
