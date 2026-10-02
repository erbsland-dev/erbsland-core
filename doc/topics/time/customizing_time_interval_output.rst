..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Time; Interval Output
    single: TimeDeltaFormat
    single: TimeDeltaUnit

********************************
Customizing Time Interval Output
********************************

The same interval can need several presentations: compact components in a log, long unit names in explanatory text, or
unit aliases in a configuration document.
:cpp:class:`TimeDeltaFormat <erbsland::time::TimeDeltaFormat>` controls those choices for both
:cpp:class:`TimeDelta <erbsland::time::TimeDelta>` and
:cpp:class:`CalendarDelta <erbsland::time::CalendarDelta>`.
You can start with a preset, adjust one option at a time, and reuse the result wherever your application displays an
interval.

Starting with a Preset
======================

Default construction and :cpp:func:`shortUnits() <erbsland::time::TimeDeltaFormat::shortUnits>` produce the same compact
format.
They separate each number from its unit with a space and separate complete components with another space.
Fixed intervals are split from weeks down to nanoseconds; zero components are omitted.
Fractions are initially disabled, and the maximum fraction-digit count is initially zero.

:cpp:func:`longUnits() <erbsland::time::TimeDeltaFormat::longUnits>` keeps those settings and selects English unit names,
with singular or plural forms according to the value.
:cpp:func:`elcl() <erbsland::time::TimeDeltaFormat::elcl>` keeps short units but removes the number-to-unit space and
separates components with ``, ``.
It also selects the aliases understood by ELCL: minutes use ``m`` rather than ``min``, while calendar months and years
use ``month`` and ``year`` rather than ``mo`` and ``y``.

Pass the format to either interval's ``toString()`` method.
The demo displays the same fixed interval and calendar change under all four choices so that you can compare exact
output before customizing anything.
:cpp:func:`usesElclUnitNames() <erbsland::time::TimeDeltaFormat::usesElclUnitNames>` reports whether the ELCL preset's
alias selection is active.
It is a query, rather than an additional formatting switch; the preset selects these aliases.

.. erbsland-demo::
    :source: time/TimeDeltaOutput/Presets.cpp
    :exec: time/time_delta_output --demo Presets
    :source-sha256: 7d571783b7891688242eb1e3517619c955d8a12afec6ddc0e9eebe0e8fb85ac3

