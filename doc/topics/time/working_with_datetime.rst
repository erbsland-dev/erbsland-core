..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Time; DateTime Objects
    single: DateTime; Local Display and UTC Instants

.. _time-working-with-datetime:

****************************
Working with DateTime Values
****************************

A recorded instant and the clock reading you show a reader are related, but they are not interchangeable.
The same instant can fall on different calendar dates in different zones.
:cpp:class:`DateTime <erbsland::time::DateTime>` keeps the UTC date and time together with the information needed to display
them in a chosen zone or numeric offset.
You can resolve local input once, compare the resulting instant, and later choose a different display without changing
when the event happened.
This page explains those steps, external representations, and the distinction between elapsed and calendar changes.

Keeping an Instant and Its Display Together
===========================================

``DateTime`` supports the proleptic Gregorian calendar from year zero through year 9999 and retains nanosecond
fractions.
Its UTC bounds are ``0000-01-01T00:00:00Z`` and ``9999-12-31T23:59:59.999999999Z``.
The Core epoch is the first of these instants.
Days have 86,400 seconds and leap seconds are not represented.

The UTC fields give chronological comparisons a common basis, even when values have different display zones.
Local accessors apply the retained offset; named zones are resolved when constructing a value, converting its zone, or
calculating a new result.
This trades some work when preparing a local display for simple, direct comparisons between instants.
A fixed numeric offset describes one relationship to UTC, whereas a named zone also has historical and daylight-saving
rules for other dates.

When you only need to copy, compare, or store an instant, :doc:`working_with_timestamps` presents
:cpp:class:`Timestamp <erbsland::time::Timestamp>` without display metadata.
When you measure an operation or keep a deadline, :cpp:class:`TimePoint <erbsland::time::TimePoint>` uses a monotonic
clock and avoids wall-clock adjustments.
``DateTime::now()`` reads the civil clock; nanosecond storage does not imply that this clock measures each nanosecond.

Resolving Calendar Fields
=========================

Default construction creates an invalid ``DateTime``.
The two-argument ``DateTime(Date, Time)`` constructor instead interprets the fields as UTC.
The overloads taking ``Seconds`` or ``Duration`` as a third argument interpret the fields as local and subtract that
numeric UTC offset.
For example, a reading of ``14:30`` at ``+03:00`` identifies ``11:30`` UTC on the same date.
Offsets are normalized by complete 24-hour rotations into the supported range; this is not an input-validation API.

For a named zone, provide :cpp:class:`TimeZone <erbsland::time::TimeZone>` as the third argument.
It resolves the offset for the supplied local date and time using the bundled database.
The ``DateTime(Date, TimeWithZone)`` overload performs the same resolution when you already keep a wall-clock reading
and zone together in :cpp:class:`TimeWithZone <erbsland::time::TimeWithZone>`.
Both accept an optional :cpp:enum:`TimeOccurrenceInFold <erbsland::time::TimeOccurrenceInFold>` to select a repeated
reading; the default selects the first occurrence.
Copies and assignments keep both the instant and its display properties.

:cpp:func:`isValid() <erbsland::time::DateTime::isValid>` tells you whether the value has a valid date.
An invalid input date produces an invalid result, with the time and offset reset.
Validate calendar combinations before construction, as described in :doc:`working_with_dates`.
Near year zero or year 9999, resolve boundary-sensitive input through a checked ``Timestamp`` representation when you
need to reject UTC range overflow: ``DateTime`` offset construction uses saturating date adjustment.
For offset-bearing ISO input, parse with ``Timestamp::fromIsoStringOrThrow()`` and convert the checked result to
``DateTime`` before choosing its display zone.

:cpp:func:`now() <erbsland::time::DateTime::now>` captures the current UTC instant.
:cpp:func:`epoch() <erbsland::time::DateTime::epoch>` selects the Core epoch by default or a specified ``TimeEpoch``.
:cpp:func:`first() <erbsland::time::DateTime::first>` and
:cpp:func:`last() <erbsland::time::DateTime::last>` return the UTC boundaries.
The POSIX, Windows, and RFC 868 epochs are ``1970-01-01``, ``1601-01-01``, and ``1900-01-01`` at midnight UTC.

.. erbsland-demo::
    :source: time/DateTime/Create.cpp
    :exec: time/date_time --demo Create
    :source-sha256: e5ec131c2ccbe50c90cb69e887caa75155a4839352243b990662d52da7418583

