..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Times; Working with Times
    single: Time; Wall-Clock Times
    single: Time; Midnight Wrapping

******************
Working with Times
******************

A time of day describes a position on a daily clock, independently of a date or time zone.
:cpp:class:`Time <erbsland::time::Time>` represents that reading with nanosecond precision, from midnight to just before
the next midnight.

This page explains how to construct and compare times, access their clock fields, and convert them while choosing the
precision you need.
You will also learn how to calculate across midnight and retain the crossed days when working with a
:doc:`date <working_with_dates>`.

A Clock Reading Within One Day
==============================

``Time`` covers ``00:00:00`` through ``23:59:59.999999999`` with nanosecond resolution.
Hours run from 0 to 23, and minutes and seconds from 0 to 59. The fraction belongs to the current second and runs from 0
to 999,999,999 nanoseconds.
There is no separate representation for ``24:00:00`` or a leap second with second number 60.

A ``Time`` has no date or time zone, so ``23:45`` is simply a position on a 24-hour clock.
It does not establish which day you mean or whether the clock is showing UTC or local time.
The nanosecond resolution lets it preserve a fine fractional reading; it does not imply that your source clock can
measure every nanosecond accurately.

Every ``Time`` is a valid time within the day.
Default construction gives midnight, and :cpp:func:`isZero() <erbsland::time::Time::isZero>` tests for that exact value,
including a zero fraction.
Unlike ``Date``, ``Time`` has no invalid sentinel and does not provide an ``isValid()`` test.
If input may be missing, keep that absence separately, for example in a ``std::optional<Time>``.

Constructing a Time
===================

The constructor accepts :cpp:class:`Hour <erbsland::time::Hour>`, :cpp:class:`Minute <erbsland::time::Minute>`, and an
optional :cpp:class:`Second <erbsland::time::Second>` and
:cpp:type:`Nanoseconds <erbsland::time::Nanoseconds>` fraction.
Omitting the second and fraction gives zeros, so ``Time{Hour{23}, Minute{45}}`` is exactly ``23:45:00``.
Copying or moving a time preserves that reading; none of these constructors reads the current clock.

The typed clock parts clamp raw construction values to their respective ranges, and the ``Time`` constructor clamps the
fractional input to ``0..999,999,999`` as well.
For example, hour 24 becomes 23 rather than rolling forward to midnight.
When you need to validate supplied numbers, check the part types' ``contains()`` methods and the fraction's range before
constructing the time.
Construction itself does not report malformed input.
The demo checks an hour of 24 before it can become a clamped ``Hour``.

:cpp:func:`first() <erbsland::time::Time::first>` is midnight, matching the default value.
:cpp:func:`last() <erbsland::time::Time::last>` is one nanosecond before the next midnight.
These mark the ends of a clock day, without identifying any particular date.

An elapsed amount follows a different rule from a clock field.
:cpp:func:`fromDurationSinceMidnight() <erbsland::time::Time::fromDurationSinceMidnight>` accepts either a
:cpp:class:`Duration <erbsland::time::Duration>` in whole seconds or a
:cpp:class:`TimeDelta <erbsland::time::TimeDelta>` with nanosecond precision.
It wraps negative amounts and amounts of one day or more into the clock's range: 25 hours becomes ``01:00:00``, and
minus one nanosecond becomes the last time of day.
This factory retains only the clock reading; use wrapping arithmetic when you also need the crossed days.

.. erbsland-demo::
    :source: time/Time/Create.cpp
    :exec: time/time --demo Create
    :source-sha256: 315fa760056a963341b0646bdbb91ca5613da1bce440943f39f15d6cc3898d6e

