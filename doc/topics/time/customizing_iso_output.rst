..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Time; ISO Output
    single: ISO Date and Time; Formatting
    single: DateTimePrecision

.. _time-iso-output:

************************************
Customizing ISO Date and Time Output
************************************

A date or clock reading can appear in a log, an exported file, or an input field with different requirements.
You might want compact fields, a visible UTC offset, or exactly three fractional digits.
The ISO output controls let you make these choices explicitly while keeping the original value intact.
This page explains the formatting flags shared by :cpp:class:`Date <erbsland::time::Date>`,
:cpp:class:`Time <erbsland::time::Time>`, and :cpp:class:`DateTime <erbsland::time::DateTime>`, then shows how to choose
which fields and fractional digits reach the text.

Starting with a Clear Text Contract
===================================

ISO 8601 describes date and time representations that put the larger calendar fields first.
An extended date such as ``2028-02-29`` uses separators; its basic counterpart is ``20280229``.
A combined reading can join the date and time with ``T``, and an offset identifies the relationship to UTC.
For example, ``2028-02-29T14:30:45+01:30`` describes a local reading one hour and thirty minutes ahead of UTC.
``Z`` is the UTC designator, equivalent to a zero offset.

A useful first step is to agree on the text your reader or receiving system expects.
A consumer might require ``T``, a dot before the fraction, and an explicit offset even though another consumer accepts
spaces, commas, or an offset-free local reading.
The methods here format calendar dates and clock readings; they do not select a display time zone or change an instant.
:doc:`working_with_datetime` explains how to choose the zone first.

The entry points are :cpp:func:`Date::toIsoString() <erbsland::time::Date::toIsoString>`,
:cpp:func:`Time::toIsoString() <erbsland::time::Time::toIsoString>`, and
:cpp:func:`DateTime::toIsoString() <erbsland::time::DateTime::toIsoString>`.
You can start with their default output and add only the details your application needs.
The default ``toIsoString()`` output uses extended fields.
``Date`` emits a complete date, ``Time`` stops at seconds, and ``DateTime`` joins the date and time with a space and
omits the offset.
That default is convenient for reading, but it does not retain a fractional second or identify the zone of a combined
reading.
For exchanging an instant, make the required offset and precision explicit, or choose the canonical UTC representation
provided by :doc:`working_with_timestamps`.
``Timestamp::toIsoString()`` has its own fixed format and does not take these flags.

.. erbsland-demo::
    :source: time/IsoTimeOutput/Defaults.cpp
    :exec: time/iso_time_output --demo Defaults
    :source-sha256: ee9e523a2abd2632f29574206e160d2786e2dc0bd5c3520a1130f2ad67f3f432

