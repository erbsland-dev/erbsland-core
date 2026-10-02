..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Time; Typed Amounts
    single: Time Amounts; Conversion
    single: Time Amounts; Arithmetic

*******************************
Working with Typed Time Amounts
*******************************

A delay of 250 milliseconds and a count of three months are both quantities, but they belong in different calculations.
Typed time amounts keep that distinction in the value itself.
A parameter of type ``Milliseconds`` tells a caller which unit to supply, and a parameter of type ``Months`` preserves
the calendar meaning that a plain integer would hide.

This page develops those quantities from construction through conversion and arithmetic.
It also explains where fixed units stop being interchangeable and why calendar changes need a date before they acquire
an elapsed length.

A Quantity Is Different from a Calendar Position
================================================

:cpp:class:`Day <erbsland::time::Day>` names a day of the month: ``Day{15}`` selects its fifteenth day.
:cpp:type:`Days <erbsland::time::Days>` names a quantity: ``Days{15}`` describes fifteen days.
Likewise, :cpp:class:`Hour <erbsland::time::Hour>` selects a clock field in ``Hour{9}``, while ``Hours{9}`` describes
nine hours.
A :cpp:class:`Month <erbsland::time::Month>` identifies a calendar month, while
:cpp:type:`Months <erbsland::time::Months>` counts a change measured in months.

These types let a function's signature distinguish a requested clock reading from a delay.
They also prevent unrelated values from being substituted merely because their numeric counts happen to match.
A count of hours cannot silently fill a parameter that expects a clock-hour field.
When you build or inspect a date or time, its parts describe positions; when you calculate a change, amounts describe
quantities.
:doc:`working_with_dates` and :doc:`working_with_times` show those parts in complete values.

The creation demo below places matching positions and quantities side by side, then uses quantities to construct
intervals.
Notice that the same integer can have a different purpose depending on its type.

The amount header is ``<erbsland/time/TimeAmounts.hpp>``.
All the amount types store a signed 64-bit integer count, from ``-9,223,372,036,854,775,808`` through
``9,223,372,036,854,775,807`` in their own unit.
They use :cpp:class:`IntegerAmount <erbsland::unit::IntegerAmount>` for common arithmetic and conversion behavior.

A Map of the Units
==================

The fixed units share a seconds-based dimension and differ only in scale.
Months and years have separate dimensions because their effect belongs to calendar arithmetic.

.. list-table::
    :header-rows: 1
    :widths: 35 30 35

    * - Amount type
      - Size of one unit
      - Conversion group
    * - :cpp:type:`Nanoseconds <erbsland::time::Nanoseconds>`
      - One billionth of a second
      - Fixed units
    * - :cpp:type:`Microseconds <erbsland::time::Microseconds>`
      - One millionth of a second
      - Fixed units
    * - :cpp:type:`Milliseconds <erbsland::time::Milliseconds>`
      - One thousandth of a second
      - Fixed units
    * - :cpp:type:`Seconds <erbsland::time::Seconds>`
      - One second
      - Fixed units
    * - :cpp:type:`Minutes <erbsland::time::Minutes>`
      - Sixty seconds
      - Fixed units
    * - :cpp:type:`Hours <erbsland::time::Hours>`
      - 3,600 seconds
      - Fixed units
    * - ``Days``
      - 86,400 seconds
      - Fixed units
    * - :cpp:type:`Weeks <erbsland::time::Weeks>`
      - Seven fixed days
      - Fixed units
    * - ``Months``
      - A calendar month
      - Months only
    * - :cpp:type:`Years <erbsland::time::Years>`
      - A calendar year
      - Years only

A fixed day is always 86,400 seconds in a unit conversion.
That definition does not promise the same local wall-clock reading after adding a day to an instant: a daylight-saving
offset change can shift its local display.
The choice between elapsed time and a recurring local appointment belongs to the calculation that applies the amount;
see :doc:`working_with_datetime` for UTC arithmetic and local-time resolution.

Months and years do not convert to fixed units or to each other through the amount conversion API.
Even a relation such as twelve months and one year should remain a calendar decision: applying a change can encounter
month-end or leap-day adjustment.
A ``CalendarDelta`` lets you retain those components without choosing an artificial seconds-per-month value.