.. code-block:: cpp

    /// Build observation times and wrap elapsed values into a day.
    /// @notest{Compiled and executed documentation demo.}
    void create() {
        const auto observation = "Observation de Saturne"_el;
        const auto midnight = el::Time{};
        const auto start = el::Time{el::Hour{23}, el::Minute{45}};
        const auto precise = el::Time{el::Hour{23}, el::Minute{45}, el::Second{12}, el::Nanoseconds{123456789}};
        el::io::printLine(el::StringFormat{"{}: {}; precise reading: {}"_el}.build(observation, start, precise));
        el::io::printLine(
            el::StringFormat{"Default: {}; first: {}; last: {}"_el}.build(midnight, el::Time::first(), el::Time::last()));

        // Factories wrap elapsed amounts into a time within the day.
        const auto afterDay = el::Time::fromDurationSinceMidnight(el::Duration{el::Hours{25}});
        const auto beforeMidnight = el::Time::fromDurationSinceMidnight(el::TimeDelta{el::Nanoseconds{-1}});
        el::io::printLine(el::StringFormat{"25 hours: {}; minus one nanosecond: {}"_el}.build(afterDay, beforeMidnight));
        const auto clamped = el::Time{el::Hour{24}, el::Minute{60}, el::Second{60}, el::Nanoseconds{1000000000}};
        el::io::printLine(el::StringFormat{"Clamped fields: {}"_el}.build(clamped));

        // Check external fields before constructing clamped clock parts.
        const auto suppliedHour = 24;
        const auto suppliedMinute = 45;
        const auto suppliedSecond = 0;
        const auto suppliedFraction = el::Nanoseconds{0};
        const auto fieldsAccepted = el::Hour::contains(suppliedHour) && el::Minute::contains(suppliedMinute) &&
            el::Second::contains(suppliedSecond) && suppliedFraction >= el::Nanoseconds{0} &&
            suppliedFraction < el::Nanoseconds{1000000000};
        el::io::printLine(el::StringFormat{"External fields accepted: {}"_el}.build(fieldsAccepted));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Observation de Saturne: 23:45:00; precise reading: 23:45:12.123456789
    Default: 00:00:00; first: 00:00:00; last: 23:59:59.999999999
    25 hours: 01:00:00; minus one nanosecond: 23:59:59.999999999
    Clamped fields: 23:59:59.999999999
    External fields accepted: false

.. erbsland-demo-end::


Comparing Times and Accessing Clock Fields
==========================================

Times support equality and ordering by their position within the day, including fractional seconds.
Ordering follows the clock reading: ``01:15`` sorts before ``23:45``.
It does not establish the order of instants on different dates.
A reading after midnight can belong to a later date even though its clock value sorts before a reading from the previous
day.
When calculating across midnight, retain the day carry alongside the time to keep track of that date change.

The :cpp:func:`hour() <erbsland::time::Time::hour>`, :cpp:func:`minute() <erbsland::time::Time::minute>`, and
:cpp:func:`second() <erbsland::time::Time::second>` accessors return the individual clock parts.
:cpp:func:`nanosecondFraction() <erbsland::time::Time::nanosecondFraction>` returns the full fraction of the
current second.
:cpp:func:`millisecondFraction() <erbsland::time::Time::millisecondFraction>` returns that fraction truncated to
whole milliseconds, rather than a count since midnight.
For a fraction of ``123,456,789`` nanoseconds, these readings are ``123,456,789`` and ``123`` respectively.

:cpp:func:`parts() <erbsland::time::Time::parts>` gathers the hour, minute, second, and nanosecond fraction into a
:cpp:struct:`TimeParts <erbsland::time::TimeParts>` with named fields.
It is a convenient way to pass a complete set of fields to a display or reconstruct the same time.
For a ``Time``, the fields already come from a simple clock reading, so prefer ``parts()`` for clarity when you need all
four; individual accessors remain appropriate when you only need one.
This differs from ``Date::parts()``, where gathering the calendar fields also avoids repeated calendar extraction.

.. erbsland-demo::
    :source: time/Time/Inspect.cpp
    :exec: time/time --demo Inspect
    :source-sha256: 8ef1a920ecb0bea50c1b3919ee5bd2c600d41668106581e63dd4bcfb9139580c

.. code-block:: cpp

    /// Compare wall-clock times and inspect fractional seconds.
    /// @notest{Compiled and executed documentation demo.}
    void inspect() {
        const auto reading = el::Time{el::Hour{23}, el::Minute{45}, el::Second{12}, el::Nanoseconds{123456789}};
        const auto parts = reading.parts();
        el::io::printLine(
            el::StringFormat{"Hour: {}; minute: {}; second: {}; fraction: {} ns"_el}.build(
                parts.hour.toValue(),
                parts.minute.toValue(),
                parts.second.toValue(),
                parts.nanosecondFraction.toRawValue()));
        el::io::printLine(
            el::StringFormat{"Individual fields: {} / {} / {}"_el}.build(
                reading.hour().toValue(), reading.minute().toValue(), reading.second().toValue()));
        el::io::printLine(
            el::StringFormat{"Millisecond fraction: {}; nanosecond fraction: {}"_el}.build(
                reading.millisecondFraction().toRawValue(), reading.nanosecondFraction().toRawValue()));
        el::io::printLine(
            el::StringFormat{"Midnight is zero: {}; reading is zero: {}"_el}.build(el::Time{}.isZero(), reading.isZero()));
        el::io::printLine(
            el::StringFormat{"Midnight sorts before evening: {}; same time equals: {}"_el}.build(
                el::Time{} < reading,
                reading == el::Time{parts.hour, parts.minute, parts.second, parts.nanosecondFraction}));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Hour: 23; minute: 45; second: 12; fraction: 123456789 ns
    Individual fields: 23 / 45 / 12
    Millisecond fraction: 123; nanosecond fraction: 123456789
    Midnight is zero: true; reading is zero: false
    Midnight sorts before evening: true; same time equals: true

.. erbsland-demo-end::


Converting a Reading Without Losing Its Meaning
===============================================

A component and a total answer different questions.
``second()`` tells you the second within the current minute; the
:cpp:func:`toSecondsSinceMidnight() <erbsland::time::Time::toSecondsSinceMidnight>` result is the total count from
midnight to the reading.
It truncates any fraction of the final second.
:cpp:func:`toNanosecondsSinceMidnight() <erbsland::time::Time::toNanosecondsSinceMidnight>` returns the complete
count with nanosecond precision.
These counts have a midnight origin, without an epoch date or a UTC offset.

You can also retain the result as an interval object.
:cpp:func:`durationSinceMidnight() <erbsland::time::Time::durationSinceMidnight>` returns ``Duration`` and discards
sub-second precision.
:cpp:func:`timeDeltaSinceMidnight() <erbsland::time::Time::timeDeltaSinceMidnight>` returns ``TimeDelta`` and
preserves it.
Passing the precise result back to ``fromDurationSinceMidnight()`` reconstructs the original reading; the whole-second
result reconstructs its truncated counterpart.

For text, :cpp:func:`toString() <erbsland::time::Time::toString>` always includes hours, minutes, and seconds.
A nonzero fraction follows a dot and has its trailing zeros removed.
For example, 120 milliseconds displays as ``.12``; a zero fraction has no fractional suffix.
The default :cpp:func:`toIsoString() <erbsland::time::Time::toIsoString>` stops at whole seconds, even when the object
holds a fraction.
Choose :cpp:enum:`DateTimePrecision <erbsland::time::DateTimePrecision>` explicitly when your output must retain
milliseconds, microseconds, or nanoseconds.

ISO fractional output uses a comma by default; the ``UseDotFraction`` flag selects a dot.
At a requested fractional precision, ISO output has a fixed number of digits, including trailing zeros.
Lower precision truncates the smaller digits, without changing the stored time.
See :ref:`time-iso-output` for the shared formatting types.

.. erbsland-demo::
    :source: time/Time/Convert.cpp
    :exec: time/time --demo Convert
    :source-sha256: cd4b9cfd5b83495983f6cc2dc5b116cb1c6afa9468ce97e307c2bf5e544573ac

.. code-block:: cpp

    /// Preserve or discard fractional precision when converting times.
    /// @notest{Compiled and executed documentation demo.}
    void convert() {
        const auto reading = el::Time{el::Hour{23}, el::Minute{45}, el::Second{12}, el::Nanoseconds{123456789}};
        el::io::printLine(
            el::StringFormat{"Whole seconds since midnight: {}"_el}.build(reading.toSecondsSinceMidnight().toRawValue()));
        el::io::printLine(
            el::StringFormat{"Nanoseconds since midnight: {}"_el}.build(reading.toNanosecondsSinceMidnight().toRawValue()));
        const auto whole = reading.durationSinceMidnight();
        const auto precise = reading.timeDeltaSinceMidnight();
        el::io::printLine(
            el::StringFormat{"Duration round trip: {}; TimeDelta round trip: {}"_el}.build(
                el::Time::fromDurationSinceMidnight(whole), el::Time::fromDurationSinceMidnight(precise)));
        el::io::printLine(
            el::StringFormat{"Display: {}; default ISO: {}"_el}.build(reading.toString(), reading.toIsoString()));
        const auto format = el::IsoTimeFormatFlags{el::IsoTimeFormat::Extended, el::IsoTimeFormat::UseDotFraction};
        el::io::printLine(
            el::StringFormat{"Nanosecond ISO: {}"_el}.build(
                reading.toIsoString(format, el::DateTimePrecision::Nanosecond)));
        el::io::printLine(
            el::StringFormat{"Millisecond ISO: {}"_el}.build(
                reading.toIsoString(format, el::DateTimePrecision::Millisecond)));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Whole seconds since midnight: 85512
    Nanoseconds since midnight: 85512123456789
    Duration round trip: 23:45:12; TimeDelta round trip: 23:45:12.123456789
    Display: 23:45:12.123456789; default ISO: 23:45:12
    Nanosecond ISO: 23:45:12.123456789
    Millisecond ISO: 23:45:12.123

.. erbsland-demo-end::


Calculating Across Midnight
===========================

Adding 90 minutes to ``23:45`` produces ``01:15`` on the next day.
:cpp:func:`addedWithWrap() <erbsland::time::Time::addedWithWrap>` returns a
:cpp:struct:`TimeWrapResult <erbsland::time::TimeWrapResult>` containing both ``time``, the resulting reading,
and ``days``, a signed :cpp:type:`Days <erbsland::time::Days>` carry.
The original time remains unchanged.
Both ``Duration`` and ``TimeDelta`` are accepted, so you can choose whole seconds or preserve fractional seconds.

The day carry is positive when the change crosses midnight forward, negative when it crosses backward, and zero when the
result stays in the same day.
It can represent more than one crossed day: adding 49 hours to ``23:45`` produces ``00:45`` with a carry of three.
Even an exact result at midnight belongs to the following day when the addition reaches it from the previous one.
Moving one nanosecond backward from midnight yields ``23:59:59.999999999`` with carry minus one.

:cpp:func:`addWithWrap() <erbsland::time::Time::addWithWrap>` changes the time in place and returns only the day
carry, since the new clock reading is already in the object.
Keep that return value whenever you are also tracking a date.
Discarding it loses the date change even though the resulting clock reading remains valid.

.. erbsland-demo::
    :source: time/Time/Wrap.cpp
    :exec: time/time --demo Wrap
    :source-sha256: 3a7f11f611177617945ca55d6580707759aa8ff3c5acd100895dea8fc96b3712

.. code-block:: cpp

    /// Carry crossed days when an observation continues past midnight.
    /// @notest{Compiled and executed documentation demo.}
    void wrap() {
        const auto date = el::Date::fromYearMonthDayOrThrow(2028, 2, 29);
        const auto start = el::Time{el::Hour{23}, el::Minute{45}};
        const auto finished = start.addedWithWrap(el::Duration{el::Minutes{90}});
        el::io::printLine(
            el::StringFormat{"Finish: {} {}; day carry: {}"_el}.build(
                date.addedOrThrow(finished.days), finished.time, finished.days.toRawValue()));
        el::io::printLine(el::StringFormat{"Start remains: {}"_el}.build(start));

        auto revised = start;
        const auto days = revised.addWithWrap(el::Duration{el::Hours{49}});
        el::io::printLine(el::StringFormat{"49 hours later: {}; day carry: {}"_el}.build(revised, days.toRawValue()));
        const auto previous = el::Time{}.addedWithWrap(el::TimeDelta{el::Nanoseconds{-1}});
        el::io::printLine(
            el::StringFormat{"One nanosecond before midnight: {}; day carry: {}"_el}.build(
                previous.time, previous.days.toRawValue()));
        auto midnight = el::Time{};
        const auto previousDays = midnight.addWithWrap(el::TimeDelta{el::Minutes{-90}});
        el::io::printLine(
            el::StringFormat{"Minus 90 minutes in place: {}; day carry: {}"_el}.build(midnight, previousDays.toRawValue()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Finish: 2028-03-01 01:15:00; day carry: 1
    Start remains: 23:45:00
    49 hours later: 00:45:00; day carry: 3
    One nanosecond before midnight: 23:59:59.999999999; day carry: -1
    Minus 90 minutes in place: 22:30:00; day carry: -1

.. erbsland-demo-end::


In the wrapping demo, the first calculation combines the wrapped reading with ``date.addedOrThrow(finished.days)``.
This checks the date boundary independently: a valid clock reading can still require a date beyond the supported
calendar.
A saturating ``date.added()`` is available when keeping the date at its supported boundary is appropriate instead.

This arithmetic describes a clock with 24-hour days.
It does not resolve daylight-saving transitions, ambiguous local times, or leap seconds.
When a recorded time must refer to a real event, keep the date and zone together in
:cpp:class:`DateTime <erbsland::time::DateTime>`.
For very large additions, the supplied interval's own representable range still matters; wrapping is not a way to
recover precision or magnitude lost while constructing an interval.
Prefer ``Duration`` for large whole-second spans and ``TimeDelta`` for precise intervals within its nanosecond range.

Choosing a Dated or Zoned Value
===============================

:doc:`working_with_dates` explains how to construct, validate, and calculate with calendar dates.
A :cpp:class:`TimeWithZone <erbsland::time::TimeWithZone>` keeps a daily wall-clock reading with its intended zone until
you choose a date; ``DateTime`` then resolves a dated instant.
For measuring how long an operation takes, choose :cpp:class:`ElapsedTimer <erbsland::time::ElapsedTimer>` or
:cpp:class:`TimePoint <erbsland::time::TimePoint>` instead of subtracting daily clock readings.
The :doc:`overview` helps you select these types, and :doc:`../../reference/time/date_and_time` lists the exact ``Time``
and ``TimeWrapResult`` interfaces.
