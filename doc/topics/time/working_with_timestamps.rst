..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Time; Timestamps
    single: Timestamp; Recording and Exchanging Instants

.. _time-working-with-timestamps:

***************************
Working with UTC Timestamps
***************************

When you record when an event happened, you usually need one unambiguous instant that you can compare, store, and send
to another application.
A local clock reading alone cannot do that: the same reading can describe different instants in different zones, and
sometimes even two instants in the same zone.
:cpp:class:`Timestamp <erbsland::time::Timestamp>` keeps the UTC instant with nanosecond precision and leaves the choice of
display zone to the code that presents it.
This page develops construction, text and binary exchange, epoch conversions, and calculations with those instants.

Choosing an Instant Type
========================

``Timestamp`` covers the proleptic Gregorian calendar from ``0000-01-01T00:00:00.000000000Z`` through
``9999-12-31T23:59:59.999999999Z``.
Year zero is part of this calendar, and its first midnight is the Core epoch.
Each day has 86,400 seconds; leap seconds are not represented.
The stored precision is one nanosecond throughout this range, even though the clock used to capture an event may have a
coarser resolution.

Choose ``Timestamp`` when the instant is the value you want to retain.
:cpp:class:`DateTime <erbsland::time::DateTime>` represents the same calendar range and precision while also retaining
information for local display and calendar calculations.
:doc:`working_with_datetime` explains that model.
For elapsed measurements and deadlines within a running program,
:cpp:class:`TimePoint <erbsland::time::TimePoint>` instead uses a monotonic clock.
A system wall clock can be adjusted, so subtracting recorded timestamps does not provide the same measurement guarantee.

Constructing and Checking an Instant
====================================

The ``Timestamp(Date, Time)`` constructor interprets both fields as UTC.
Create and validate the :cpp:class:`Date <erbsland::time::Date>` first; an invalid date gives an invalid timestamp.
The :cpp:class:`Time <erbsland::time::Time>` supplies the time within that UTC day, including its fraction.
Default construction creates an invalid value, rather than capturing the clock or choosing an epoch.
Copies and assignments retain the instant, with no display metadata to manage.

:cpp:func:`now() <erbsland::time::Timestamp::now>` captures the system wall clock.
Read it at the point where the event occurs; repeated calls can return equal values or move backward after a clock
adjustment.
A separate sequence number is appropriate when you also need a strict event order.
:cpp:func:`epoch() <erbsland::time::Timestamp::epoch>` defaults to the Core epoch and accepts a
:cpp:enum:`TimeEpoch <erbsland::time::TimeEpoch>` for POSIX, Windows, or RFC 868 origins.
:cpp:func:`first() <erbsland::time::Timestamp::first>` equals the Core epoch, while
:cpp:func:`last() <erbsland::time::Timestamp::last>` includes the last nanosecond of year 9999.

When an external source supplies a day count and a time-of-day count,
:cpp:func:`fromDaysAndNanoseconds() <erbsland::time::Timestamp::fromDaysAndNanoseconds>` checks them together.
Days are counted from the Core epoch and must lie in ``0..3652424``; nanoseconds since midnight must lie in
``0..86399999999999``.
An oversized nonnegative field returns ``std::nullopt``.
:cpp:func:`fromDaysAndNanosecondsOrThrow() <erbsland::time::Timestamp::fromDaysAndNanosecondsOrThrow>` reports that failure
with :cpp:class:`ParameterError <erbsland::err::ParameterError>`.
Neither form carries an oversized time field into the next day.

Negative input fields have a different meaning: they produce a successfully decoded, canonical invalid timestamp.
This distinction lets an exchange format preserve “no recorded instant” without treating it as a corrupt field.
An optional containing a value therefore does not always contain a valid instant.
Check :cpp:func:`isValid() <erbsland::time::Timestamp::isValid>` when your application requires one.
All invalid timestamps compare equal and sort before valid timestamps; valid ones compare by their UTC instant.
The same ordering works across midnight and across years without converting to a scalar nanosecond count.
In the following demo, the negative day count successfully decodes that sentinel, while a full day of nanoseconds is
rejected because it no longer describes a reading within one day.

.. erbsland-demo::
    :source: time/Timestamp/Create.cpp
    :exec: time/timestamp --demo Create
    :source-sha256: 72b76d8ac964f25af216175bc71d19c7bedbb9591e0683b99cefeffd1a6c804e