Creating and Inspecting an Amount
=================================

Default construction produces zero, making an amount a convenient starting value for an accumulated total.
Explicit construction accepts a signed count, including negative values, and the suffixes in
:doc:`writing_time_literals` provide a concise spelling for units from nanoseconds through hours.
Keeping the amount typed as it moves through your code makes each later conversion a visible choice.
Construction from an unsigned integer is deliberately unavailable; when a count arrives as unsigned input, check its
range and convert it to a suitable signed count before constructing the amount.

The queries ``isZero()``, ``isPositive()``, and ``isNegative()`` describe the count's sign.
They do not establish that it is suitable for every caller: a negative correction can be meaningful in arithmetic, while
a delay parameter may require zero or more.

.. erbsland-demo::
    :source: time/TimeAmounts/Create.cpp
    :exec: time/time_amounts --demo Create
    :source-sha256: 2ee083ace58233e1a62ae91a7da2a340c43304740788eed8df65b56f490eb04e

.. code-block:: cpp

    /// Construct signed quantities and keep their units separate from calendar positions and raw counts.
    /// @notest{Compiled and executed documentation demo.}
    void create() {
        using namespace erbsland::time::literals;

        const auto creature = "Fenice del bosco"_el;
        const auto empty = el::Milliseconds{};
        const auto delay = el::Milliseconds{250};
        const auto correction = el::Milliseconds{-25};
        const auto fromLiteral = 250_ms;
        el::io::printLine(
            el::StringFormat{"{}: delay {} ms; default zero: {}; positive: {}; negative: {}; literal equal: {}"_el}.build(
                creature,
                delay.toRawValue(),
                empty.isZero(),
                delay.isPositive(),
                correction.isNegative(),
                delay == fromLiteral));

        // toValue() retains saturating integer behavior, but no longer carries the time unit.
        const auto raw = delay.toRawValue();
        const auto value = delay.toValue();
        el::io::printLine(el::StringFormat{"Native count: {}; saturating count: {}"_el}.build(raw, value.toRawValue()));

        const auto position = el::Day{15};
        const auto quantity = el::Days{15};
        const auto clockHour = el::Hour{9};
        const auto elapsedHours = el::Hours{9};
        const auto calendarMonth = el::Month::march();
        const auto monthCount = el::Months{3};
        el::io::printLine(
            el::StringFormat{
                "Day field: {}; day count: {}; hour field: {}; hour count: {}; month field: {}; month count: {}"_el}
                .build(
                    position.toRawValue(),
                    quantity.toRawValue(),
                    clockHour.toRawValue(),
                    elapsedHours.toRawValue(),
                    calendarMonth.toRawValue(),
                    monthCount.toRawValue()));

        const auto whole = el::Duration{el::Minutes{2}};
        const auto precise = el::TimeDelta{delay};
        const auto calendar = el::CalendarDelta{monthCount};
        el::io::printLine(
            el::StringFormat{"Whole interval: {} s; precise interval: {}; calendar change: {}"_el}.build(
                whole.toSeconds().toRawValue(), precise.toString(), calendar.toString()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Fenice del bosco: delay 250 ms; default zero: true; positive: true; negative: true; litera
    l equal: true
    Native count: 250; saturating count: 250
    Day field: 15; day count: 15; hour field: 9; hour count: 9; month field: 3; month count: 3
    Whole interval: 120 s; precise interval: 250 ms; calendar change: 3 mo

.. erbsland-demo-end::

``toRawValue()`` returns the native signed integer count in the amount's current unit.
It does not convert milliseconds to seconds.
``toValue()`` instead returns the :cpp:class:`SaturatingInteger <erbsland::math::SaturatingInteger>` wrapper, retaining
saturating integer behavior but discarding the time-unit type.
Both accessors are useful at an interface boundary; retaining ``Milliseconds`` or another amount is clearer while the
quantity still participates in time calculations.

An amount can also initialize an interval.
:doc:`working_with_durations` covers ``Duration`` for whole seconds;
:cpp:class:`TimeDelta <erbsland::time::TimeDelta>` retains nanoseconds; and
:cpp:class:`CalendarDelta <erbsland::time::CalendarDelta>` keeps independent fixed and calendar components.
The :doc:`date and time reference <../../reference/time/date_and_time>` describes the latter two interfaces.
``Duration`` and ``TimeDelta`` accept seconds-based amounts, while ``CalendarDelta`` also accepts ``Months`` and
``Years``.

Converting to a Compatible Unit
===============================

Conversion starts with a target type: ``amount.converted<Milliseconds>()`` produces a new amount in milliseconds.
The source remains unchanged.
Among the fixed time units, choosing a finer unit multiplies the count and choosing a coarser unit divides it.
``Seconds{2}`` therefore becomes ``Milliseconds{2000}``, and ``Weeks{1}`` becomes ``Days{7}``.

Integer division truncates toward zero.
Converting ``Milliseconds{1500}`` to ``Seconds`` gives one second, and converting ``Milliseconds{-1500}`` gives minus
one second.
An amount smaller than one target unit becomes zero, with the same rule for positive and negative values.
The demo includes ``999`` and ``-999`` milliseconds so you can see this behavior around zero as well as for a larger
fractional second.
Once that remainder has been discarded, converting back cannot recover it.
Choose a common unit fine enough for your calculation before combining inputs.

.. erbsland-demo::
    :source: time/TimeAmounts/Convert.cpp
    :exec: time/time_amounts --demo Convert
    :source-sha256: 2b0c29ea529e54e16bf07140230943f2a436d5425ae5a4fc5fbec9c8447e136e

.. code-block:: cpp

    /// Choose a common fixed unit, recognizing truncation and checking finer-unit overflow.
    /// @notest{Compiled and executed documentation demo.}
    void convert() {
        const auto delay = el::Seconds{2}.converted<el::Milliseconds>();
        const auto week = el::Weeks{1}.converted<el::Days>();
        el::io::printLine(
            el::StringFormat{"2 s: {} ms; one week: {} days; one day: {} seconds"_el}.build(
                delay.toRawValue(), week.toRawValue(), el::Days{1}.converted<el::Seconds>().toRawValue()));

        for (const auto input : {1500, -1500, 999, -999}) {
            const auto amount = el::Milliseconds{input};
            const auto seconds = amount.convertedOrThrow<el::Seconds>();
            el::io::printLine(
                el::StringFormat{"{} ms: {} whole seconds; would saturate: {}"_el}.build(
                    input, seconds.toRawValue(), amount.wouldConvertSaturate<el::Seconds>()));
        }

        const auto large = el::Seconds::maximum();
        const auto saturated = large.converted<el::Nanoseconds>();
        el::io::printLine(
            el::StringFormat{"Maximum seconds to ns would saturate: {}; clamped result: {}"_el}.build(
                large.wouldConvertSaturate<el::Nanoseconds>(), saturated.toRawValue()));
        try {
            const auto checked = large.convertedOrThrow<el::Nanoseconds>();
            el::io::printLine(el::StringFormat{"Checked result: {}"_el}.build(checked.toRawValue()));
        } catch (const el::err::OverflowError &) {
            el::io::printLine("Checked conversion rejected overflow."_el);
        }
        // Months and years cannot be converted to fixed units or to each other.
    }

.. erbsland-ansi::
    :escape-char: ␛

    2 s: 2000 ms; one week: 7 days; one day: 86400 seconds
    1500 ms: 1 whole seconds; would saturate: false
    -1500 ms: -1 whole seconds; would saturate: false
    999 ms: 0 whole seconds; would saturate: false
    -999 ms: 0 whole seconds; would saturate: false
    Maximum seconds to ns would saturate: true; clamped result: 9223372036854775807
    Checked conversion rejected overflow.

.. erbsland-demo-end::

Three conversion forms let you decide how to handle the target's bounds:

* ``converted<T>()`` clamps an overflowing result to the target minimum or maximum.
* ``wouldConvertSaturate<T>()`` reports whether the conversion would exceed those bounds.
* ``convertedOrThrow<T>()`` throws :cpp:class:`OverflowError <erbsland::err::OverflowError>` when the conversion would
  saturate.

The throwing form checks overflow, not exact divisibility.
It still converts 1,500 milliseconds to one whole second, and the saturation query returns ``false`` for that precision
loss.
If discarding a remainder would be an error in your application, keep a finer unit or explicitly check that the
converted value converts back to the original amount.

Attempting ``Months{1}.converted<Seconds>()`` or ``Years{1}.converted<Months>()`` fails to compile because these amount
types have different unit dimensions.
This helps keep calendar interpretation out of a calculation that only expects a fixed scale conversion.

Calculating and Comparing in One Unit
=====================================

Arithmetic and comparison between amounts require the exact same type.
Two ``Milliseconds`` values can be added, subtracted, or ordered directly, and the arithmetic result remains
``Milliseconds``.
A ``Seconds`` value must first be converted when it participates in that calculation.
This also means a comparison needs a common unit: comparing a raw count of one second with a raw count of 250
milliseconds would compare the numbers rather than the quantities.

Multiplication and division take signed integer operands and preserve the amount type.
Multiplying by three produces three times the quantity; dividing by four produces a whole-number share in the same unit.
Division truncates toward zero, so a share of 1,001 milliseconds is 250 milliseconds and a share of -1,001 milliseconds
is -250 milliseconds.
The in-place forms ``+=``, ``-=``, ``*=``, and ``/=`` update the original amount.

.. erbsland-demo::
    :source: time/TimeAmounts/Calculate.cpp
    :exec: time/time_amounts --demo Calculate
    :source-sha256: 7dbfb3f665a7627432c7b4f42568ed0d86f0edf6a1e7a7aed1a6b89fba73fe92

.. code-block:: cpp

    /// Compare like units and calculate signed totals with integer scaling and division.
    /// @notest{Compiled and executed documentation demo.}
    void calculate() {
        const auto sample = el::Milliseconds{250};
        const auto pause = el::Seconds{1}.converted<el::Milliseconds>();
        const auto budget = el::Milliseconds{2000};
        auto required = sample * 3 + pause;
        required += el::Milliseconds{100};
        required -= el::Milliseconds{50};
        el::io::printLine(
            el::StringFormat{"Required: {} ms; remaining: {} ms; within budget: {}; equal budget: {}"_el}.build(
                required.toRawValue(), (budget - required).toRawValue(), required <= budget, required == budget));

        const auto correction = -sample;
        const auto share = el::Milliseconds{1001} / 4;
        const auto negativeShare = el::Milliseconds{-1001} / 4;
        el::io::printLine(
            el::StringFormat{"Correction: {} ms; share: {} ms; negative share: {} ms; cancelled: {}"_el}.build(
                correction.toRawValue(), share.toRawValue(), negativeShare.toRawValue(), (sample + correction).isZero()));
        auto scaled = sample;
        scaled *= 4;
        scaled /= 2;
        el::io::printLine(
            el::StringFormat{"Scaled in place: {} ms; same as two samples: {}"_el}.build(
                scaled.toRawValue(), scaled == sample + sample));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Required: 1800 ms; remaining: 200 ms; within budget: true; equal budget: false
    Correction: -250 ms; share: 250 ms; negative share: -250 ms; cancelled: true
    Scaled in place: 500 ms; same as two samples: true

.. erbsland-demo-end::

Boundary checks matter when quantities come from configuration, repeated accumulation, or very large conversions.
Amount arithmetic saturates at the signed count's bounds instead of wrapping around or throwing an exception.
``wouldAddSaturate()``, ``wouldSubtractSaturate()``, and ``wouldMultiplySaturate()`` let you check those operations
before performing them.
The ``minimum()`` and ``maximum()`` factories give you the bounds in the amount's current unit.

.. erbsland-demo::
    :source: time/TimeAmounts/Boundaries.cpp
    :exec: time/time_amounts --demo Boundaries
    :source-sha256: ed2f0e3bf5bf37c5db5674d73dfe63fad3875fdac7f4f876493de0efb85f1ebd

.. code-block:: cpp

    /// Check saturating addition, subtraction, multiplication, and minimum-value negation.
    /// @notest{Compiled and executed documentation demo.}
    void boundaries() {
        const auto maximum = el::Seconds::maximum();
        const auto minimum = el::Seconds::minimum();
        const auto one = el::Seconds{1};
        el::io::printLine(
            el::StringFormat{"Add would saturate: {}; subtract would saturate: {}; multiply would saturate: {}"_el}.build(
                maximum.wouldAddSaturate(one), minimum.wouldSubtractSaturate(one), maximum.wouldMultiplySaturate(2)));
        el::io::printLine(
            el::StringFormat{"Maximum plus one: {}; minimum minus one: {}; maximum times two: {}"_el}.build(
                (maximum + one).toRawValue(), (minimum - one).toRawValue(), (maximum * 2).toRawValue()));
        el::io::printLine(
            el::StringFormat{"Minimum: {}; negated minimum: {}; saturated negation: {}"_el}.build(
                minimum.toRawValue(), (-minimum).toRawValue(), (-minimum).isMaximum()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Add would saturate: true; subtract would saturate: true; multiply would saturate: true
    Maximum plus one: 9223372036854775807; minimum minus one: -9223372036854775808; maximum ti
    mes two: 9223372036854775807
    Minimum: -9223372036854775808; negated minimum: 9223372036854775807; saturated negation: t
    rue

.. erbsland-demo-end::

The signed minimum has no equally large positive counterpart.
Negating it therefore yields the maximum, losing one unit of magnitude.
Saturation also means that adding and then subtracting the same amount need not restore the original value near a bound.
A boundary query is useful when that loss would invalidate your calculation.

The divisor must be nonzero; division by zero terminates rather than returning an amount or throwing a recoverable
exception.
Validate an externally supplied divisor before using it.
The :doc:`IntegerAmount reference <../../reference/unit/units_and_versions>` supplies the complete operator contract,
including remainder, increment, and other common amount operations.

Giving Calendar Quantities a Starting Date
==========================================

A month count becomes concrete when you apply it to a date.
Adding one month to January 31 reaches the last available day in February, whereas adding thirty fixed days reaches a
different date.
Similarly, adding one year to February 29 adjusts the result to February 28 in a common year.
These are properties of the dated calculation, rather than a conversion of the ``Months`` or ``Years`` count.

.. erbsland-demo::
    :source: time/TimeAmounts/Calendar.cpp
    :exec: time/time_amounts --demo Calendar
    :source-sha256: 1bc17f15beae0f3d11d6cf9ae94394188c7befaad5401ec57ecf677d57976627

.. code-block:: cpp

    /// Apply month and year quantities in calendar context, and preserve combined changes in CalendarDelta.
    /// @notest{Compiled and executed documentation demo.}
    void calendar() {
        const auto start = el::Date{el::Year{2026}, el::Month::january(), el::Day{31}};
        const auto nextMonth = start.added(el::Months{1});
        const auto fixedDays = start.added(el::Days{30});
        const auto leapDay = el::Date{el::Year{2024}, el::Month::february(), el::Day{29}};
        el::io::printLine(
            el::StringFormat{"Start: {}; plus one month: {}; plus 30 days: {}; leap day plus one year: {}"_el}.build(
                start.toString(), nextMonth.toString(), fixedDays.toString(), leapDay.added(el::Years{1}).toString()));

        const auto change = el::CalendarDelta{el::Months{1}}.setYears(el::Years{1}).setDays(el::Days{2});
        const auto instant = el::DateTime{start, el::Time{el::Hour{9}, el::Minute{0}}};
        el::io::printLine(
            el::StringFormat{"Combined change: {}; applied: {}"_el}.build(
                change.toString(), instant.added(change).toString()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Start: 2026-01-31; plus one month: 2026-02-28; plus 30 days: 2026-03-02; leap day plus one
     year: 2025-02-28
    Combined change: 1 y 1 mo 2 d; applied: 2027-03-02 09:00:00Z

.. erbsland-demo-end::

``Date::added()`` accepts individual ``Days``, ``Months``, and ``Years`` quantities.
:doc:`working_with_dates` explains its month-end behavior and the saturating, throwing, and boundary-test variants.
For a combined change, ``CalendarDelta`` preserves separate components that a ``DateTime`` can apply in its defined
order.
The demo retains years, months, and days together; none is converted into a single fixed elapsed span first.
See :doc:`working_with_datetime` and the :doc:`calendar-delta reference <../../reference/time/date_and_time>` for
application order and time-zone behavior.

When your task instead selects a particular day, month, or hour, return to the corresponding calendar or time part.
Keeping positions and quantities distinct makes both construction and arithmetic easier to follow.