.. code-block:: cpp

    /// Choose ISO output for a recorded date and clock reading.
    /// @notest{Compiled and executed documentation demo.}
    void defaults() {
        const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
        const auto time = el::Time{el::Hour{14}, el::Minute{30}, el::Second{45}, el::Nanoseconds{123456789}};
        const auto reading = el::DateTime{date, time, el::Seconds{5400}};
        const auto prototype = "試作センサー"_el;
        el::io::printLine(el::StringFormat{"{}: {}"_el}.build(prototype, reading.toIsoString()));
        el::io::printLine(el::StringFormat{"Date: {}; time: {}"_el}.build(date.toIsoString(), time.toIsoString()));

        // Explicit flags and precision retain the fractional reading and its offset.
        const auto format = el::IsoTimeFormatFlags{
            el::IsoTimeFormat::Extended,
            el::IsoTimeFormat::TimePrefix,
            el::IsoTimeFormat::TimeShift,
            el::IsoTimeFormat::UseDotFraction};
        el::io::printLine(
            el::StringFormat{"Exchange text: {}"_el}.build(reading.toIsoString(format, el::DateTimePrecision::Nanosecond)));
        el::io::printLine(el::StringFormat{"Readable display: {}"_el}.build(reading.toString()));
        el::io::printLine(
            el::StringFormat{"Invalid date: [{}]; invalid date/time: [{}]"_el}.build(
                el::Date{}.toIsoString(), el::DateTime{}.toIsoString()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    試作センサー: 2028-02-29 14:30:45
    Date: 2028-02-29; time: 14:30:45
    Exchange text: 2028-02-29T14:30:45.123456789+01:30
    Readable display: 2028-02-29 14:30:45.123456789+01:30
    Invalid date: []; invalid date/time: []

.. erbsland-demo-end::

Invalid ``Date`` and ``DateTime`` values produce an empty ISO string.
A ``Time`` always holds a valid reading within its day.
Formatting therefore does not replace validation of an optional date or instant; check validity before exporting a
required field.
The last line of the demo puts empty results in brackets to make them visible.

Choosing the Formatting Flags
=============================

:cpp:type:`IsoTimeFormatFlags <erbsland::time::IsoTimeFormatFlags>` holds a set of
:cpp:enum:`IsoTimeFormat <erbsland::time::IsoTimeFormat>` options.
Construct it with the flags you need, then pass it as the first argument to ``toIsoString()``.
An explicitly empty set selects basic fields without a time prefix or an offset; passing your own set replaces the
default flags rather than adding to them.
Once a set has a useful baseline, you can keep it and change one option at a time.
The examples below do this with ``set()`` and ``clear()``, so each pair of outputs makes one choice visible.

Each flag controls one aspect of the text, but some choices depend on others.
Offset-detail flags need ``TimeShift``, and the fraction separator only appears at a fractional precision.
``Date`` ignores time-only flags, while ``Time`` has no offset to append.
The following examples keep the same date, time, and offset so that you can compare the effects directly.

Extended: Separators Between Fields
-----------------------------------

:cpp:enumerator:`Extended <erbsland::time::IsoTimeFormat::Extended>` inserts hyphens into dates and colons into clock
readings and numeric offsets.
Without it, the fields remain zero-padded but touch one another: ``14:30:45`` becomes ``143045``, and ``+01:30`` becomes
``+0130``.
The ``T`` separator and the fraction separator have their own flags; changing ``Extended`` leaves those choices alone.

.. erbsland-demo::
    :source: time/IsoTimeOutput/Extended.cpp
    :exec: time/iso_time_output --demo Extended
    :source-sha256: 86265d11ad58f07c17d983822dd3f82087ef65c338a3945512346e0e948a59b0

.. code-block:: cpp

    /// Compare basic and extended ISO date and time fields.
    /// @notest{Compiled and executed documentation demo.}
    void extended() {
        const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
        const auto time = el::Time{el::Hour{14}, el::Minute{30}, el::Second{45}, el::Nanoseconds{123456789}};
        const auto reading = el::DateTime{date, time, el::Seconds{5400}};
        auto format = el::IsoTimeFormatFlags{el::IsoTimeFormat::TimePrefix, el::IsoTimeFormat::TimeShift};
        el::io::printLine(
            el::StringFormat{"Basic date: {}; time: {}; date/time: {}"_el}.build(
                date.toIsoString(format), time.toIsoString(format), reading.toIsoString(format)));
        format.set(el::IsoTimeFormat::Extended);
        el::io::printLine(
            el::StringFormat{"Extended date: {}; time: {}; date/time: {}"_el}.build(
                date.toIsoString(format), time.toIsoString(format), reading.toIsoString(format)));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Basic date: 20280229; time: T143045; date/time: 20280229T143045+0130
    Extended date: 2028-02-29; time: T14:30:45; date/time: 2028-02-29T14:30:45+01:30

.. erbsland-demo-end::

TimePrefix: The T Separator
---------------------------

:cpp:enumerator:`TimePrefix <erbsland::time::IsoTimeFormat::TimePrefix>` prefixes a standalone ``Time`` with ``T``.
For ``DateTime``, it selects ``T`` between the date and time instead of the default space.
The combined result contains a single ``T``, and a standalone ``Date`` is unaffected.
When your text contract calls for the familiar ``dateTtime`` spelling, set this flag explicitly.

.. erbsland-demo::
    :source: time/IsoTimeOutput/TimePrefix.cpp
    :exec: time/iso_time_output --demo TimePrefix
    :source-sha256: 5e7fb8efcd6df75cea81cad3ee76fc9524cd407835bf0856d312e6cc4bc48f3d

.. code-block:: cpp

    /// Select the T prefix and combined date/time separator.
    /// @notest{Compiled and executed documentation demo.}
    void timePrefix() {
        const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
        const auto time = el::Time{el::Hour{14}, el::Minute{30}, el::Second{45}, el::Nanoseconds{123456789}};
        const auto reading = el::DateTime{date, time, el::Seconds{5400}};
        auto format = el::IsoTimeFormatFlags{el::IsoTimeFormat::Extended};
        el::io::printLine(
            el::StringFormat{"Without prefix: {}; {}"_el}.build(time.toIsoString(format), reading.toIsoString(format)));
        format.set(el::IsoTimeFormat::TimePrefix);
        el::io::printLine(
            el::StringFormat{"With prefix: {}; {}"_el}.build(time.toIsoString(format), reading.toIsoString(format)));
        el::io::printLine(el::StringFormat{"Date stays: {}"_el}.build(date.toIsoString(format)));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Without prefix: 14:30:45; 2028-02-29 14:30:45
    With prefix: T14:30:45; 2028-02-29T14:30:45
    Date stays: 2028-02-29

.. erbsland-demo-end::

TimeShift: Including the UTC Offset
-----------------------------------

:cpp:enumerator:`TimeShift <erbsland::time::IsoTimeFormat::TimeShift>` appends the resolved UTC offset to ``DateTime``
output that includes a clock reading.
It writes ``Z`` for UTC by default and a signed hours-and-minutes offset otherwise.
For a named zone, this is the offset at the represented instant; the text does not carry the zone's IANA identifier.
The flag also forces offset output for a value whose display zone came from the local system setting.

Adding the flag preserves the displayed local fields.
It does not convert ``14:30:45+01:30`` to ``13:00:45Z``; those are two displays of the same instant in different zones.
The demo constructs a UTC view separately to show that distinction.
Neither a standalone date nor a standalone time contains an offset, so this flag has no effect on those values.

.. erbsland-demo::
    :source: time/IsoTimeOutput/TimeShift.cpp
    :exec: time/iso_time_output --demo TimeShift
    :source-sha256: 8b7f4dd33fbae41b00385f7ce1446c1c72dce6c768de4e37281b6728aa696ed6

.. code-block:: cpp

    /// Include a resolved UTC offset without changing the display zone.
    /// @notest{Compiled and executed documentation demo.}
    void timeShift() {
        const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
        const auto time = el::Time{el::Hour{14}, el::Minute{30}, el::Second{45}, el::Nanoseconds{123456789}};
        const auto reading = el::DateTime{date, time, el::Seconds{5400}};
        auto format = el::IsoTimeFormatFlags{el::IsoTimeFormat::Extended, el::IsoTimeFormat::TimePrefix};
        el::io::printLine(el::StringFormat{"Offset omitted: {}"_el}.build(reading.toIsoString(format)));
        format.set(el::IsoTimeFormat::TimeShift);
        el::io::printLine(el::StringFormat{"Offset included: {}"_el}.build(reading.toIsoString(format)));
        const auto utc = el::DateTime{reading.utcDate(), reading.utcTime()};
        el::io::printLine(el::StringFormat{"Same instant in UTC: {}"_el}.build(utc.toIsoString(format)));
        el::io::printLine(
            el::StringFormat{"Standalone fields: {}; {}"_el}.build(date.toIsoString(format), time.toIsoString(format)));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Offset omitted: 2028-02-29T14:30:45
    Offset included: 2028-02-29T14:30:45+01:30
    Same instant in UTC: 2028-02-29T13:00:45Z
    Standalone fields: 2028-02-29; T14:30:45

.. erbsland-demo-end::

TimeShiftAlwaysComplete: Numeric UTC
------------------------------------

:cpp:enumerator:`TimeShiftAlwaysComplete <erbsland::time::IsoTimeFormat::TimeShiftAlwaysComplete>` replaces ``Z`` with
``+00:00`` in extended output, or ``+0000`` in basic output.
This is useful when the receiving format expects every offset to have the same numeric form.
The flag does not enable offset output by itself: ``TimeShift`` must also be set.
When combined with ``TimeShiftUpToSeconds``, it additionally forces the offset seconds field, including ``:00``.

.. erbsland-demo::
    :source: time/IsoTimeOutput/CompleteOffset.cpp
    :exec: time/iso_time_output --demo CompleteOffset
    :source-sha256: 97c618db73aa415f65202de1662f17937c5e057334d4fd931eb2be49ce2698fb

.. code-block:: cpp

    /// Replace the UTC designator with a numeric zero offset.
    /// @notest{Compiled and executed documentation demo.}
    void completeOffset() {
        const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
        const auto time = el::Time{el::Hour{14}, el::Minute{30}, el::Second{45}, el::Nanoseconds{123456789}};
        const auto utc = el::DateTime{date, time};
        auto format = el::IsoTimeFormatFlags{
            el::IsoTimeFormat::Extended, el::IsoTimeFormat::TimePrefix, el::IsoTimeFormat::TimeShift};
        el::io::printLine(el::StringFormat{"UTC designator: {}"_el}.build(utc.toIsoString(format)));
        format.set(el::IsoTimeFormat::TimeShiftAlwaysComplete);
        el::io::printLine(el::StringFormat{"Numeric UTC offset: {}"_el}.build(utc.toIsoString(format)));
        format.clear(el::IsoTimeFormat::Extended);
        el::io::printLine(el::StringFormat{"Basic numeric offset: {}"_el}.build(utc.toIsoString(format)));
        format.clear(el::IsoTimeFormat::TimeShift);
        el::io::printLine(el::StringFormat{"Without TimeShift: {}"_el}.build(utc.toIsoString(format)));
    }

.. erbsland-ansi::
    :escape-char: ␛

    UTC designator: 2028-02-29T14:30:45Z
    Numeric UTC offset: 2028-02-29T14:30:45+00:00
    Basic numeric offset: 20280229T143045+0000
    Without TimeShift: 20280229T143045

.. erbsland-demo-end::

TimeShiftUpToSeconds: Preserving Offset Seconds
-----------------------------------------------

Offsets can contain seconds, for example when working with historical zone data or an explicit second-level offset.
:cpp:enumerator:`TimeShiftUpToSeconds <erbsland::time::IsoTimeFormat::TimeShiftUpToSeconds>` retains that seconds field
when it is nonzero.
An offset of 5,417 seconds then appears as ``+01:30:17`` instead of ``+01:30``.
Without the flag, the omitted seconds change the instant implied by exported text, even if the clock fields themselves
retain nanosecond precision.
Choose it when the complete resolved offset matters.

A whole-minute offset still ends at minutes unless ``TimeShiftAlwaysComplete`` is also set.
With both detail flags, ``+01:30`` becomes ``+01:30:00``, and UTC becomes ``+00:00:00``.
Basic output omits the colons in these offset fields as well.
Both flags require ``TimeShift`` and a precision that includes the time.

.. erbsland-demo::
    :source: time/IsoTimeOutput/OffsetSeconds.cpp
    :exec: time/iso_time_output --demo OffsetSeconds
    :source-sha256: 0faa43ac456ae8a51ace155adec6776f34900a54391696e829c85d58e1c09fcd

.. code-block:: cpp

    /// Preserve second-level offsets and understand complete offset output.
    /// @notest{Compiled and executed documentation demo.}
    void offsetSeconds() {
        const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
        const auto time = el::Time{el::Hour{14}, el::Minute{30}, el::Second{45}, el::Nanoseconds{123456789}};
        const auto reading = el::DateTime{date, time, el::Seconds{5400}};
        const auto secondOffset = el::DateTime{date, time, el::Seconds{5417}};
        auto format = el::IsoTimeFormatFlags{
            el::IsoTimeFormat::Extended, el::IsoTimeFormat::TimePrefix, el::IsoTimeFormat::TimeShift};
        el::io::printLine(el::StringFormat{"Offset seconds omitted: {}"_el}.build(secondOffset.toIsoString(format)));
        format.set(el::IsoTimeFormat::TimeShiftUpToSeconds);
        el::io::printLine(el::StringFormat{"Offset seconds retained: {}"_el}.build(secondOffset.toIsoString(format)));
        el::io::printLine(el::StringFormat{"Whole-minute offset: {}"_el}.build(reading.toIsoString(format)));
        format.set(el::IsoTimeFormat::TimeShiftAlwaysComplete);
        el::io::printLine(el::StringFormat{"Complete whole-minute offset: {}"_el}.build(reading.toIsoString(format)));
        const auto utc = el::DateTime{date, time};
        el::io::printLine(el::StringFormat{"Complete UTC offset: {}"_el}.build(utc.toIsoString(format)));
        const auto all = el::IsoTimeFormatFlags{el::IsoTimeFormat::All};
        el::io::printLine(
            el::StringFormat{"All flags: {}"_el}.build(secondOffset.toIsoString(all, el::DateTimePrecision::Nanosecond)));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Offset seconds omitted: 2028-02-29T14:30:45+01:30
    Offset seconds retained: 2028-02-29T14:30:45+01:30:17
    Whole-minute offset: 2028-02-29T14:30:45+01:30
    Complete whole-minute offset: 2028-02-29T14:30:45+01:30:00
    Complete UTC offset: 2028-02-29T14:30:45+00:00:00
    All flags: 2028-02-29T14:30:45.123456789+01:30:17

.. erbsland-demo-end::

The final example uses :cpp:enumerator:`All <erbsland::time::IsoTimeFormat::All>`, which enables every formatting flag.
It is a convenient combination when you want all these choices together, including numeric UTC and complete offset
seconds.
It still needs a separate precision argument: enabling all flags alone does not request fractional seconds.

UseDotFraction: Choosing the Fraction Separator
-----------------------------------------------

ISO fractional seconds can use a comma or a dot.
The formatter uses a comma unless :cpp:enumerator:`UseDotFraction <erbsland::time::IsoTimeFormat::UseDotFraction>` is
set.
This choice applies to ``Time`` and the time portion of ``DateTime``; a ``Date`` has no fraction.
At second precision or less, neither separator appears.

Fractional output has a fixed width: three digits for milliseconds, six for microseconds, and nine for nanoseconds.
A stored zero fraction therefore becomes ``.000`` at millisecond precision, and 120 milliseconds becomes ``.120``.
This differs from the readable ``Time::toString()`` display, which removes trailing fractional zeros.
The precision argument, rather than the separator flag, chooses how many digits to emit.

.. erbsland-demo::
    :source: time/IsoTimeOutput/Fractions.cpp
    :exec: time/iso_time_output --demo Fractions
    :source-sha256: b580c8a0f3ffe07e460dbc3a94705ed2fda7651cf70243cadfca8db0769d02d5

.. code-block:: cpp

    /// Choose a fraction separator and retain fixed-width fractional digits.
    /// @notest{Compiled and executed documentation demo.}
    void fractions() {
        const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
        const auto time = el::Time{el::Hour{14}, el::Minute{30}, el::Second{45}, el::Nanoseconds{123456789}};
        const auto reading = el::DateTime{date, time, el::Seconds{5400}};
        auto format = el::IsoTimeFormatFlags{el::IsoTimeFormat::Extended, el::IsoTimeFormat::TimePrefix};
        const auto precision = el::DateTimePrecision::Millisecond;
        el::io::printLine(
            el::StringFormat{"Comma: {}; {}"_el}.build(
                time.toIsoString(format, precision), reading.toIsoString(format, precision)));
        format.set(el::IsoTimeFormat::UseDotFraction);
        el::io::printLine(
            el::StringFormat{"Dot: {}; {}"_el}.build(
                time.toIsoString(format, precision), reading.toIsoString(format, precision)));
        const auto exact = el::Time{el::Hour{14}, el::Minute{30}, el::Second{45}};
        const auto trailing = el::Time{el::Hour{14}, el::Minute{30}, el::Second{45}, el::Nanoseconds{120000000}};
        el::io::printLine(
            el::StringFormat{"Zero fraction: {}; trailing zeros: {}"_el}.build(
                exact.toIsoString(format, precision), trailing.toIsoString(format, precision)));
        el::io::printLine(
            el::StringFormat{"Second precision: {}"_el}.build(time.toIsoString(format, el::DateTimePrecision::Second)));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Comma: T14:30:45,123; 2028-02-29T14:30:45,123
    Dot: T14:30:45.123; 2028-02-29T14:30:45.123
    Zero fraction: T14:30:45.000; trailing zeros: T14:30:45.120
    Second precision: T14:30:45

.. erbsland-demo-end::

Selecting the Displayed Precision
=================================

:cpp:enum:`DateTimePrecision <erbsland::time::DateTimePrecision>` is the second ``toIsoString()`` argument.
It sets the last field or fractional digit included in the output.
For a month selector, ``Month`` can be enough; for a recorded measurement, the fraction may be part of the data you need
to preserve.
Choosing precision at the point of formatting lets both views come from the same stored value.
Reducing it leaves the stored value unchanged and discards smaller fields in the text, without rounding.
For example, ``14:30:45.123456789`` becomes ``14:30:45.123`` at millisecond precision.
There is no carry into the next second, minute, or date.

The same enum works across all three types, but each can only emit fields it actually has.
A ``Date`` stops at the day even when you request nanoseconds.
A ``Time`` always emits at least the hour, so ``Year``, ``Month``, and ``Day`` each produce ``14`` for this example.
A ``DateTime`` emits only calendar fields below ``Hour`` precision; it includes neither a time separator nor an offset
at those levels, even with ``TimeShift`` enabled.

.. list-table::
    :header-rows: 1
    :widths: 23 40 37

    * - Precision
      - Combined output reaches
      - Standalone field behavior
    * - ``Year``
      - Four-digit year
      - ``Date``: year; ``Time``: hour
    * - ``Month``
      - Year and month
      - ``Date``: year and month; ``Time``: hour
    * - ``Day``
      - Full date
      - ``Date``: full date; ``Time``: hour
    * - ``Hour``
      - Full date and hour
      - ``Date``: full date; ``Time``: hour
    * - ``Minute``
      - Full date, hour, and minute
      - ``Date``: full date; ``Time``: through minute
    * - ``Second``
      - Full date and time through second
      - ``Date``: full date; ``Time``: through second
    * - ``Millisecond``
      - Time with three fractional digits
      - ``Date``: full date; ``Time``: three fractional digits
    * - ``Microsecond``
      - Time with six fractional digits
      - ``Date``: full date; ``Time``: six fractional digits
    * - ``Nanosecond``
      - Time with nine fractional digits
      - ``Date``: full date; ``Time``: nine fractional digits

The demo prints every enum value for the same date, standalone time, and combined reading.
The dot, ``T``, and offset remain enabled throughout, making it easier to see which details depend on precision.

.. erbsland-demo::
    :source: time/IsoTimeOutput/Precision.cpp
    :exec: time/iso_time_output --demo Precision
    :source-sha256: d04140117f3fa10a90c640e419c086fdd99b721a758737e7ca12b2f6e423a422

.. code-block:: cpp

    /// Compare every ISO output precision on the same stored values.
    /// @notest{Compiled and executed documentation demo.}
    void precision() {
        const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
        const auto time = el::Time{el::Hour{14}, el::Minute{30}, el::Second{45}, el::Nanoseconds{123456789}};
        const auto reading = el::DateTime{date, time, el::Seconds{5400}};
        const auto format = el::IsoTimeFormatFlags{
            el::IsoTimeFormat::Extended,
            el::IsoTimeFormat::TimePrefix,
            el::IsoTimeFormat::TimeShift,
            el::IsoTimeFormat::UseDotFraction};
        const auto levels = std::array{
            std::pair{"Year"_el, el::DateTimePrecision::Year},
            std::pair{"Month"_el, el::DateTimePrecision::Month},
            std::pair{"Day"_el, el::DateTimePrecision::Day},
            std::pair{"Hour"_el, el::DateTimePrecision::Hour},
            std::pair{"Minute"_el, el::DateTimePrecision::Minute},
            std::pair{"Second"_el, el::DateTimePrecision::Second},
            std::pair{"Millisecond"_el, el::DateTimePrecision::Millisecond},
            std::pair{"Microsecond"_el, el::DateTimePrecision::Microsecond},
            std::pair{"Nanosecond"_el, el::DateTimePrecision::Nanosecond}};
        for (const auto &[name, precision] : levels) {
            el::io::printLine(el::StringFormat{"{}:"_el}.build(name));
            el::io::printLine(el::StringFormat{"  Date: {}"_el}.build(date.toIsoString(format, precision)));
            el::io::printLine(el::StringFormat{"  Time: {}"_el}.build(time.toIsoString(format, precision)));
            el::io::printLine(el::StringFormat{"  Date/time: {}"_el}.build(reading.toIsoString(format, precision)));
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Year:
      Date: 2028
      Time: T14
      Date/time: 2028
    Month:
      Date: 2028-02
      Time: T14
      Date/time: 2028-02
    Day:
      Date: 2028-02-29
      Time: T14
      Date/time: 2028-02-29
    Hour:
      Date: 2028-02-29
      Time: T14
      Date/time: 2028-02-29T14+01:30
    Minute:
      Date: 2028-02-29
      Time: T14:30
      Date/time: 2028-02-29T14:30+01:30
    Second:
      Date: 2028-02-29
      Time: T14:30:45
      Date/time: 2028-02-29T14:30:45+01:30
    Millisecond:
      Date: 2028-02-29
      Time: T14:30:45.123
      Date/time: 2028-02-29T14:30:45.123+01:30
    Microsecond:
      Date: 2028-02-29
      Time: T14:30:45.123456
      Date/time: 2028-02-29T14:30:45.123456+01:30
    Nanosecond:
      Date: 2028-02-29
      Time: T14:30:45.123456789
      Date/time: 2028-02-29T14:30:45.123456789+01:30

.. erbsland-demo-end::

Choosing the Next Step
======================

:doc:`working_with_dates` and :doc:`working_with_times` explain how to construct and inspect the fields being formatted.
:doc:`working_with_datetime` covers display zones and reading date/time text back into a value, including the separate
role of precision requirements during parsing.
For a fixed UTC exchange format, continue with :doc:`working_with_timestamps`.
Time intervals use unit-based output rather than calendar fields; their controls are listed under ``TimeDeltaFormat`` in
the :doc:`date and time reference <../../reference/time/date_and_time>`.