.. code-block:: cpp

    /// Construct, validate, and compare UTC instants for recorded events.
    /// @notest{Compiled and executed documentation demo.}
    void create() {
        const auto eventName = "Μάθημα αστρονομίας"_el;
        const auto date = el::Date::fromYearMonthDayOrThrow(2026, 7, 1);
        const auto start = el::Timestamp{date, el::Time{el::Hour{12}, el::Minute{30}}};
        const auto restored = el::Timestamp::fromDaysAndNanosecondsOrThrow(start.dateAsDays(), start.timeAsNanoseconds());
        el::io::printLine(el::StringFormat{"Event: {}; start: {}; restored: {}"_el}.build(eventName, start, restored));

        // A successfully decoded invalid sentinel is different from malformed input.
        const auto invalid = el::Timestamp::fromDaysAndNanoseconds(el::Days{-1}, el::Nanoseconds{});
        const auto malformed = el::Timestamp::fromDaysAndNanoseconds(el::Days{}, el::Nanoseconds{86'400'000'000'000});
        el::io::printLine(
            el::StringFormat{"Sentinel decoded: {}; sentinel valid: {}; oversized input decoded: {}"_el}.build(
                invalid.has_value(), invalid->isValid(), malformed.has_value()));
        el::io::printLine(
            el::StringFormat{"Default valid: {}; invalid sorts first: {}; invalid values equal: {}"_el}.build(
                el::Timestamp{}.isValid(), el::Timestamp{} < start, el::Timestamp{} == *invalid));
        el::io::printLine(
            el::StringFormat{"Epoch: {}; first: {}; last: {}"_el}.build(
                el::Timestamp::epoch(), el::Timestamp::first(), el::Timestamp::last()));
        // Avoid capturing a changing clock reading in the documentation output.
        el::io::printLine(el::StringFormat{"Current wall clock is valid: {}"_el}.build(el::Timestamp::now().isValid()));
        try {
            const auto unexpected = el::Timestamp::fromDaysAndNanosecondsOrThrow(el::Days{3'652'425}, el::Nanoseconds{});
            el::io::printLine(el::StringFormat{"Unexpected successful result: {}"_el}.build(unexpected.toString()));
        } catch (const el::err::ParameterError &) {
            el::io::printLine("Checked construction rejects an out-of-range day count."_el);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Event: Μάθημα αστρονομίας; start: 2026-07-01T12:30:00.000000000Z; restored: 2026-07-01T12:
    30:00.000000000Z
    Sentinel decoded: true; sentinel valid: false; oversized input decoded: false
    Default valid: false; invalid sorts first: true; invalid values equal: true
    Epoch: 0000-01-01T00:00:00.000000000Z; first: 0000-01-01T00:00:00.000000000Z; last: 9999-1
    2-31T23:59:59.999999999Z
    Current wall clock is valid: true
    Checked construction rejects an out-of-range day count.

.. erbsland-demo-end::

Exchanging Text
===============

:cpp:func:`fromIsoString() <erbsland::time::Timestamp::fromIsoString>` accepts the library's ISO date/time subset with
seconds and an explicit ``Z`` or numeric UTC offset.
It accepts up to nine fractional digits, and normalizes an offset-bearing input to UTC.
A missing offset, impossible date, unsupported syntax, or UTC result outside the calendar range returns no value.
:cpp:func:`fromIsoStringOrThrow() <erbsland::time::Timestamp::fromIsoStringOrThrow>` raises
:cpp:class:`ParseError <erbsland::err::ParseError>` instead.
The explicit offset is useful at an exchange boundary: the receiving application does not need to guess the sender's
local zone.

:cpp:func:`toIsoString() <erbsland::time::Timestamp::toIsoString>` always produces
``YYYY-MM-DDTHH:MM:SS.nnnnnnnnnZ``, with a dot and exactly nine fractional digits.
It returns an optional string, with no value for an invalid timestamp.
:cpp:func:`toIsoStringOrThrow() <erbsland::time::Timestamp::toIsoStringOrThrow>` raises
:cpp:class:`OutOfRangeError <erbsland::err::OutOfRangeError>` for that invalid state.
:cpp:func:`toString() <erbsland::time::Timestamp::toString>` produces the same text, but returns an empty string when invalid.

This canonical format preserves the full precision during a round trip.
For shorter output, different ISO flags, or a local display, convert to ``DateTime`` first.
The shared controls are introduced under :ref:`time-iso-output`; the canonical timestamp format stays fixed.

.. erbsland-demo::
    :source: time/Timestamp/Text.cpp
    :exec: time/timestamp --demo Text
    :source-sha256: aeb06bd297601da31114f0dd8364d9713b3ca797c38478035d7e019b6936da72

.. code-block:: cpp

    /// Exchange timestamp text with an explicit UTC offset and full fractional precision.
    /// @notest{Compiled and executed documentation demo.}
    void text() {
        const auto event = el::Timestamp::fromIsoStringOrThrow("2026-07-01T14:30:00.123456789+02:00"_el);
        el::io::printLine(
            el::StringFormat{"Canonical UTC: {}; ISO round trip: {}"_el}.build(
                event.toIsoStringOrThrow(), el::Timestamp::fromIsoStringOrThrow(event.toString()) == event));
        el::io::printLine(
            el::StringFormat{"Missing offset accepted: {}; invalid ISO available: {}"_el}.build(
                el::Timestamp::fromIsoString("2026-07-01T14:30:00"_el).has_value(),
                el::Timestamp{}.toIsoString().has_value()));
        el::io::printLine(
            el::StringFormat{"Out-of-calendar UTC accepted: {}"_el}.build(
                el::Timestamp::fromIsoString("0000-01-01T00:00:00+01:00"_el).has_value()));
        try {
            const auto unexpected = el::Timestamp::fromIsoStringOrThrow("2026-02-30T12:00:00Z"_el);
            el::io::printLine(el::StringFormat{"Unexpected successful result: {}"_el}.build(unexpected.toString()));
        } catch (const el::err::ParseError &) {
            el::io::printLine("Checked parsing rejects an impossible date."_el);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Canonical UTC: 2026-07-01T12:30:00.123456789Z; ISO round trip: true
    Missing offset accepted: false; invalid ISO available: false
    Out-of-calendar UTC accepted: false
    Checked parsing rejects an impossible date.

.. erbsland-demo-end::

Storing Fields and Portable Bytes
=================================

:cpp:func:`toRawValue() <erbsland::time::Timestamp::toRawValue>` returns a pair of signed integers: an ``int32_t`` day count
and an ``int64_t`` nanosecond count since midnight.
:cpp:func:`fromRawValue() <erbsland::time::Timestamp::fromRawValue>` accepts either that pair or the two separate fields.
It follows the same bounds and negative-field convention as
:cpp:func:`fromDaysAndNanoseconds() <erbsland::time::Timestamp::fromDaysAndNanoseconds>`.
The checked :cpp:func:`fromRawValueOrThrow() <erbsland::time::Timestamp::fromRawValueOrThrow>` rejects oversized
nonnegative fields with ``ParameterError``; a negative field still successfully constructs the invalid sentinel.
The invalid raw value is always ``(-1, 0)``.

For a file or network message, :cpp:func:`toByteBlock() <erbsland::time::Timestamp::toByteBlock>` writes exactly twelve
bytes: the signed four-byte day count followed by the signed eight-byte nanosecond count, both in big-endian order.
This is the portable representation to retain; copying the object's memory would also copy platform-dependent padding.
The invalid value has the encoding ``ffffffff0000000000000000``.

:cpp:func:`fromByteBlock() <erbsland::time::Timestamp::fromByteBlock>` checks the byte count and decoded fields.
It returns no value for a malformed block, whereas
:cpp:func:`fromByteBlockOrThrow() <erbsland::time::Timestamp::fromByteBlockOrThrow>` raises ``ParameterError``.
A block that decodes to negative fields yields the canonical invalid timestamp, so check validity after decoding when
invalid values are not allowed in your format.

.. erbsland-demo::
    :source: time/Timestamp/Storage.cpp
    :exec: time/timestamp --demo Storage
    :source-sha256: d6c9c7167b0f2756c7eaceb3181cd86a357f4fdec93f6780c11a8d833fd1323e

.. code-block:: cpp

    /// Store canonical timestamp fields or a portable twelve-byte representation.
    /// @notest{Compiled and executed documentation demo.}
    void storage() {
        const auto event = el::Timestamp::fromIsoStringOrThrow("2026-07-01T12:30:00.123456789Z"_el);
        const auto raw = event.toRawValue();
        el::io::printLine(
            el::StringFormat{"Raw days: {}; nanoseconds: {}; restored: {}"_el}.build(
                raw.first, raw.second, el::Timestamp::fromRawValueOrThrow(raw) == event));
        el::io::printLine(
            el::StringFormat{"Separate fields restored: {}; invalid fields canonicalized: {}"_el}.build(
                el::Timestamp::fromRawValueOrThrow(raw.first, raw.second) == event,
                el::Timestamp::fromRawValueOrThrow(-4, 27) == el::Timestamp{}));
        const auto bytes = event.toByteBlock();
        el::io::printLine(
            el::StringFormat{"Encoded: {}; byte round trip: {}"_el}.build(
                el::String::fromByteBlock(bytes), el::Timestamp::fromByteBlockOrThrow(bytes) == event));
        el::io::printLine(
            el::StringFormat{"Invalid encoding: {}; empty block accepted: {}"_el}.build(
                el::String::fromByteBlock(el::Timestamp{}.toByteBlock()),
                el::Timestamp::fromByteBlock(el::ByteBlock{}).has_value()));
        try {
            const auto unexpected = el::Timestamp::fromByteBlockOrThrow(el::ByteBlock{});
            el::io::printLine(el::StringFormat{"Unexpected successful result: {}"_el}.build(unexpected.toString()));
        } catch (const el::err::ParameterError &) {
            el::io::printLine("Checked decoding rejects the wrong byte count."_el);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Raw days: 740163; nanoseconds: 45000123456789; restored: true
    Separate fields restored: true; invalid fields canonicalized: true
    Encoded: 000b4b43000028ed685f9d15; byte round trip: true
    Invalid encoding: ffffffff0000000000000000; empty block accepted: false
    Checked decoding rejects the wrong byte count.

.. erbsland-demo-end::

Choosing a Display Zone
=======================

:cpp:func:`fromDateTime() <erbsland::time::Timestamp::fromDateTime>` copies the stored UTC instant and discards the
``DateTime`` display properties.
It preserves the fractional second and returns no value for an invalid input.
:cpp:func:`toDateTime() <erbsland::time::Timestamp::toDateTime>` returns a UTC ``DateTime`` for every valid timestamp.
Both ``OrThrow`` variants raise ``OutOfRangeError`` for invalid inputs.
:cpp:func:`isValidDateTime() <erbsland::time::Timestamp::isValidDateTime>` is equivalent to :cpp:func:`isValid() <erbsland::time::Timestamp::isValid>`: the two types have
matching calendar ranges.

A round trip through ``Timestamp`` preserves the instant, but does not restore the previous display zone.
Choose the zone again using ``DateTime::toTimeZone()`` when presenting the result.
This also avoids treating a zone's numeric offset on one date as its permanent offset.

.. erbsland-demo::
    :source: time/Timestamp/DateTime.cpp
    :exec: time/timestamp --demo DateTime
    :source-sha256: e1e7bfa6379c6a887c2c0444df32f36c618fda5c36166185f4573d026aeded15

.. code-block:: cpp

    /// Retain an instant while choosing or discarding its display zone.
    /// @notest{Compiled and executed documentation demo.}
    void dateTime() {
        const auto local = el::DateTime::fromIsoStringOrThrow("2026-07-01T14:30:00.123456789+02:00"_el);
        const auto event = el::Timestamp::fromDateTimeOrThrow(local);
        const auto utc = event.toDateTimeOrThrow();
        el::io::printLine(el::StringFormat{"Local input: {}; UTC conversion: {}"_el}.build(local, utc));
        el::io::printLine(
            el::StringFormat{"Instant unchanged: {}; DateTime equality: {}; convertible: {}"_el}.build(
                utc.toUtc() == local.toUtc(), utc == local, event.isValidDateTime()));
        const auto zone = el::TimeZone::fromNameOrThrow("Europe/Athens"_el);
        el::io::printLine(el::StringFormat{"Chosen display: {}"_el}.build(utc.toTimeZone(zone)));
        el::io::printLine(
            el::StringFormat{"Invalid input converts: {}; invalid timestamp converts: {}"_el}.build(
                el::Timestamp::fromDateTime(el::DateTime{}).has_value(), el::Timestamp{}.toDateTime().has_value()));
        try {
            const auto unexpected = el::Timestamp{}.toDateTimeOrThrow();
            el::io::printLine(el::StringFormat{"Unexpected successful result: {}"_el}.build(unexpected.toString()));
        } catch (const el::err::OutOfRangeError &) {
            el::io::printLine("An invalid timestamp has no dated instant."_el);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Local input: 2026-07-01 14:30:00.123456789+02:00; UTC conversion: 2026-07-01 12:30:00.1234
    56789Z
    Instant unchanged: true; DateTime equality: false; convertible: true
    Chosen display: 2026-07-01 15:30:00.123456789+03:00
    Invalid input converts: false; invalid timestamp converts: false
    An invalid timestamp has no dated instant.

.. erbsland-demo-end::

Converting Epoch Counts Without Losing Meaning
==============================================

An epoch count needs both a unit and an origin to identify an instant.
The :cpp:func:`toTicks() <erbsland::time::Timestamp::toTicks>` and
:cpp:func:`fromTicks() <erbsland::time::Timestamp::fromTicks>` templates support ``Seconds``, ``Milliseconds``,
``Microseconds``, and ``Nanoseconds`` relative to ``TimeEpoch``.
The default origin is the Core epoch, rather than POSIX.
Select the epoch explicitly when exchanging counts with another API.

These conversions allow signed counts: ``Milliseconds{-250}`` relative to POSIX means a quarter of a second before
``1970-01-01T00:00:00Z``.
Scalar output truncates toward zero when the requested unit is coarser than the stored precision.
That instant becomes zero scalar seconds, but minus 250 milliseconds.
Conversion returns no value when the instant is invalid, the count cannot fit its unit, or input ticks would leave the
calendar range.
The ``OrThrow`` forms report ``OutOfRangeError``.
A modern date fits POSIX nanoseconds, but a nanosecond count from year zero has a much smaller calendar reach.

The split :cpp:func:`toSecondsAndFractions() <erbsland::time::Timestamp::toSecondsAndFractions>` representation keeps
full precision over the entire calendar range.
It uses floor seconds and a nonnegative nanosecond fraction in ``0..999999999``.
For the quarter-second before POSIX, this is ``(-1, 750000000)``, which adds back to the correct instant.
The fraction is positive because it counts forward from the beginning of second minus one; it is not an independent
negative remainder.
The ``fromTicks(Seconds, Nanoseconds, epoch)`` overload consumes this representation.
Its optional form rejects an invalid fraction or out-of-range result; its throwing form reports ``ParameterError`` for
the fraction and ``OutOfRangeError`` for calendar overflow.

For native C or operating-system APIs, :cpp:func:`toTimeT() <erbsland::time::Timestamp::toTimeT>` returns floor POSIX
seconds and :cpp:func:`fromTimeT() <erbsland::time::Timestamp::fromTimeT>` reconstructs an instant at a whole second.
``std::time_t`` has a platform-dependent range and signedness, so this round trip can discard a fraction or fail to fit.
:cpp:func:`toTimeT() <erbsland::time::Timestamp::toTimeT>` returns minus one for invalid or unrepresentable values; on signed platforms, minus one is also a genuine
POSIX second before the epoch.
Keep validity and range information separately, or use the optional typed conversion when that distinction matters.
:cpp:func:`fromTimeT() <erbsland::time::Timestamp::fromTimeT>` returns an invalid timestamp outside the calendar range.

.. erbsland-demo::
    :source: time/Timestamp/Ticks.cpp
    :exec: time/timestamp --demo Ticks
    :source-sha256: 1de64355439420ee4b1d32bd1e5835ba6df63ee09d13825fa5a6afe7fae6eec6

.. code-block:: cpp

    /// Convert signed epoch ticks while preserving the meaning of negative fractions.
    /// @notest{Compiled and executed documentation demo.}
    void ticks() {
        const auto before = el::Timestamp::fromTicksOrThrow(el::Milliseconds{-250}, el::TimeEpoch::Posix);
        const auto [seconds, fraction] = before.toSecondsAndFractionsOrThrow(el::TimeEpoch::Posix);
        el::io::printLine(
            el::StringFormat{"Before POSIX epoch: {}; scalar seconds: {}; milliseconds: {}"_el}.build(
                before,
                before.toTicksOrThrow<el::Seconds>(el::TimeEpoch::Posix).toRawValue(),
                before.toTicksOrThrow<el::Milliseconds>(el::TimeEpoch::Posix).toRawValue()));
        el::io::printLine(
            el::StringFormat{"Floor seconds: {}; fraction: {}; split round trip: {}"_el}.build(
                seconds.toRawValue(),
                fraction.toRawValue(),
                el::Timestamp::fromTicksOrThrow(seconds, fraction, el::TimeEpoch::Posix) == before));
        el::io::printLine(
            el::StringFormat{"Microseconds: {}; nanoseconds: {}; DateTime accepts signed ticks: {}"_el}.build(
                before.toTicksOrThrow<el::Microseconds>(el::TimeEpoch::Posix).toRawValue(),
                before.toTicksOrThrow<el::Nanoseconds>(el::TimeEpoch::Posix).toRawValue(),
                el::DateTime::fromTicks(el::Milliseconds{-250}, el::TimeEpoch::Posix).has_value()));
        el::io::printLine(
            el::StringFormat{"Last instant fits Core nanoseconds: {}; invalid fraction accepted: {}"_el}.build(
                el::Timestamp::last().toTicks<el::Nanoseconds>().has_value(),
                el::Timestamp::fromTicks(el::Seconds{}, el::Nanoseconds{1'000'000'000}).has_value()));
        try {
            const auto unexpected = el::Timestamp::last().toTicksOrThrow<el::Nanoseconds>();
            el::io::printLine(el::StringFormat{"Unexpected successful result: {}"_el}.build(unexpected.toRawValue()));
        } catch (const el::err::OutOfRangeError &) {
            el::io::printLine("Checked tick conversion rejects overflow."_el);
        }
        // Native time_t has platform-dependent range and discards fractional seconds.
        const auto posix = el::Timestamp::epoch(el::TimeEpoch::Posix);
        el::io::printLine(
            el::StringFormat{"POSIX time_t: {}; native round trip: {}; negative fraction floors to: {}"_el}.build(
                posix.toTimeT(), el::Timestamp::fromTimeT(posix.toTimeT()) == posix, before.toTimeT()));
    }

    }

.. erbsland-ansi::
    :escape-char: ␛

    Before POSIX epoch: 1969-12-31T23:59:59.750000000Z; scalar seconds: 0; milliseconds: -250
    Floor seconds: -1; fraction: 750000000; split round trip: true
    Microseconds: -250000; nanoseconds: -250000000; DateTime accepts signed ticks: false
    Last instant fits Core nanoseconds: false; invalid fraction accepted: false
    Checked tick conversion rejects overflow.
    POSIX time_t: 0; native round trip: true; negative fraction floors to: -1

.. erbsland-demo-end::

Shifting an Instant by a Fixed Interval
=======================================

Timestamp arithmetic accepts ``Seconds``, ``Milliseconds``, ``Microseconds``, ``Nanoseconds``,
:cpp:class:`Duration <erbsland::time::Duration>`, and :cpp:class:`TimeDelta <erbsland::time::TimeDelta>`.
Convert larger typed amounts such as ``Hours`` to an accepted unit, or place them in a ``Duration`` or ``TimeDelta``.
These shifts describe fixed elapsed intervals; months and years belong to ``DateTime`` calendar arithmetic.

:cpp:func:`added() <erbsland::time::Timestamp::added>` and
:cpp:func:`subtracted() <erbsland::time::Timestamp::subtracted>` return a shifted copy, while :cpp:func:`add() <erbsland::time::Timestamp::add>` and :cpp:func:`subtract() <erbsland::time::Timestamp::subtract>`
update the object in place.
The ``+``, ``-``, ``+=``, and ``-=`` operators follow these saturating operations.
At a calendar boundary, saturation retains the first or last supported instant, including its boundary time of day.

If clamping would misrepresent the event you are calculating, choose
:cpp:func:`addedOrThrow() <erbsland::time::Timestamp::addedOrThrow>`,
:cpp:func:`subtractedOrThrow() <erbsland::time::Timestamp::subtractedOrThrow>`,
:cpp:func:`addOrThrow() <erbsland::time::Timestamp::addOrThrow>`, or :cpp:func:`subtractOrThrow() <erbsland::time::Timestamp::subtractOrThrow>`.
They report :cpp:class:`OverflowError <erbsland::err::OverflowError>` and leave an in-place target unchanged on failure.
:cpp:func:`wouldAddSaturate() <erbsland::time::Timestamp::wouldAddSaturate>` and
:cpp:func:`wouldSubtractSaturate() <erbsland::time::Timestamp::wouldSubtractSaturate>` let you check the boundary first.
Invalid timestamps remain invalid even under throwing arithmetic, and their saturation predicates return false.
An overflow check therefore does not replace a validity check.

.. erbsland-demo::
    :source: time/Timestamp/Calculate.cpp
    :exec: time/timestamp --demo Calculate
    :source-sha256: a3ac434a951e771481391bbc8ae79a7c9e6f50e702ec00f852d6bdb76c400dc2

.. code-block:: cpp

    /// Shift fixed intervals and explicitly choose saturation or an overflow error.
    /// @notest{Compiled and executed documentation demo.}
    void calculate() {
        const auto start = el::Timestamp::fromIsoStringOrThrow("2026-07-01T23:59:59.900000000Z"_el);
        const auto finish = start.addedOrThrow(el::Milliseconds{250});
        auto adjusted = finish;
        adjusted.subtractOrThrow(el::TimeDelta{el::Milliseconds{250}});
        adjusted.add(el::Duration{el::Seconds{2}});
        adjusted.subtract(el::Seconds{2});
        el::io::printLine(
            el::StringFormat{"Start: {}; finish: {}; restored in place: {}"_el}.build(start, finish, adjusted == start));
        el::io::printLine(
            el::StringFormat{"Operators agree: {}"_el}.build(
                (start + el::Milliseconds{250}) - el::Milliseconds{250} == start));
        auto boundary = el::Timestamp::last();
        el::io::printLine(
            el::StringFormat{"Adding 1 ns saturates: {}; subtracting at first saturates: {}; clamped: {}"_el}.build(
                boundary.wouldAddSaturate(el::Nanoseconds{1}),
                el::Timestamp::first().wouldSubtractSaturate(el::Seconds{1}),
                boundary.added(el::Nanoseconds{1}) == boundary));
        try {
            boundary.addOrThrow(el::Nanoseconds{1});
        } catch (const el::err::OverflowError &) {
            el::io::printLine(
                el::StringFormat{"Checked addition rejected; original unchanged: {}"_el}.build(
                    boundary == el::Timestamp::last()));
        }
        el::io::printLine(
            el::StringFormat{"Invalid remains invalid: {}; invalid overflow predicate: {}"_el}.build(
                !el::Timestamp{}.addedOrThrow(el::Seconds{1}).isValid(), el::Timestamp{}.wouldAddSaturate(el::Seconds{1})));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Start: 2026-07-01T23:59:59.900000000Z; finish: 2026-07-02T00:00:00.150000000Z; restored in
     place: true
    Operators agree: true
    Adding 1 ns saturates: true; subtracting at first saturates: true; clamped: true
    Checked addition rejected; original unchanged: true
    Invalid remains invalid: true; invalid overflow predicate: false

.. erbsland-demo-end::

Finding the Distance Between Instants
=====================================

A distance from ``start`` to ``finish`` is positive when ``finish`` comes later and negative when it comes earlier.
The methods :cpp:func:`secondsTo() <erbsland::time::Timestamp::secondsTo>`,
:cpp:func:`millisecondsTo() <erbsland::time::Timestamp::millisecondsTo>`,
:cpp:func:`microsecondsTo() <erbsland::time::Timestamp::microsecondsTo>`, and
:cpp:func:`nanosecondsTo() <erbsland::time::Timestamp::nanosecondsTo>` return typed total amounts at the requested
resolution.
They truncate the complete signed distance toward zero, rather than truncating each timestamp before subtracting.
:cpp:func:`durationTo() <erbsland::time::Timestamp::durationTo>` expresses the whole-second result as ``Duration``; :cpp:func:`timeDeltaTo() <erbsland::time::Timestamp::timeDeltaTo>` expresses the nanosecond result as
``TimeDelta``.
Subtracting ``finish - start`` returns the same precise result as ``start.timeDeltaTo(finish)``.

Both timestamps can be valid while their precise distance is too large for a signed 64-bit nanosecond interval.
The ordinary distance methods saturate to the interval's supported bound.
This is a bound on the answer, not on either event: reducing the output precision can retain the distance when a precise
interval cannot represent it.
Each has a corresponding ``...OrThrow()`` form that reports ``OverflowError`` and a ``would...ToSaturate()`` predicate,
such as :cpp:func:`wouldNanosecondsToSaturate() <erbsland::time::Timestamp::wouldNanosecondsToSaturate>` or
:cpp:func:`wouldTimeDeltaToSaturate() <erbsland::time::Timestamp::wouldTimeDeltaToSaturate>`.
Whole seconds, milliseconds, and microseconds can span the complete supported calendar.
A signed 64-bit nanosecond interval reaches roughly 292 years in either direction, so it cannot express every distance
between supported timestamps.

Ordinary distances involving an invalid operand return zero, checked distances report ``ParameterError``, and overflow
predicates return false.
When zero has a meaningful interpretation in your application, validate the operands or use the checked form so that
missing data cannot masquerade as simultaneous events.

.. erbsland-demo::
    :source: time/Timestamp/Distances.cpp
    :exec: time/timestamp --demo Distances
    :source-sha256: 722605ba92d137d45ad041294222aeec14c9ee9b37f43af87e76e6ed912e9e9f

.. code-block:: cpp

    /// Choose the precision and range of a signed distance between recorded instants.
    /// @notest{Compiled and executed documentation demo.}
    void distances() {
        const auto start = el::Timestamp::fromIsoStringOrThrow("2026-07-01T12:00:00.900000000Z"_el);
        const auto finish = start.addedOrThrow(el::Milliseconds{250});
        el::io::printLine(
            el::StringFormat{"Seconds: {}; milliseconds: {}; microseconds: {}; nanoseconds: {}"_el}.build(
                start.secondsToOrThrow(finish).toRawValue(),
                start.millisecondsToOrThrow(finish).toRawValue(),
                start.microsecondsToOrThrow(finish).toRawValue(),
                start.nanosecondsToOrThrow(finish).toRawValue()));
        el::io::printLine(
            el::StringFormat{"Whole duration (seconds): {}; precise delta: {}; reverse: {}; subtraction: {}"_el}.build(
                start.durationToOrThrow(finish).toSeconds().toRawValue(),
                start.timeDeltaToOrThrow(finish),
                finish.timeDeltaTo(start),
                finish - start));
        el::io::printLine(
            el::StringFormat{"Full calendar span fits seconds: {}; fits precise delta: {}"_el}.build(
                !el::Timestamp::first().wouldSecondsToSaturate(el::Timestamp::last()),
                !el::Timestamp::first().wouldTimeDeltaToSaturate(el::Timestamp::last())));
        try {
            const auto unexpected = el::Timestamp::first().timeDeltaToOrThrow(el::Timestamp::last());
            el::io::printLine(el::StringFormat{"Unexpected successful result: {}"_el}.build(unexpected.toString()));
        } catch (const el::err::OverflowError &) {
            el::io::printLine("Two valid instants can be too far apart for TimeDelta."_el);
        }
        el::io::printLine(
            el::StringFormat{"Tolerant invalid distance: {}"_el}.build(el::Timestamp{}.secondsTo(start).toRawValue()));
        try {
            const auto unexpected = el::Timestamp{}.secondsToOrThrow(start);
            el::io::printLine(el::StringFormat{"Unexpected successful result: {}"_el}.build(unexpected.toRawValue()));
        } catch (const el::err::ParameterError &) {
            el::io::printLine("Checked distance rejects an invalid operand."_el);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Seconds: 0; milliseconds: 250; microseconds: 250000; nanoseconds: 250000000
    Whole duration (seconds): 0; precise delta: 250 ms; reverse: -250 ms; subtraction: 250 ms
    Full calendar span fits seconds: true; fits precise delta: false
    Two valid instants can be too far apart for TimeDelta.
    Tolerant invalid distance: 0
    Checked distance rejects an invalid operand.

.. erbsland-demo-end::

Continuing with Calendar and Clock Tasks
========================================

:doc:`working_with_datetime` develops local display, named zones, and calendar changes while retaining an instant.
:doc:`working_with_dates` and :doc:`working_with_times` explain the separate fields used by the UTC constructor.
For clock measurements, the :doc:`overview` introduces ``TimePoint`` and ``ElapsedTimer``.
The :doc:`timestamp reference <../../reference/time/timestamp>` lists the complete interface and serialization contract.