.. code-block:: cpp

    /// Resolve UTC, fixed-offset, and named-zone calendar fields into dated instants.
    /// @notest{Compiled and executed documentation demo.}
    void create() {
        const auto lesson = "Μάθημα μαγείας"_el;
        const auto date = el::Date::fromYearMonthDayOrThrow(2026, 7, 1);
        const auto time = el::Time{el::Hour{14}, el::Minute{30}};
        const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
        const auto utc = el::DateTime{date, time};
        const auto fixedSeconds = el::DateTime{date, time, el::Seconds{10'800}};
        const auto fixedDuration = el::DateTime{date, time, el::Duration{el::Hours{3}}};
        const auto named = el::DateTime{date, time, zone};
        const auto zonedTime = el::DateTime{date, el::TimeWithZone{time, zone}};
        el::io::printLine(
            el::StringFormat{"Lesson: {}; UTC input: {}; named local input: {}"_el}.build(lesson, utc, named));
        el::io::printLine(
            el::StringFormat{"Fixed constructors agree: {}; zoned-time constructor agrees: {}"_el}.build(
                fixedSeconds == fixedDuration, named == zonedTime));
        el::io::printLine(
            el::StringFormat{"Default valid: {}; invalid date valid: {}; current clock valid: {}"_el}.build(
                el::DateTime{}.isValid(), el::DateTime{el::Date{}, time, zone}.isValid(), el::DateTime::now().isValid()));
        el::io::printLine(
            el::StringFormat{"Core epoch: {}; POSIX epoch: {}; first: {}; last: {}"_el}.build(
                el::DateTime::epoch(),
                el::DateTime::epoch(el::TimeEpoch::Posix),
                el::DateTime::first(),
                el::DateTime::last()));
    }

    }

.. erbsland-ansi::
    :escape-char: ␛

    Lesson: Μάθημα μαγείας; UTC input: 2026-07-01 14:30:00Z; named local input: 2026-07-01 14:
    30:00+03:00
    Fixed constructors agree: true; zoned-time constructor agrees: true
    Default valid: false; invalid date valid: false; current clock valid: true
    Core epoch: 0000-01-01 00:00:00Z; POSIX epoch: 1970-01-01 00:00:00Z; first: 0000-01-01 00:
    00:00Z; last: 9999-12-31 23:59:59.999999999Z

.. erbsland-demo-end::

Reading Local and UTC Fields
============================

The accessors :cpp:func:`utcDate() <erbsland::time::DateTime::utcDate>` and
:cpp:func:`utcTime() <erbsland::time::DateTime::utcTime>` return the UTC fields.
:cpp:func:`date() <erbsland::time::DateTime::date>` and :cpp:func:`time() <erbsland::time::DateTime::time>` return the local
fields for display.
A local reading just after midnight can still have the preceding UTC date, so keep the date and time from the same view
together.

The individual :cpp:func:`year() <erbsland::time::DateTime::year>`,
:cpp:func:`month() <erbsland::time::DateTime::month>`, :cpp:func:`day() <erbsland::time::DateTime::day>`,
:cpp:func:`dayOfYear() <erbsland::time::DateTime::dayOfYear>`, and
:cpp:func:`dayOfWeek() <erbsland::time::DateTime::dayOfWeek>` accessors describe the local date.
Likewise, :cpp:func:`hour() <erbsland::time::DateTime::hour>`, :cpp:func:`minute() <erbsland::time::DateTime::minute>`,
and :cpp:func:`second() <erbsland::time::DateTime::second>` describe the local time.
:cpp:func:`nanosecondFraction() <erbsland::time::DateTime::nanosecondFraction>` returns the full fraction of its current second, while :cpp:func:`millisecondFraction() <erbsland::time::DateTime::millisecondFraction>` truncates that
fraction to whole milliseconds.
Neither fraction accessor is a total count since an epoch.

:cpp:func:`parts() <erbsland::time::DateTime::parts>` returns a
:cpp:struct:`DateTimeParts <erbsland::time::DateTimeParts>` with year, month, day, hour, minute, second, and nanosecond fraction.
When preparing several fields for a display, this groups extraction into one operation and avoids repeating calendar
extraction for each date component.
The fields are local; reconstructing the same instant also requires its offset or zone and, during a fold, its
occurrence.

:cpp:func:`timeOffset() <erbsland::time::DateTime::timeOffset>` is the resolved numeric offset as a ``Duration``.
:cpp:func:`timeZone() <erbsland::time::DateTime::timeZone>` returns the retained named zone, but returns UTC for a plain
fixed-offset value.
Read :cpp:func:`timeOffset() <erbsland::time::DateTime::timeOffset>` to recover a fixed numeric offset, rather than
assuming :cpp:func:`timeZone() <erbsland::time::DateTime::timeZone>` reconstructs it.
:cpp:func:`timeZoneAbbreviation() <erbsland::time::DateTime::timeZoneAbbreviation>` supplies the current display abbreviation.
Check validity before interpreting any components: invalid values expose fallback fields rather than an event date.

.. erbsland-demo::
    :source: time/DateTime/Inspect.cpp
    :exec: time/date_time --demo Inspect
    :source-sha256: 0e248efc85b798867b4738996d2cbc9f88cc8674d000fed64f314b7f0655492b

.. code-block:: cpp

    /// Read local fields together and distinguish them from the stored UTC fields.
    /// @notest{Compiled and executed documentation demo.}
    void inspect() {
        const auto reading = el::DateTime::fromIsoStringOrThrow("2026-07-01T00:30:12.123456789+03:00"_el)
                                 .toTimeZone(el::TimeZone::fromNameOrThrow("Europe/Athens"_el));
        const auto parts = reading.parts();
        el::io::printLine(
            el::StringFormat{"Local: {} {}; UTC: {} {}"_el}.build(
                reading.date(), reading.time(), reading.utcDate(), reading.utcTime()));
        el::io::printLine(
            el::StringFormat{"Parts: {}-{}-{} {}:{}:{}; fraction: {} ns"_el}.build(
                parts.year.toValue(),
                parts.month.toValue(),
                parts.day.toValue(),
                parts.hour.toValue(),
                parts.minute.toValue(),
                parts.second.toValue(),
                parts.nanosecondFraction.toRawValue()));
        el::io::printLine(
            el::StringFormat{"Individual fields: {}-{}-{} {}:{}:{}; day of year: {}; weekday: {}"_el}.build(
                reading.year().toValue(),
                reading.month().toValue(),
                reading.day().toValue(),
                reading.hour().toValue(),
                reading.minute().toValue(),
                reading.second().toValue(),
                reading.dayOfYear().toValue(),
                reading.dayOfWeek().toValue()));
        el::io::printLine(
            el::StringFormat{
                "Millisecond fraction: {}; nanosecond fraction: {}; offset (seconds): {}; zone: {}; abbreviation: {}"_el}
                .build(
                    reading.millisecondFraction().toRawValue(),
                    reading.nanosecondFraction().toRawValue(),
                    reading.timeOffset().toSeconds().toRawValue(),
                    reading.timeZone().name(),
                    reading.timeZoneAbbreviation()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Local: 2026-07-01 00:30:12.123456789; UTC: 2026-06-30 21:30:12.123456789
    Parts: 2026-7-1 0:30:12; fraction: 123456789 ns
    Individual fields: 2026-7-1 0:30:12; day of year: 182; weekday: 2
    Millisecond fraction: 123; nanosecond fraction: 123456789; offset (seconds): 10800; zone:
    Europe/Athens; abbreviation: EEST

.. erbsland-demo-end::

Comparing Instants and Comparing Complete Values
================================================

Chronological ordering uses UTC date and time, without converting either value to local fields.
It therefore remains straightforward even when two values have different offsets or named zones.
The default invalid value sorts before every valid instant.

Equality has a broader meaning: ``operator==`` also compares retained display metadata.
Two values can have the same chronological ordering position and still be unequal because one displays UTC and the other
displays a local offset.
For instant equality, compare ``a.toUtc() == b.toUtc()`` or convert both values to ``Timestamp``.
For preserving a complete display value, ordinary equality retains the useful distinction between those views.
Invalid values have reset display metadata and compare equal to one another.

.. erbsland-demo::
    :source: time/DateTime/Compare.cpp
    :exec: time/date_time --demo Compare
    :source-sha256: 4025fb2c185a76db6d7f8286b15e141547c2e51c180f52a03ab8d2b750acda1e

.. code-block:: cpp

    /// Distinguish chronological ordering from equality that retains display metadata.
    /// @notest{Compiled and executed documentation demo.}
    void compare() {
        const auto local = el::DateTime::fromIsoStringOrThrow("2026-07-01T14:30:00+03:00"_el);
        const auto utc = local.toUtc();
        el::io::printLine(el::StringFormat{"Local: {}; UTC: {}"_el}.build(local, utc));
        el::io::printLine(
            el::StringFormat{"Same ordering position: {}; same object value: {}; equal after UTC conversion: {}"_el}.build(
                (local <=> utc) == std::strong_ordering::equal, local == utc, local.toUtc() == utc));
        const auto later = utc.addedOrThrow(el::Duration{el::Seconds{1}});
        el::io::printLine(
            el::StringFormat{"Local instant before later UTC: {}; invalid sorts first: {}; invalid values equal: {}"_el}
                .build(local < later, el::DateTime{} < utc, el::DateTime{} == el::DateTime{}));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Local: 2026-07-01 14:30:00+03:00; UTC: 2026-07-01 11:30:00Z
    Same ordering position: true; same object value: false; equal after UTC conversion: true
    Local instant before later UTC: true; invalid sorts first: true; invalid values equal: tru
    e

.. erbsland-demo-end::

Changing the Display and Resolving Clock Transitions
====================================================

:cpp:func:`toUtc() <erbsland::time::DateTime::toUtc>` returns the same instant displayed in UTC.
:cpp:func:`toTimeZone() <erbsland::time::DateTime::toTimeZone>` returns it in a chosen zone, resolving the appropriate
offset at that instant.
Both leave the original unchanged and preserve invalidity.
``TimeZone::fromName()`` returns no value for an unknown name; ``fromNameOrThrow()`` reports ``ParseError``.
:doc:`working_with_time_zones` covers UTC, fixed, named, and system-local
zones.

When a clock moves backward, a local reading in the repeated interval is called a *fold*.
The ``First`` and ``Second`` occurrence choices identify different UTC instants with identical local fields.
A numeric offset makes the selected instant explicit when you serialize it, but does not retain the name and future
rules of its zone.

When a clock moves forward, readings in the skipped interval form a *gap*.
The constructor chooses an offset using the database transition rule; it does not reject a gap as an invalid date.
The retained constructor display can still show the requested fields.
Re-resolve the chosen instant with ``value.toUtc().toTimeZone(zone)`` and compare its local date and time with the input
when you need to detect that the wall-clock reading does not exist.
The gap demo shows why the extra resolution matters: its constructor display reads ``03:30+03:00``, but the selected
instant displays as ``02:30+02:00`` when the zone rules are resolved again.
:cpp:func:`isValid() <erbsland::time::DateTime::isValid>` alone answers a different question and cannot detect a gap.

.. erbsland-demo::
    :source: time/DateTime/Zones.cpp
    :exec: time/date_time --demo Zones
    :source-sha256: 7741498a8892680f6813af65187cacee956dfdd5c43265ea7541fd6f1c0dfa06

.. code-block:: cpp

    /// Resolve repeated or missing local readings and refresh named-zone display metadata.
    /// @notest{Compiled and executed documentation demo.}
    void zones() {
        const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
        const auto foldDate = el::Date::fromYearMonthDayOrThrow(2026, 10, 25);
        const auto reading = el::Time{el::Hour{3}, el::Minute{30}};
        const auto first = el::DateTime{foldDate, reading, zone, el::TimeOccurrenceInFold::First};
        const auto second = el::DateTime{foldDate, reading, zone, el::TimeOccurrenceInFold::Second};
        el::io::printLine(
            el::StringFormat{"First occurrence: {}; second occurrence: {}; apart (seconds): {}"_el}.build(
                first, second, first.durationTo(second).toSeconds().toRawValue()));

        // Re-resolve the selected UTC instant to check a requested wall-clock reading.
        const auto gapDate = el::Date::fromYearMonthDayOrThrow(2026, 3, 29);
        const auto gap = el::DateTime{gapDate, reading, zone};
        const auto resolved = gap.toUtc().toTimeZone(zone);
        el::io::printLine(
            el::StringFormat{"Gap constructor display: {}; refreshed display: {}; valid: {}"_el}.build(
                gap, resolved, gap.isValid()));
        el::io::printLine(
            el::StringFormat{"Requested local fields retained after resolution: {}"_el}.build(
                resolved.date() == gapDate && resolved.time() == reading));
        el::io::printLine(
            el::StringFormat{"Unknown zone accepted: {}; invalid remains invalid on zone conversion: {}"_el}.build(
                el::TimeZone::fromName("Unknown/Academy"_el).has_value(), !el::DateTime{}.toTimeZone(zone).isValid()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    First occurrence: 2026-10-25 03:30:00+03:00; second occurrence: 2026-10-25 03:30:00+02:00;
     apart (seconds): 3600
    Gap constructor display: 2026-03-29 03:30:00+03:00; refreshed display: 2026-03-29 02:30:00
    +02:00; valid: true
    Requested local fields retained after resolution: false
    Unknown zone accepted: false; invalid remains invalid on zone conversion: true

.. erbsland-demo-end::

Reading and Writing ISO Text
============================

:cpp:func:`fromIsoString() <erbsland::time::DateTime::fromIsoString>` without a zone argument interprets an explicit
numeric offset when present, and treats text without an offset as UTC.
The overload taking a ``TimeZone`` instead expects local text without an offset and resolves it in that zone.
It rejects text that supplies both an offset and a separate zone argument.
Choose the overload from the meaning of the input, rather than from the zone you eventually want to display.

Parsing returns an invalid value on failure;
:cpp:func:`fromIsoStringOrThrow() <erbsland::time::DateTime::fromIsoStringOrThrow>` reports ``ParseError``.
The ``requiredPrecision`` argument sets the minimum precision the text must contain, with seconds required by default.
Requiring milliseconds rejects whole-second input, rather than supplying an invented fractional reading.
The parser accepts the library's ISO subset, so validate inputs instead of assuming every ISO 8601 variant is supported.

:cpp:func:`toString() <erbsland::time::DateTime::toString>` gives a compact readable date and time with the resolved offset.
A nonzero fraction uses a dot and omits trailing zeros.
For system-local-origin values, that compact display omits the offset.
:cpp:func:`toIsoString() <erbsland::time::DateTime::toIsoString>` takes ``IsoTimeFormatFlags`` and ``DateTimePrecision``.
Its default stops at whole seconds and omits the offset, even when the object retains both.
That default is convenient for a short display, but parsing it without a supplied zone treats the reading as UTC and can
change its meaning.
ISO fractions use a comma by default.
For precise exchange, explicitly request nanoseconds, a dot if desired, and the ``TimeShift`` flag so the resolved
offset is included even for a system-local display.
Both output methods return an empty string for an invalid value.

A numeric ISO offset preserves the instant when parsed again, but cannot restore named-zone metadata.
Store the zone name separately if your format needs that information, then apply
:cpp:func:`toTimeZone() <erbsland::time::DateTime::toTimeZone>` after parsing.
See :ref:`time-iso-output` for the shared formatting controls and
:doc:`working_with_timestamps` for canonical UTC exchange text.

.. erbsland-demo::
    :source: time/DateTime/Text.cpp
    :exec: time/date_time --demo Text
    :source-sha256: 4e5b4583d83c80332a20dad2722a74122e1c4b2a431c069c3c6b0f454bcf3f99

.. code-block:: cpp

    /// Parse ISO calendar text and choose fractional precision and offset output explicitly.
    /// @notest{Compiled and executed documentation demo.}
    void text() {
        const auto reading = el::DateTime::fromIsoStringOrThrow("2026-07-01T14:30:00.123456789+03:00"_el);
        const auto format = el::IsoTimeFormatFlags{
            el::IsoTimeFormat::Extended,
            el::IsoTimeFormat::TimePrefix,
            el::IsoTimeFormat::TimeShift,
            el::IsoTimeFormat::UseDotFraction};
        el::io::printLine(
            el::StringFormat{"Display: {}; default ISO: {}; precise ISO: {}"_el}.build(
                reading.toString(), reading.toIsoString(), reading.toIsoString(format, el::DateTimePrecision::Nanosecond)));
        el::io::printLine(
            el::StringFormat{"Default ISO preserves the instant when parsed as UTC: {}"_el}.build(
                el::DateTime::fromIsoStringOrThrow(reading.toIsoString()).toUtc() == reading.toUtc()));
        const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
        const auto local = el::DateTime::fromIsoStringOrThrow("2026-07-01T14:30:00"_el, zone);
        el::io::printLine(
            el::StringFormat{"Local text with a zone: {}; bare text interpreted as UTC: {}"_el}.build(
                local, el::DateTime::fromIsoStringOrThrow("2026-07-01T14:30:00"_el).isUtc()));
        el::io::printLine(
            el::StringFormat{
                "Offset text with explicit zone accepted: {}; insufficient precision valid: {}; invalid output empty: {}"_el}
                .build(
                    el::DateTime::fromIsoString("2026-07-01T14:30:00+03:00"_el, zone).isValid(),
                    el::DateTime::fromIsoString("2026-07-01T14:30:00"_el, el::DateTimePrecision::Millisecond).isValid(),
                    el::DateTime{}.toIsoString().isEmpty()));
        try {
            const auto unexpected = el::DateTime::fromIsoStringOrThrow("2026-02-30T14:30:00Z"_el);
            el::io::printLine(el::StringFormat{"Unexpected successful result: {}"_el}.build(unexpected.toString()));
        } catch (const el::err::ParseError &) {
            el::io::printLine("Checked parsing rejects an impossible date."_el);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Display: 2026-07-01 14:30:00.123456789+03:00; default ISO: 2026-07-01 14:30:00; precise IS
    O: 2026-07-01T14:30:00.123456789+03:00
    Default ISO preserves the instant when parsed as UTC: false
    Local text with a zone: 2026-07-01 14:30:00+03:00; bare text interpreted as UTC: true
    Offset text with explicit zone accepted: false; insufficient precision valid: false; inval
    id output empty: true
    Checked parsing rejects an impossible date.

.. erbsland-demo-end::

Choosing an Epoch and an External Number Format
===============================================

The ``toTicks<T>()`` and :cpp:func:`fromTicks() <erbsland::time::DateTime::fromTicks>` templates support seconds,
milliseconds, microseconds, and nanoseconds relative to ``TimeEpoch``.
:cpp:func:`toSeconds() <erbsland::time::DateTime::toSeconds>` and :cpp:func:`toNanoseconds() <erbsland::time::DateTime::toNanoseconds>` are convenient output aliases; :cpp:func:`fromSeconds() <erbsland::time::DateTime::fromSeconds>` and :cpp:func:`fromNanoseconds() <erbsland::time::DateTime::fromNanoseconds>` are
matching input aliases.
They default to the Core epoch, so select ``TimeEpoch::Posix`` explicitly for Unix-style counts.
Tick construction creates a UTC ``DateTime``; apply a display zone afterward when needed.

Unlike ``Timestamp``, these APIs require nonnegative epoch-relative counts.
Output fails when the instant precedes the selected epoch or when its total count exceeds the chosen amount's range.
Input fails for negative ticks or a result outside the calendar.
The optional forms return no value, and ``...OrThrow()`` forms report ``OutOfRangeError``.
Selecting a coarser output unit discards the smaller fractional digits; the checked form rejects range errors, not this
intentional loss of precision.
For example, the millisecond reconstruction below keeps ``.123`` and discards the final six nanosecond digits.
Choose the split representation when those digits are part of the value you must preserve.
Nanoseconds measured from year zero cannot reach a modern date, even though the ``DateTime`` itself is valid.

:cpp:func:`toSecondsAndFractions() <erbsland::time::DateTime::toSecondsAndFractions>` avoids that scalar range limitation.
It returns complete seconds and a nanosecond fraction, retaining the full precision and calendar range for instants at
or after the chosen epoch.
The ``fromTicks(Seconds, Nanoseconds, epoch)`` overload reconstructs it and requires a fraction in ``0..999999999``.
Its optional form returns no value for invalid input; the checked form raises ``OutOfRangeError``.
The split output likewise has an ``OrThrow`` variant.
For a signed pre-epoch representation, convert to ``Timestamp`` instead.

:cpp:func:`fromTimeT() <erbsland::time::DateTime::fromTimeT>` reads native POSIX whole seconds and returns an invalid value
outside the calendar range.
:cpp:func:`toTimeT() <erbsland::time::DateTime::toTimeT>` discards fractions and returns minus one for invalid values,
pre-POSIX instants, or counts outside the native range.
The native range depends on the platform; prefer optional typed ticks when you need an explicit conversion result.
For raw-field and portable binary exchange, convert through ``Timestamp`` as described in its topic.

.. erbsland-demo::
    :source: time/DateTime/Ticks.cpp
    :exec: time/date_time --demo Ticks
    :source-sha256: 8633648786c52026e0e95340348a22e2326337f5928cab1eb9465e658005cfd6

.. code-block:: cpp

    /// Choose an epoch and a representation that can retain the full calendar range and precision.
    /// @notest{Compiled and executed documentation demo.}
    void ticks() {
        const auto reading = el::DateTime::fromIsoStringOrThrow("2026-07-01T14:30:00.123456789+03:00"_el);
        const auto [seconds, fraction] = reading.toSecondsAndFractionsOrThrow(el::TimeEpoch::Posix);
        const auto restored = el::DateTime::fromTicksOrThrow(seconds, fraction, el::TimeEpoch::Posix);
        el::io::printLine(
            el::StringFormat{"POSIX seconds: {}; fraction: {}; full precision round trip: {}"_el}.build(
                seconds.toRawValue(), fraction.toRawValue(), restored == reading.toUtc()));
        el::io::printLine(
            el::StringFormat{"Seconds: {}; milliseconds: {}; microseconds: {}; nanoseconds: {}"_el}.build(
                reading.toSecondsOrThrow(el::TimeEpoch::Posix).toRawValue(),
                reading.toTicksOrThrow<el::Milliseconds>(el::TimeEpoch::Posix).toRawValue(),
                reading.toTicksOrThrow<el::Microseconds>(el::TimeEpoch::Posix).toRawValue(),
                reading.toNanosecondsOrThrow(el::TimeEpoch::Posix).toRawValue()));
        const auto nanos = reading.toNanosecondsOrThrow(el::TimeEpoch::Posix);
        el::io::printLine(
            el::StringFormat{"Nanosecond alias round trip: {}; invalid split fraction accepted: {}"_el}.build(
                el::DateTime::fromNanosecondsOrThrow(nanos, el::TimeEpoch::Posix) == reading.toUtc(),
                el::DateTime::fromTicks(el::Seconds{}, el::Nanoseconds{1'000'000'000}).has_value()));
        const auto millis = reading.toTicksOrThrow<el::Milliseconds>(el::TimeEpoch::Posix);
        el::io::printLine(
            el::StringFormat{"Millisecond reconstruction: {}"_el}.build(
                el::DateTime::fromTicksOrThrow(millis, el::TimeEpoch::Posix)));
        el::io::printLine(
            el::StringFormat{
                "Modern date fits Core nanoseconds: {}; Core epoch fits POSIX seconds: {}; negative ticks accepted: {}"_el}
                .build(
                    reading.toNanoseconds().has_value(),
                    el::DateTime::epoch().toSeconds(el::TimeEpoch::Posix).has_value(),
                    el::DateTime::fromSeconds(el::Seconds{-1}, el::TimeEpoch::Posix).has_value()));
        try {
            const auto unexpected = reading.toNanosecondsOrThrow();
            el::io::printLine(el::StringFormat{"Unexpected successful result: {}"_el}.build(unexpected.toRawValue()));
        } catch (const el::err::OutOfRangeError &) {
            el::io::printLine("Checked conversion rejects a nanosecond total that does not fit."_el);
        }
        const auto native = reading.toTimeT();
        el::io::printLine(
            el::StringFormat{"Native whole-second reconstruction: {}; pre-POSIX native output: {}"_el}.build(
                el::DateTime::fromTimeT(native), el::DateTime::first().toTimeT()));
    }

    }

.. erbsland-ansi::
    :escape-char: ␛

    POSIX seconds: 1782905400; fraction: 123456789; full precision round trip: true
    Seconds: 1782905400; milliseconds: 1782905400123; microseconds: 1782905400123456; nanoseco
    nds: 1782905400123456789
    Nanosecond alias round trip: true; invalid split fraction accepted: false
    Millisecond reconstruction: 2026-07-01 11:30:00.123Z
    Modern date fits Core nanoseconds: false; Core epoch fits POSIX seconds: false; negative t
    icks accepted: false
    Checked conversion rejects a nanosecond total that does not fit.
    Native whole-second reconstruction: 2026-07-01 11:30:00Z; pre-POSIX native output: -1

.. erbsland-demo-end::

Applying Elapsed and Calendar Changes
=====================================

A fixed interval and a calendar change answer different questions.
``Duration{Hours{24}}`` moves an instant by 86,400 seconds.
``CalendarDelta{Months{1}}`` moves its calendar month, with the result depending on the starting date.
Both are applied to the UTC representation, retaining a fixed display offset or refreshing a named zone for the result.
Neither promises to preserve the same local clock reading across a daylight-saving transition.

:cpp:func:`added() <erbsland::time::DateTime::added>` and :cpp:func:`subtracted() <erbsland::time::DateTime::subtracted>` return changed copies; :cpp:func:`add() <erbsland::time::DateTime::add>` and :cpp:func:`subtract() <erbsland::time::DateTime::subtract>` change the value in place.
Both interval types also work with ``+``, ``-``, ``+=``, and ``-=``.
These operations saturate at the UTC calendar bounds.
:cpp:func:`wouldAddSaturate() <erbsland::time::DateTime::wouldAddSaturate>` and :cpp:func:`wouldSubtractSaturate() <erbsland::time::DateTime::wouldSubtractSaturate>` test for range overflow before applying the change.
The corresponding ``...OrThrow()`` methods raise ``OverflowError`` and leave an in-place target unchanged on failure.
Invalid inputs remain invalid, and their overflow predicates return false.

Calendar components are applied from smallest to largest: nanoseconds, microseconds, milliseconds, seconds, minutes,
hours, days, weeks, months, and years.
Month and year steps clamp a day that does not exist in the destination month, just as in :doc:`working_with_dates`.
Moving January 31 forward a month and back a month can therefore finish on January 29 in a leap year.
Checked calendar application rejects overflow at an intermediate step, even if a later component would move back into
range.
The ``CalendarDelta`` reference in :doc:`../../reference/time/date_and_time` gives the complete application contract.

For fractional shifts, supply a ``CalendarDelta`` containing nanoseconds, microseconds, or milliseconds.
The ``Duration`` overload has whole-second resolution.
For distances, ``a.durationTo(b)`` and ``b - a`` subtract the two whole-second epoch readings.
``a.timeDeltaTo(b)`` wraps that whole-second result in a ``TimeDelta``; it does not recover the fractional difference.
Validate both operands before calling these ``DateTime`` distance methods.
For precise signed distances, checked invalid handling, and explicit distance overflow checks, convert to ``Timestamp``
and use its ``...ToOrThrow()`` APIs.

.. erbsland-demo::
    :source: time/DateTime/Calculate.cpp
    :exec: time/date_time --demo Calculate
    :source-sha256: c5b3942a36121cb2e3514d3d67cfc298ad6a4a7e4fe176c80589ce1143e04520

.. code-block:: cpp

    /// Apply fixed or calendar changes in UTC and detect boundaries before updating an instant.
    /// @notest{Compiled and executed documentation demo.}
    void calculate() {
        const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
        const auto start =
            el::DateTime{el::Date::fromYearMonthDayOrThrow(2026, 3, 28), el::Time{el::Hour{12}, el::Minute{0}}, zone};
        const auto next = start.addedOrThrow(el::Duration{el::Hours{24}});
        el::io::printLine(
            el::StringFormat{"Start: {}; 24 hours later: {}; elapsed (seconds): {}"_el}.build(
                start, next, start.durationTo(next).toSeconds().toRawValue()));
        auto adjusted = next;
        adjusted.subtractOrThrow(el::Duration{el::Hours{24}});
        adjusted.add(el::Duration{el::Seconds{1}});
        adjusted.subtract(el::Duration{el::Seconds{1}});
        el::io::printLine(el::StringFormat{"In-place round trip: {}"_el}.build(adjusted == start));
        const auto monthEnd = el::DateTime::fromIsoStringOrThrow("2028-01-31T12:00:00Z"_el);
        const auto shifted = monthEnd.addedOrThrow(el::CalendarDelta{el::Months{1}});
        el::io::printLine(
            el::StringFormat{"One month later: {}; one month back: {}"_el}.build(
                shifted, shifted.subtractedOrThrow(el::CalendarDelta{el::Months{1}})));
        const auto precise = monthEnd.addedOrThrow(el::CalendarDelta{el::Nanoseconds{250'000'000}});
        el::io::printLine(
            el::StringFormat{"Fractional shift: {}; DateTime delta: {}; Timestamp delta: {}"_el}.build(
                precise,
                monthEnd.timeDeltaTo(precise),
                el::Timestamp::fromDateTimeOrThrow(monthEnd).timeDeltaToOrThrow(
                    el::Timestamp::fromDateTimeOrThrow(precise))));
        el::io::printLine(
            el::StringFormat{"Last plus second saturates: {}; first minus second saturates: {}; clamped: {}"_el}.build(
                el::DateTime::last().wouldAddSaturate(el::Duration{el::Seconds{1}}),
                el::DateTime::first().wouldSubtractSaturate(el::Duration{el::Seconds{1}}),
                el::DateTime::last().added(el::Duration{el::Seconds{1}}) == el::DateTime::last()));
        try {
            const auto unexpected = el::DateTime::last().addedOrThrow(el::CalendarDelta{el::Nanoseconds{1}});
            el::io::printLine(el::StringFormat{"Unexpected successful result: {}"_el}.build(unexpected.toString()));
        } catch (const el::err::OverflowError &) {
            el::io::printLine("Checked calendar arithmetic rejects a boundary crossing."_el);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Start: 2026-03-28 12:00:00+02:00; 24 hours later: 2026-03-29 13:00:00+03:00; elapsed (seco
    nds): 86400
    In-place round trip: true
    One month later: 2028-02-29 12:00:00Z; one month back: 2028-01-29 12:00:00Z
    Fractional shift: 2028-01-31 12:00:00.25Z; DateTime delta: 0 s; Timestamp delta: 250 ms
    Last plus second saturates: true; first minus second saturates: true; clamped: true
    Checked calendar arithmetic rejects a boundary crossing.

.. erbsland-demo-end::

Keeping a Recurring Local Time
==============================

If your requirement is the same local clock reading on another date, carry the intended ``TimeWithZone`` separately.
Advance the local ``Date``, then resolve a new ``DateTime`` from that date and zoned reading, choosing a fold occurrence
and checking gap resolution as appropriate.
Adding a fixed day to yesterday's instant answers an elapsed-time question instead.
The following example resolves noon on each side of a spring transition.
Both occurrences display noon, while the elapsed interval is only 23 hours.
Resolving each local occurrence makes that difference explicit without changing the intended daily reading.

.. erbsland-demo::
    :source: time/DateTime/Recurrence.cpp
    :exec: time/date_time --demo Recurrence
    :source-sha256: 61a5ff6ffe7d1dbb2fa19547224bede6c40a71fa0d4f2db768ddd101ec4645ee

.. code-block:: cpp

    /// Resolve each occurrence from its intended local date, time, and zone.
    /// @notest{Compiled and executed documentation demo.}
    void recurrence() {
        const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
        const auto lessonTime = el::TimeWithZone{el::Time{el::Hour{12}, el::Minute{0}}, zone};
        const auto date = el::Date::fromYearMonthDayOrThrow(2026, 3, 28);
        const auto today = el::DateTime{date, lessonTime};
        const auto tomorrow = el::DateTime{date.addedOrThrow(el::Days{1}), lessonTime};
        el::io::printLine(
            el::StringFormat{"Today: {}; next local occurrence: {}; elapsed (seconds): {}"_el}.build(
                today, tomorrow, today.durationTo(tomorrow).toSeconds().toRawValue()));
        el::io::printLine(
            el::StringFormat{"Adding 24 hours instead: {}"_el}.build(today.addedOrThrow(el::Duration{el::Hours{24}})));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Today: 2026-03-28 12:00:00+02:00; next local occurrence: 2026-03-29 12:00:00+03:00; elapse
    d (seconds): 82800
    Adding 24 hours instead: 2026-03-29 13:00:00+03:00

.. erbsland-demo-end::

The :doc:`overview` introduces ``TimeWithZone`` and calendar amounts; :doc:`working_with_dates` explains date
navigation.
The :doc:`date and time reference <../../reference/time/date_and_time>` lists all ``DateTime`` overloads and related
types.