.. code-block:: cpp

    /// Render fixed and calendar intervals using default, long, and ELCL presets.
    /// @notest{Compiled and executed documentation demo.}
    void presets() {
        // Keep the interval exact; choose its presentation or conversion separately.
        const auto interval = el::TimeDelta::weeks(1) + el::TimeDelta::days(1) + el::TimeDelta::hours(1) +
            el::TimeDelta::minutes(1) + el::TimeDelta::nanoseconds(1'123'456'789);
        const auto change = el::CalendarDelta{
            el::CalendarDeltaParts{.minutes = el::Minutes{4}, .months = el::Months{2}, .years = el::Years{1}}};
        const auto formats = std::array{
            el::TimeDeltaFormat{},
            el::TimeDeltaFormat::shortUnits(),
            el::TimeDeltaFormat::longUnits(),
            el::TimeDeltaFormat::elcl()};
        const auto labels = std::array{"Default"_el, "Short"_el, "Long"_el, "ELCL"_el};
        // Compare the output for each choice using the same input.
        for (std::size_t index = 0; index < formats.size(); ++index) {
            el::io::printLine(
                el::StringFormat{"{}; ELCL aliases: {}"_el}.build(labels[index], formats[index].usesElclUnitNames()));
            el::io::printLine(interval.toString(formats[index]));
            el::io::printLine(el::StringFormat{"Calendar: {}"_el}.build(change.toString(formats[index])));
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Default; ELCL aliases: false
    1 w 1 d 1 h 1 min 1 s 123 ms 456 us 789 ns
    Calendar: 1 y 2 mo 4 min
    Short; ELCL aliases: false
    1 w 1 d 1 h 1 min 1 s 123 ms 456 us 789 ns
    Calendar: 1 y 2 mo 4 min
    Long; ELCL aliases: false
    1 week 1 day 1 hour 1 minute 1 second 123 milliseconds 456 microseconds 789 nanoseconds
    Calendar: 1 year 2 months 4 minutes
    ELCL; ELCL aliases: true
    1w, 1d, 1h, 1m, 1s, 123ms, 456us, 789ns
    Calendar: 1year, 2month, 4m

.. erbsland-demo-end::

Adjusting Individual Options
============================

The following examples start from a consistent interval containing one week, one day, one hour, one minute, and
``1.123456789`` seconds.
Each subsection isolates a setting, keeping the rest of its format unchanged.
Setters return the format by reference, so you can chain them when you are ready to combine several choices.
The matching getters let you inspect the effective settings.
You do not need to memorize the full set to get started: a preset can handle your first display, and each adjustment
below answers a specific presentation need.


Short or Long Unit Names
------------------------

Set :cpp:func:`setUnitStyle() <erbsland::time::TimeDeltaFormat::setUnitStyle>` to either ``Short`` or ``Long`` from
:cpp:enum:`UnitStyle <erbsland::time::TimeDeltaFormat::UnitStyle>`.
The regular short names are ``w``, ``d``, ``h``, ``min``, ``s``, ``ms``, ``us``, and ``ns``.
Calendar years and months use ``y`` and ``mo``.
Long names are English and use the singular for an integral component of one or minus one, such as ``1 second`` or
``-1 second``; other values use the plural.
A component with a fractional remainder uses a plural name even if its integral part is one.
The separators remain independent of the name style.

.. erbsland-demo::
    :source: time/TimeDeltaOutput/UnitStyle.cpp
    :exec: time/time_delta_output --demo UnitStyle
    :source-sha256: 7ecee25574cd380c3e777bf9a1e1a2ae1dccf30dc38c4bd5ee7e0d3287446d81

.. code-block:: cpp

    /// Choose short abbreviations or singular and plural English unit names.
    /// @notest{Compiled and executed documentation demo.}
    void unitStyle() {
        // Keep the interval exact; choose its presentation or conversion separately.
        const auto interval = el::TimeDelta::weeks(1) + el::TimeDelta::days(1) + el::TimeDelta::hours(1) +
            el::TimeDelta::minutes(1) + el::TimeDelta::nanoseconds(1'123'456'789);
        // Compare the output for each choice using the same input.
        for (const auto style : {el::TimeDeltaFormat::UnitStyle::Short, el::TimeDeltaFormat::UnitStyle::Long}) {
            const auto format = el::TimeDeltaFormat{}.setUnitStyle(style);
            el::io::printLine(interval.toString(format));
            el::io::printLine(
                el::StringFormat{"One: {}; two: {}; negative one: {}"_el}.build(
                    el::TimeDelta::seconds(1).toString(format),
                    el::TimeDelta::seconds(2).toString(format),
                    el::TimeDelta::seconds(-1).toString(format)));
        }
        // Fractional unit values use the plural long name.
        const auto fractional = el::TimeDeltaFormat::longUnits()
                                    .setSmallestUnit(el::TimeDeltaUnit::Seconds)
                                    .setShowFractions(true)
                                    .setMaximumFractionDigits(3);
        el::io::printLine(el::TimeDelta::milliseconds(1250).toString(fractional));
    }

.. erbsland-ansi::
    :escape-char: ␛

    1 w 1 d 1 h 1 min 1 s 123 ms 456 us 789 ns
    One: 1 s; two: 2 s; negative one: -1 s
    1 week 1 day 1 hour 1 minute 1 second 123 milliseconds 456 microseconds 789 nanoseconds
    One: 1 second; two: 2 seconds; negative one: -1 second
    1.25 seconds

.. erbsland-demo-end::

Between a Number and Its Unit
-----------------------------

The value separator sits immediately after the number, including any fractional part, and before its unit name.
:cpp:func:`setValueSeparator() <erbsland::time::TimeDeltaFormat::setValueSeparator>` accepts an ordinary library string.
A space gives ``1 s``, an empty string gives ``1s``, and a colon gives ``1:s``.
This setting changes every displayed component consistently and does not affect the boundary between components.

.. erbsland-demo::
    :source: time/TimeDeltaOutput/ValueSeparator.cpp
    :exec: time/time_delta_output --demo ValueSeparator
    :source-sha256: 46237266b2bb829319d4a3f1513a6ed732852cde8377d3b8e8cf72e8c61d7f0b

.. code-block:: cpp

    /// Customize separators while preserving the same interval components.
    /// @notest{Compiled and executed documentation demo.}
    void valueSeparator() {
        // Keep the interval exact; choose its presentation or conversion separately.
        const auto interval = el::TimeDelta::weeks(1) + el::TimeDelta::days(1) + el::TimeDelta::hours(1) +
            el::TimeDelta::minutes(1) + el::TimeDelta::nanoseconds(1'123'456'789);
        // Compare the output for each choice using the same input.
        for (const auto separator : {" "_el, ""_el, ":"_el}) {
            const auto format = el::TimeDeltaFormat{}.setValueSeparator(separator);
            el::io::printLine(interval.toString(format));
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    1 w 1 d 1 h 1 min 1 s 123 ms 456 us 789 ns
    1w 1d 1h 1min 1s 123ms 456us 789ns
    1:w 1:d 1:h 1:min 1:s 123:ms 456:us 789:ns

.. erbsland-demo-end::

Between Complete Components
---------------------------

The unit separator sits between complete number-and-unit pairs.
:cpp:func:`setUnitSeparator() <erbsland::time::TimeDeltaFormat::setUnitSeparator>` can make a long interval easier to scan
without changing its units.
A space gives ``1 w 2 d``, a comma and space gives ``1 w, 2 d``, and a custom separator gives ``1 w | 2 d``.
There is no leading or trailing unit separator.

.. erbsland-demo::
    :source: time/TimeDeltaOutput/UnitSeparator.cpp
    :exec: time/time_delta_output --demo UnitSeparator
    :source-sha256: bd8f9a07b4ed347641432cd23b23775309f203a22d43e0e44a3dd56754d7a4a0

.. code-block:: cpp

    /// Customize separators while preserving the same interval components.
    /// @notest{Compiled and executed documentation demo.}
    void unitSeparator() {
        // Keep the interval exact; choose its presentation or conversion separately.
        const auto interval = el::TimeDelta::weeks(1) + el::TimeDelta::days(1) + el::TimeDelta::hours(1) +
            el::TimeDelta::minutes(1) + el::TimeDelta::nanoseconds(1'123'456'789);
        // Compare the output for each choice using the same input.
        for (const auto separator : {" "_el, ", "_el, " | "_el}) {
            const auto format = el::TimeDeltaFormat{}.setUnitSeparator(separator);
            el::io::printLine(interval.toString(format));
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    1 w 1 d 1 h 1 min 1 s 123 ms 456 us 789 ns
    1 w, 1 d, 1 h, 1 min, 1 s, 123 ms, 456 us, 789 ns
    1 w | 1 d | 1 h | 1 min | 1 s | 123 ms | 456 us | 789 ns

.. erbsland-demo-end::

The Smallest Displayed Unit
---------------------------

:cpp:func:`setSmallestUnit() <erbsland::time::TimeDeltaFormat::setSmallestUnit>` chooses where the fixed-component
decomposition stops.
:cpp:enum:`TimeDeltaUnit <erbsland::time::TimeDeltaUnit>` offers ``Nanoseconds``, ``Microseconds``, ``Milliseconds``,
``Seconds``, ``Minutes``, ``Hours``, ``Days``, and ``Weeks``.
Larger components are still shown, so choosing seconds does not turn a multi-day interval into a single total second
count.
For a total count instead, :doc:`working_with_time_deltas` covers the value-conversion methods.

With fractions disabled, the remainder below the selected unit is discarded from the output.
For the baseline, stopping at seconds ends with ``1 s``; stopping at minutes ends with ``1 min``.
The demo shows every enum value in order so that you can see exactly which components disappear.
Calendar years and months remain independently visible regardless of this fixed-unit cutoff.

.. erbsland-demo::
    :source: time/TimeDeltaOutput/SmallestUnit.cpp
    :exec: time/time_delta_output --demo SmallestUnit
    :source-sha256: 59285d1e8d2a38d663e45804da904dfc97d89afe3d774dde3f06b86fc1192869

.. code-block:: cpp

    /// Limit component output to each available fixed unit without changing the interval.
    /// @notest{Compiled and executed documentation demo.}
    void smallestUnit() {
        // Keep the interval exact; choose its presentation or conversion separately.
        const auto interval = el::TimeDelta::weeks(1) + el::TimeDelta::days(1) + el::TimeDelta::hours(1) +
            el::TimeDelta::minutes(1) + el::TimeDelta::nanoseconds(1'123'456'789);
        const auto units = std::array{
            el::TimeDeltaUnit::Nanoseconds,
            el::TimeDeltaUnit::Microseconds,
            el::TimeDeltaUnit::Milliseconds,
            el::TimeDeltaUnit::Seconds,
            el::TimeDeltaUnit::Minutes,
            el::TimeDeltaUnit::Hours,
            el::TimeDeltaUnit::Days,
            el::TimeDeltaUnit::Weeks};
        const auto labels = std::array{
            "Nanoseconds"_el,
            "Microseconds"_el,
            "Milliseconds"_el,
            "Seconds"_el,
            "Minutes"_el,
            "Hours"_el,
            "Days"_el,
            "Weeks"_el};
        // Compare the output for each choice using the same input.
        for (std::size_t index = 0; index < units.size(); ++index) {
            const auto format = el::TimeDeltaFormat{}.setSmallestUnit(units[index]);
            el::io::printLine(el::StringFormat{"{}: {}"_el}.build(labels[index], interval.toString(format)));
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Nanoseconds: 1 w 1 d 1 h 1 min 1 s 123 ms 456 us 789 ns
    Microseconds: 1 w 1 d 1 h 1 min 1 s 123 ms 456 us
    Milliseconds: 1 w 1 d 1 h 1 min 1 s 123 ms
    Seconds: 1 w 1 d 1 h 1 min 1 s
    Minutes: 1 w 1 d 1 h 1 min
    Hours: 1 w 1 d 1 h
    Days: 1 w 1 d
    Weeks: 1 w

.. erbsland-demo-end::

Showing the Remainder as a Fraction
-----------------------------------

:cpp:func:`setShowFractions() <erbsland::time::TimeDeltaFormat::setShowFractions>` controls whether the remainder appears
as a decimal fraction of the smallest displayed unit.
That fraction belongs only to the final unit: selecting seconds combines the smaller millisecond, microsecond, and
nanosecond amounts into its decimal part.
Larger components stay integral.

There are two separate decisions here: whether a fraction is useful and how much of it to show.
A nonzero digit allowance is also needed.
The example sets the maximum to nine before toggling fraction visibility, so the enabled form ends with
``1.123456789 s`` and the disabled form ends with ``1 s``.
At nanosecond precision there is no smaller stored remainder to display as a fraction.

.. erbsland-demo::
    :source: time/TimeDeltaOutput/Fractions.cpp
    :exec: time/time_delta_output --demo Fractions
    :source-sha256: accd417356d307d721e13bc5d5da0248d77d91416864e87420be2e5bf2087e10

.. code-block:: cpp

    /// Enable fractional output at the chosen smallest unit with an explicit digit allowance.
    /// @notest{Compiled and executed documentation demo.}
    void fractions() {
        // Keep the interval exact; choose its presentation or conversion separately.
        const auto interval = el::TimeDelta::weeks(1) + el::TimeDelta::days(1) + el::TimeDelta::hours(1) +
            el::TimeDelta::minutes(1) + el::TimeDelta::nanoseconds(1'123'456'789);
        auto format = el::TimeDeltaFormat{}.setSmallestUnit(el::TimeDeltaUnit::Seconds).setMaximumFractionDigits(9);
        el::io::printLine(el::StringFormat{"Fractions off: {}"_el}.build(interval.toString(format)));
        format.setShowFractions(true);
        el::io::printLine(el::StringFormat{"Fractions on: {}"_el}.build(interval.toString(format)));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Fractions off: 1 w 1 d 1 h 1 min 1 s
    Fractions on: 1 w 1 d 1 h 1 min 1.123456789 s

.. erbsland-demo-end::

Limiting Fractional Digits
--------------------------

:cpp:func:`setMaximumFractionDigits() <erbsland::time::TimeDeltaFormat::setMaximumFractionDigits>` limits decimal digits
in the smallest unit's fraction.
Its argument is ``uint8_t``; valid inputs above nine are clamped to nine, as the requested value twelve demonstrates.
A zero limit removes fractional digits even when fraction visibility is enabled.

Digits are truncated, rather than rounded: a two-digit limit turns ``1.123456789`` seconds into ``1.12 s``.
Trailing zeros are removed, so the allowance is a maximum rather than a fixed width.
Whole values have no decimal point.
A nine-digit allowance can preserve every nanosecond in a seconds fraction, while fractions of minutes, hours, days, or
weeks can still need more decimal places; nine digits do not guarantee a lossless representation in those units.

.. erbsland-demo::
    :source: time/TimeDeltaOutput/FractionDigits.cpp
    :exec: time/time_delta_output --demo FractionDigits
    :source-sha256: a6405432fea1aadc88ba34a2a3189d46e9ebeb4f4d7741b58d197c67f427752b

.. code-block:: cpp

    /// Truncate fractional digits and observe the nine-digit upper limit.
    /// @notest{Compiled and executed documentation demo.}
    void fractionDigits() {
        // Keep the interval exact; choose its presentation or conversion separately.
        const auto interval = el::TimeDelta::weeks(1) + el::TimeDelta::days(1) + el::TimeDelta::hours(1) +
            el::TimeDelta::minutes(1) + el::TimeDelta::nanoseconds(1'123'456'789);
        // Compare the output for each choice using the same input.
        for (const auto requested : {0, 2, 3, 6, 9, 12}) {
            const auto format = el::TimeDeltaFormat{}
                                    .setSmallestUnit(el::TimeDeltaUnit::Seconds)
                                    .setShowFractions(true)
                                    .setMaximumFractionDigits(static_cast<uint8_t>(requested));
            el::io::printLine(
                el::StringFormat{"Requested {}; effective {}: {}"_el}.build(
                    requested, format.maximumFractionDigits(), interval.toString(format)));
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Requested 0; effective 0: 1 w 1 d 1 h 1 min 1 s
    Requested 2; effective 2: 1 w 1 d 1 h 1 min 1.12 s
    Requested 3; effective 3: 1 w 1 d 1 h 1 min 1.123 s
    Requested 6; effective 6: 1 w 1 d 1 h 1 min 1.123456 s
    Requested 9; effective 9: 1 w 1 d 1 h 1 min 1.123456789 s
    Requested 12; effective 9: 1 w 1 d 1 h 1 min 1.123456789 s

.. erbsland-demo-end::

Combining Precision Settings
============================

It helps to choose the display unit first, then decide whether its remainder matters, and finally choose how many digits
the reader needs.
A seconds format with fractions enabled and three digits shows ``1250 ms`` as ``1.25 s``.
It shows ``1,999,999,999 ns`` as ``1.999 s``, preserving the decision to truncate rather than round into two seconds.
Negative fractions also truncate toward zero, so ``-1250 ms`` becomes ``-1.25 s``.
For multi-component negative intervals, each nonzero component carries its own minus sign.

If a nonzero remainder is too small to survive the digit limit, fractional mode can display ``0 s`` or ``-0 s``.
The minus sign of a negative sub-unit input remains even when all allowed digits vanish.
Without fractional mode, an interval with no surviving component instead falls back to a plain zero.
Zero itself is displayed as ``0 s`` when the smallest unit is seconds or finer, and in the selected unit when it is
minutes or coarser, such as ``0 min``.
These are display results; none of them means that the original interval has changed.

The demo includes a minutes format as well.
One second becomes ``0.016 min`` at three digits, illustrating why a display fraction can lose information even though
the interval retains its exact nanosecond count.
If you serialize an exact value, retain the necessary components rather than reusing a format designed for a brief
human-readable label.

.. erbsland-demo::
    :source: time/TimeDeltaOutput/Precision.cpp
    :exec: time/time_delta_output --demo Precision
    :source-sha256: 6c92e7253750ae6e7e47f55e6cf05f2364b33b5922489835aeebf4b4138394ef

.. code-block:: cpp

    /// Combine precision settings and inspect truncation, zeros, signs, and trailing zeros.
    /// @notest{Compiled and executed documentation demo.}
    void precision() {
        auto seconds = el::TimeDeltaFormat{}
                           .setSmallestUnit(el::TimeDeltaUnit::Seconds)
                           .setShowFractions(true)
                           .setMaximumFractionDigits(3);
        const auto values = std::array{
            el::TimeDelta::zero(),
            el::TimeDelta::milliseconds(1250),
            el::TimeDelta::milliseconds(-1250),
            el::TimeDelta::nanoseconds(1'999'999'999),
            el::TimeDelta::microseconds(1),
            el::TimeDelta::microseconds(-1)};
        // Compare the output for each choice using the same input.
        for (const auto value : values) {
            el::io::printLine(
                el::StringFormat{"{} ns: {}"_el}.build(value.toNanoseconds().toRawValue(), value.toString(seconds)));
        }
        seconds.setShowFractions(false);
        el::io::printLine(el::StringFormat{"Hidden sub-second input: {}"_el}.build(values.back().toString(seconds)));
        const auto minutes = el::TimeDeltaFormat{}
                                 .setSmallestUnit(el::TimeDeltaUnit::Minutes)
                                 .setShowFractions(true)
                                 .setMaximumFractionDigits(3);
        el::io::printLine(
            el::StringFormat{"One second as minutes: {}; zero as minutes: {}"_el}.build(
                el::TimeDelta::seconds(1).toString(minutes), el::TimeDelta::zero().toString(minutes)));
    }

.. erbsland-ansi::
    :escape-char: ␛

    0 ns: 0 s
    1250000000 ns: 1.25 s
    -1250000000 ns: -1.25 s
    1999999999 ns: 1.999 s
    1000 ns: 0 s
    -1000 ns: -0 s
    Hidden sub-second input: 0 s
    One second as minutes: 0.016 min; zero as minutes: 0 min

.. erbsland-demo-end::

Formatting Calendar Changes
===========================

A ``CalendarDelta`` stores independent amounts rather than one elapsed-time total.
Its output always emits nonzero years first, then months, retaining each amount's sign.
Fourteen months are not folded into a year and two months: those calendar components remain independent because they can
behave differently when applied to a starting date.

The fixed components from nanoseconds through weeks are combined exactly and normalized for display.
For example, ninety minutes and minus one hour produce ``30 min``.
Opposite fixed components can cancel completely, producing ``0 s`` when there are no calendar components, or leaving
only the year and month text when those are present.
The signs of the calendar components can differ from each other and from the fixed total.
A change may therefore show mixed signs even though its fixed section has one consistent direction.

This normalization also works when the fixed total exceeds the range of ``TimeDelta``.
The demo formats the maximum stored week amount while showing that converting it to ``TimeDelta`` fails.
Formatting does not need that conversion.

Read this text as a presentation of the change, rather than a list of replacement parts.
The stored parts remain unchanged.
A cancellation displayed as zero can still have ``isZero() == false`` because its stored minute and hour amounts are
both nonzero.
Likewise, display normalization does not change the order in which ``DateTime`` applies the parts.
The :doc:`date and time reference <../../reference/time/date_and_time>` documents calendar application and conversion;
:doc:`working_with_datetime` shows applying changes to instants.
The final example selects weeks as the smallest fixed display unit and retains the years and months.

.. erbsland-demo::
    :source: time/TimeDeltaOutput/Calendar.cpp
    :exec: time/time_delta_output --demo Calendar
    :source-sha256: 822139e3ebecdca44ff623ba2b8636f107665d888efb8c5353a9e288f88267cd

.. code-block:: cpp

    /// Normalize fixed display components without merging calendar months or changing stored parts.
    /// @notest{Compiled and executed documentation demo.}
    void calendar() {
        const auto change = el::CalendarDelta{el::CalendarDeltaParts{
            .minutes = el::Minutes{90}, .hours = el::Hours{-1}, .months = el::Months{-14}, .years = el::Years{1}}};
        el::io::printLine(
            el::StringFormat{"Short: {}; long: {}"_el}.build(
                change.toString(), change.toString(el::TimeDeltaFormat::longUnits())));
        el::io::printLine(
            el::StringFormat{"Stored minutes: {}; stored hours: {}"_el}.build(
                change.minutes().toRawValue(), change.hours().toRawValue()));
        const auto cancelling =
            el::CalendarDelta{el::CalendarDeltaParts{.minutes = el::Minutes{60}, .hours = el::Hours{-1}}};
        el::io::printLine(
            el::StringFormat{"Cancellation: {}; all stored parts zero: {}"_el}.build(
                cancelling.toString(), cancelling.isZero()));
        const auto wide = el::CalendarDelta{el::Weeks::maximum()};
        el::io::printLine(
            el::StringFormat{"Wide fixed total: {}; fits TimeDelta: {}"_el}.build(
                wide.toString(), wide.toTimeDelta().has_value()));
        const auto weeksOnly = el::TimeDeltaFormat{}.setSmallestUnit(el::TimeDeltaUnit::Weeks);
        el::io::printLine(
            el::StringFormat{"Weeks-only fixed display retains calendar units: {}"_el}.build(change.toString(weeksOnly)));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Short: 1 y -14 mo 30 min; long: 1 year -14 months 30 minutes
    Stored minutes: 90; stored hours: -1
    Cancellation: 0 s; all stored parts zero: false
    Wide fixed total: 9223372036854775807 w; fits TimeDelta: false
    Weeks-only fixed display retains calendar units: 1 y -14 mo

.. erbsland-demo-end::

Reusing a Format Across an Application
======================================

A format is a value you can keep with your presentation settings and pass to multiple ``toString()`` calls.
Chain setters while constructing it, then retain a ``const`` copy so that each displayed interval follows the same
policy.
The demo combines long names, comma separators, seconds as the smallest unit, and a two-digit fractional allowance.
Its separate ELCL call uses the unchanged preset to preserve the expected aliases and punctuation.

ELCL output is a serialization choice rather than merely a more compact display.
A single component forms a delta literal; several comma-separated components form a value list in an ELCL document.
The parser stores individual delta literals as separate ``CalendarDelta`` values and does not automatically combine a
list into one change.
The :doc:`configuration reference <../../reference/conf/configuration_language>` explains typed values and parsing, and
the :doc:`configuration topics <../conf/index>` introduce the document workflow.
Changing separators, choosing long names, or dropping precision can make a presentation format unsuitable for exact
configuration output, even if ``usesElclUnitNames()`` still reports true after customizing an ELCL preset.

For the calculations behind the text, return to :doc:`working_with_time_deltas` for precise fixed intervals or to the
``CalendarDelta`` reference for independent calendar parts.
A format chooses how the reader sees a value; the interval type determines what the value means in a calculation.

.. erbsland-demo::
    :source: time/TimeDeltaOutput/Reuse.cpp
    :exec: time/time_delta_output --demo Reuse
    :source-sha256: a19a0ac85e4fc9a11b7dedcb8616c2b64720733d7bd4e69aa7943487bb0e461c

.. code-block:: cpp

    /// Reuse a chain-configured presentation format and serialize with unchanged ELCL presets.
    /// @notest{Compiled and executed documentation demo.}
    void reuse() {
        const auto display = el::TimeDeltaFormat::longUnits()
                                 .setUnitSeparator(", "_el)
                                 .setSmallestUnit(el::TimeDeltaUnit::Seconds)
                                 .setShowFractions(true)
                                 .setMaximumFractionDigits(2);
        const auto objectives = std::array{"Εξερεύνηση"_el, "Συνεργασία"_el};
        const auto intervals = std::array{el::TimeDelta::milliseconds(1250), el::TimeDelta::milliseconds(2650)};
        // Compare the output for each choice using the same input.
        for (std::size_t index = 0; index < objectives.size(); ++index) {
            el::io::printLine(el::StringFormat{"{}: {}"_el}.build(objectives[index], intervals[index].toString(display)));
        }
        const auto change = el::CalendarDelta{
            el::CalendarDeltaParts{.minutes = el::Minutes{4}, .months = el::Months{2}, .years = el::Years{1}}};
        el::io::printLine(el::StringFormat{"Configuration: {}"_el}.build(change.toString(el::TimeDeltaFormat::elcl())));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Εξερεύνηση: 1.25 seconds
    Συνεργασία: 2.65 seconds
    Configuration: 1year, 2month, 4m

.. erbsland-demo-end::
