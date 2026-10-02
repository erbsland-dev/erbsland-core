..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Time; Literals
    single: Time Amounts; Literal Suffixes

*********************************
Writing Time Values with Literals
*********************************

An interval such as ``250`` tells you little without its unit.
Writing ``250_ms`` keeps that unit beside the value, so a delay, timeout, or arithmetic expression remains readable at
the point where you use it.
The time literals create the same typed amounts as explicit construction; they are a shorter spelling, rather than a
separate kind of time value.

This page explains how to bring the suffixes into scope, choose a unit, and combine amounts without hiding a conversion.
For the conversion and arithmetic rules of the resulting types, continue with :doc:`working_with_time_amounts`.

Bringing the Suffixes into Scope
================================

The operators are declared in ``<erbsland/time/Literals.hpp>`` and belong to ``erbsland::time::literals``.
Including ``<erbsland/time/all.hpp>`` also makes their declarations available.
An include alone does not bring the suffixes into unqualified lookup: a ``using namespace erbsland::time::literals;``
declaration makes them available in the scope where you write it.
Keeping this declaration inside a function is convenient when only that function needs the suffixes.
You can then read an expression such as ``TimeDelta{250_ms}`` directly: it constructs an interval from a count whose
unit is already explicit.

The following complete demo function constructs a :cpp:class:`TimeDelta <erbsland::time::TimeDelta>` from a literal.
Its source includes ``<erbsland/time/Literals.hpp>``; the surrounding demo framework provides the printing facilities
and the ``el`` namespace alias.

.. erbsland-demo::
    :source: time/TimeLiterals/Scope.cpp
    :exec: time/time_literals --demo Scope
    :source-sha256: 30ad9fd80a16cfda8769aafe2e2927cfe324a40ff4c4e8521ed8e07ef5b0c1ca

.. code-block:: cpp

    /// Keep units visible in an interval expression by importing the time literals locally.
    /// @notest{Compiled and executed documentation demo.}
    void scope() {
        using namespace erbsland::time::literals;

        const auto creature = "Drago delle nuvole"_el;
        const auto interval = el::TimeDelta{250_ms};
        el::io::printLine(el::StringFormat{"{}: observation interval {}"_el}.build(creature, interval.toString()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Drago delle nuvole: observation interval 250 ms

.. erbsland-demo-end::

The literal ``250_ms`` itself produces :cpp:type:`Milliseconds <erbsland::time::Milliseconds>`.
It describes a quantity, not a position on a clock and not an instant.
Constructing ``TimeDelta`` then expresses that quantity as a fixed interval with nanosecond resolution.
This distinction becomes useful when the same amount is passed to several APIs: its unit stays attached until you choose
a different representation.

Choosing a Unit and Writing a Signed Amount
===========================================

Each suffix produces a signed integer amount in the unit shown below.
The numeric part is a whole number; ``_m`` means minutes.
There are no day, week, month, or year suffixes in this collection, so those quantities use explicit construction.

.. list-table::
    :header-rows: 1
    :widths: 15 40 45

    * - Suffix
      - Result type
      - Meaning of ``1``
    * - ``_ns``
      - :cpp:type:`Nanoseconds <erbsland::time::Nanoseconds>`
      - One billionth of a second
    * - ``_us``
      - :cpp:type:`Microseconds <erbsland::time::Microseconds>`
      - One millionth of a second
    * - ``_ms``
      - :cpp:type:`Milliseconds <erbsland::time::Milliseconds>`
      - One thousandth of a second
    * - ``_s``
      - :cpp:type:`Seconds <erbsland::time::Seconds>`
      - One second
    * - ``_m``
      - :cpp:type:`Minutes <erbsland::time::Minutes>`
      - Sixty seconds
    * - ``_h``
      - :cpp:type:`Hours <erbsland::time::Hours>`
      - 3,600 seconds

Digit separators make longer values easier to read: ``1'500_ms`` and ``1500_ms`` describe the same amount.
Zero also retains its unit, so ``0_s`` is a zero ``Seconds`` value.
A minus sign is unary negation of the amount returned by the literal: ``-250_ms`` creates a negative millisecond amount.
It is useful for corrections and subtraction, even though a particular timeout API may require a nonnegative argument.

.. erbsland-demo::
    :source: time/TimeLiterals/Units.cpp
    :exec: time/time_literals --demo Units
    :source-sha256: 9272a6043dfe7351058e87bbbf67e492abfa07f21d0917dea212469ad6dbc09b

.. code-block:: cpp

    /// Integer suffixes create typed amounts, including zero and negated values.
    /// @notest{Compiled and executed documentation demo.}
    void units() {
        using namespace erbsland::time::literals;

        const el::Nanoseconds tick = 1_ns;
        const el::Microseconds pulse = 20_us;
        const el::Milliseconds sample = 250_ms;
        const el::Seconds pause = 3_s;
        const el::Minutes observation = 2_m;
        const el::Hours window = 1_h;
        el::io::printLine(
            el::StringFormat{"{} ns; {} us; {} ms; {} s; {} min; {} h"_el}.build(
                tick.toRawValue(),
                pulse.toRawValue(),
                sample.toRawValue(),
                pause.toRawValue(),
                observation.toRawValue(),
                window.toRawValue()));

        // Digit separators change spelling, not the amount or its unit.
        const auto detailedSample = 1'500_ms;
        const auto correction = -250_ms;
        const auto noDelay = 0_s;
        el::io::printLine(
            el::StringFormat{"Sample: {} ms; correction: {} ms; zero: {}"_el}.build(
                detailedSample.toRawValue(), correction.toRawValue(), noDelay.isZero()));
        const auto season = el::Months{3};
        const auto fixedDays = el::Days{7};
        el::io::printLine(
            el::StringFormat{"Explicit amounts: {} months; {} fixed days"_el}.build(
                season.toRawValue(), fixedDays.toRawValue()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    1 ns; 20 us; 250 ms; 3 s; 2 min; 1 h
    Sample: 1500 ms; correction: -250 ms; zero: true
    Explicit amounts: 3 months; 7 fixed days

.. erbsland-demo-end::

Choosing a Representation for an Expression
===========================================

A literal is useful wherever an API accepts its amount type.
You can construct a :cpp:class:`Duration <erbsland::time::Duration>` for whole seconds, a ``TimeDelta`` for a precise
fixed span, or a :cpp:class:`CalendarDelta <erbsland::time::CalendarDelta>` for independent fixed and calendar
components.
The choice is about the receiving API and the precision you need.
For example, a whole-second timeout can be written as ``Duration{2_m}``, while a short measured interval can be written
as ``TimeDelta{250_ms}``.
The demo also passes ``Duration{30_m}`` to a time-of-day calculation and keeps the resulting clock reading.

Adding two amounts of the same type preserves that type: ``250_ms + 250_ms`` produces ``Milliseconds``.
Different amount types have no direct addition operator, even when their units are convertible.
For ``1_s`` and ``250_ms``, you first choose the precision of the result.
Converting the seconds to milliseconds retains both terms; converting the milliseconds to whole seconds would discard
the fractional contribution before the addition.

A second approach is to construct a ``TimeDelta`` from each term and add the intervals.
Both terms then share nanosecond resolution.
This is often convenient when the expression already represents a precise elapsed span rather than a count in one
particular unit.

.. erbsland-demo::
    :source: time/TimeLiterals/Expressions.cpp
    :exec: time/time_literals --demo Expressions
    :source-sha256: 0f291d34a4afb10f55d0090a2431b87c140efde40dd97d56134d3d7afc175e60

.. code-block:: cpp

    /// Pass typed literals to APIs and explicitly choose a common representation for mixed units.
    /// @notest{Compiled and executed documentation demo.}
    void expressions() {
        using namespace erbsland::time::literals;

        // A whole-second interval, a precise interval, and a calendar change have different meanings.
        const auto timeout = el::Duration{2_m};
        const auto latency = el::TimeDelta{250_ms};
        const auto calendar = el::CalendarDelta{1_h}.setMonths(el::Months{1});
        const auto shifted = el::Time{el::Hour{9}, el::Minute{0}}.addedWithWrap(el::Duration{30_m});
        el::io::printLine(
            el::StringFormat{"Timeout: {} s; latency: {}; calendar change: {}; time: {}"_el}.build(
                timeout.toSeconds().toRawValue(), latency.toString(), calendar.toString(), shifted.time.toString()));

        // Same-type arithmetic preserves the type. Convert before adding different amount types.
        const auto twoSamples = 250_ms + 250_ms;
        const auto commonUnit = (1_s).converted<el::Milliseconds>() + 250_ms;
        const auto preciseTotal = el::TimeDelta{1_s} + el::TimeDelta{250_ms};
        const auto mixedCalendar = el::CalendarDelta{el::Months{1}} + el::CalendarDelta{2_h};
        el::io::printLine(
            el::StringFormat{"Samples: {} ms; converted sum: {} ms; precise sum: {}; mixed change: {}"_el}.build(
                twoSamples.toRawValue(), commonUnit.toRawValue(), preciseTotal.toString(), mixedCalendar.toString()));
        // 1_s + 250_ms and Months{1} + 2_h have no amount operator: choose a representation first.
    }

.. erbsland-ansi::
    :escape-char: ␛

    Timeout: 120 s; latency: 250 ms; calendar change: 1 mo 1 h; time: 09:30:00
    Samples: 500 ms; converted sum: 1250 ms; precise sum: 1 s 250 ms; mixed change: 1 mo 2 h

.. erbsland-demo-end::

A month cannot participate in a fixed-unit conversion because its elapsed length depends on a starting date.
``Months{1} + 2_h`` therefore has no amount addition operator.
The demo combines the quantities as calendar deltas instead, preserving a month component and an hour component until
the change is applied to a dated value.
See :doc:`working_with_datetime` and the :doc:`calendar-delta reference <../../reference/time/date_and_time>` for how
these components are applied.

Integer Precision and Representable Bounds
==========================================

The suffixes have integer overloads only.
A floating-point spelling such as ``1.5_s`` does not compile; ``1500_ms`` expresses the same intended quantity using a
finer integer unit.
The literal's numeric token must fit ``unsigned long long`` before the operator can receive it.
Within that input range, a value above the amount's signed 64-bit maximum saturates to ``9,223,372,036,854,775,807``.
This is a bound on the count in the selected unit, not on the count after a later unit conversion.

Negation happens after literal construction.
Consequently, negating an oversized literal gives the negative of the saturated maximum, rather than the minimum signed
value.
If you need the exact lower bound, construct it with the amount type's ``minimum()`` factory.

Even a value that fits its literal's unit can overflow when converted to a finer unit.
For example, ten million hours fit ``Hours`` but exceed the nanosecond range.
Constructing ``TimeDelta`` from that hour amount clamps to its nanosecond maximum.
The amount conversion APIs let you test or reject the conversion first, as explained in
:doc:`working_with_time_amounts`.

.. erbsland-demo::
    :source: time/TimeLiterals/Limits.cpp
    :exec: time/time_literals --demo Limits
    :source-sha256: e1ad263db702da957c3e61f5db9cc6d95c7cc680c516d38b464f95c441f58856

.. code-block:: cpp

    /// Literal saturation and later unit conversion are separate steps with separate precision limits.
    /// @notest{Compiled and executed documentation demo.}
    void limits() {
        using namespace erbsland::time::literals;

        // The token fits unsigned long long, but exceeds the signed amount range.
        const auto oversized = 18'446'744'073'709'551'615_s;
        const auto negated = -18'446'744'073'709'551'615_s;
        el::io::printLine(
            el::StringFormat{"Oversized literal: {}; saturated: {}; then negated: {}"_el}.build(
                oversized.toRawValue(), oversized.isMaximum(), negated.toRawValue()));

        const auto fraction = 1'500_ms;
        const auto seconds = fraction.converted<el::Seconds>();
        const auto negativeSeconds = (-fraction).converted<el::Seconds>();
        el::io::printLine(
            el::StringFormat{"1500 ms in whole seconds: {}; -1500 ms: {}; Duration: {} s"_el}.build(
                seconds.toRawValue(), negativeSeconds.toRawValue(), el::Duration{fraction}.toSeconds().toRawValue()));
        // There is no floating-point suffix: express 1.5 seconds as 1500_ms.

        const auto large = 10'000'000_h;
        el::io::printLine(
            el::StringFormat{"Large hours fit; conversion to nanoseconds would saturate: {}"_el}.build(
                large.wouldConvertSaturate<el::Nanoseconds>()));
        el::io::printLine(
            el::StringFormat{"TimeDelta construction clamps to maximum nanoseconds: {}"_el}.build(
                el::TimeDelta{large}.toNanoseconds().isMaximum()));
        const auto core = 250_ms;
        const auto standard = std::chrono::milliseconds{250};
        el::io::printLine(
            el::StringFormat{"Core and chrono intervals agree: {}"_el}.build(
                el::TimeDelta{core} == el::TimeDelta{standard}));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Oversized literal: 9223372036854775807; saturated: true; then negated: -922337203685477580
    7
    1500 ms in whole seconds: 1; -1500 ms: -1; Duration: 1 s
    Large hours fit; conversion to nanoseconds would saturate: true
    TimeDelta construction clamps to maximum nanoseconds: true
    Core and chrono intervals agree: true

.. erbsland-demo-end::

A conversion to a coarser unit instead drops the remainder toward zero.
Both ``1500_ms`` and ``-1500_ms`` lose half a second when converted to whole seconds, giving ``1`` and ``-1``
respectively.
Constructing ``Duration`` has the same whole-second precision limit; ``TimeDelta`` preserves these milliseconds.
:doc:`working_with_durations` explains how to choose between precision and range when storing an interval.

Erbsland's suffixes belong to ``erbsland::time::literals`` and use an underscore.
The standard library's ``std::chrono_literals`` instead provides spellings such as ``250ms`` and ``2s``, which produce
``std::chrono::duration`` values.
They are different types with different contracts, even when the displayed unit looks familiar.
The demo uses an explicitly constructed chrono value to show the common interval after both are converted to
``TimeDelta``.
Keep the amount or interval type expected by your API visible when crossing that boundary.
