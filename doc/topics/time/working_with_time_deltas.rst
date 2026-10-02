..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Time; Precise Intervals
    single: TimeDelta; Calculations and Conversion

***********************************
Working with Precise Time Intervals
***********************************

A latency measurement or a short timeout can lose its meaning if you reduce it to whole seconds.
:cpp:class:`TimeDelta <erbsland::time::TimeDelta>` preserves the fractional part in a signed nanosecond interval.
You can combine measurements, divide a time budget, compare intervals, and choose an output unit when passing the result
to another API.
This page develops those calculations and explains where precision loss or range limits become relevant.

Choosing Nanosecond Precision
=============================

A ``TimeDelta`` describes a fixed amount of elapsed time.
It has no date, epoch, or time zone, and it does not identify an instant.
For example, 250 milliseconds can be a delay, a measured duration, or a correction; its interpretation comes from your
calculation.
Positive and negative intervals are both supported, and every stored value is valid, including zero.

Its nanosecond count spans ``-9,223,372,036,854,775,808`` through ``9,223,372,036,854,775,807``.
That is approximately 292 years in either direction.
:cpp:class:`Duration <erbsland::time::Duration>` stores a signed 64-bit count of seconds instead, giving it a much
larger range at whole-second resolution.
A useful first question is whether losing a fractional second would change the meaning of your result.
A short measurement usually needs ``TimeDelta``, while a long whole-second interval can use ``Duration``.
:doc:`working_with_durations` develops that alternative and its larger range.
A nanosecond representation lets you retain a fine-grained result; it does not guarantee that the clock producing that
result can distinguish events one nanosecond apart.

Fixed days mean 86,400 seconds, and weeks mean seven such days.
Calendar months and years have lengths that depend on a starting date, so they belong in
:cpp:class:`CalendarDelta <erbsland::time::CalendarDelta>`.
An event instant belongs in :doc:`working_with_timestamps` or :doc:`working_with_datetime` instead.
Keeping the interval separate from the instant makes its unit and purpose clear when you reuse it.


Constructing an Interval
========================

Typed amounts put the unit beside the number: ``TimeDelta{Milliseconds{1250}}`` retains one second and 250 milliseconds,
rather than truncating to one second.
The constructor accepts the fixed amount types from nanoseconds through weeks and converts them to nanoseconds with
saturation at the supported bounds.
:doc:`working_with_time_amounts` explains those compatible units and their conversion rules.

Both default construction and :cpp:func:`zero() <erbsland::time::TimeDelta::zero>` produce zero.
The time literals are another readable way to supply an amount once ``erbsland::time::literals`` is in scope.
``1250_ms`` produces ``Milliseconds``; constructing a ``TimeDelta`` chooses the interval representation explicitly.
:doc:`writing_time_literals` covers every suffix and the required header.

A ``std::chrono::duration`` can also be passed to the constructor.
It is converted using ``std::chrono::duration_cast<std::chrono::nanoseconds>``.
Supply a finite value whose converted count fits that representation; this path is not a checked replacement for the
library's typed-amount conversion.
Any precision finer than a nanosecond is discarded toward zero.

.. erbsland-demo::
    :source: time/TimeDelta/Create.cpp
    :exec: time/time_delta --demo Create
    :source-sha256: 1d751b4df2bafa88854343ba3b00e85f593bda79dd6d1991aa60eab0965d1259

.. code-block:: cpp

    /// Construct nanosecond intervals from typed amounts, literals, and chrono durations.
    /// @notest{Compiled and executed documentation demo.}
    void create() {
        using namespace el::time::literals;

        const auto component = "reloj de arena"_el;
        const auto typed = el::TimeDelta{el::Milliseconds{1250}};
        const auto literal = el::TimeDelta{1250_ms};
        const auto chrono = el::TimeDelta{std::chrono::microseconds{1'250'001}};
        el::io::printLine(
            el::StringFormat{"{}: typed {}; literal {}; chrono {}"_el}.build(
                component, typed.toString(), literal.toString(), chrono.toString()));
        el::io::printLine(
            el::StringFormat{"Default is zero: {}; zero factory: {}"_el}.build(
                el::TimeDelta{}.isZero(), el::TimeDelta::zero().isZero()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    reloj de arena: typed 1 s 250 ms; literal 1 s 250 ms; chrono 1 s 250 ms 1 us
    Default is zero: true; zero factory: true

.. erbsland-demo-end::

Choosing a Factory and Overflow Policy
======================================

When your input is a plain integer whose unit is already known, the static unit factories make that unit visible at the
call site.
:cpp:func:`nanoseconds() <erbsland::time::TimeDelta::nanoseconds>` stores the signed count directly.
The factories ``microseconds()``, ``milliseconds()``, ``seconds()``, ``minutes()``, ``hours()``, ``days()``, and
``weeks()`` convert their counts into the stored nanoseconds.
All of them saturate if that conversion exceeds the range: the result stops at the nearest supported endpoint.

A count that fits in its original unit can still be too large after conversion.
For instance, the maximum ``Seconds`` amount fits in a whole-second duration but greatly exceeds ``TimeDelta``.
Check the source amount's ``wouldConvertSaturate<Nanoseconds>()`` if your next step depends on retaining the exact
value.
The demo shows both the predicate and the saturated nanosecond count.

For input that must be rejected when it is too large, choose ``microsecondsOrThrow()``, ``millisecondsOrThrow()``,
``secondsOrThrow()``, ``minutesOrThrow()``, ``hoursOrThrow()``, ``daysOrThrow()``, or ``weeksOrThrow()``.
These factories throw :cpp:class:`OverflowError <erbsland::err::OverflowError>` on conversion overflow.
There is no separate nanosecond throwing factory because every signed 64-bit nanosecond input already fits.
The same distinction is useful for configuration: saturation supplies a bounded value, while a throwing factory lets you
report that a supplied interval cannot be represented.

.. erbsland-demo::
    :source: time/TimeDelta/Factories.cpp
    :exec: time/time_delta --demo Factories
    :source-sha256: 15365ba4d7a82d987eee89089d2194b564617f013a850138e59fb15fb646d216

.. code-block:: cpp

    /// Choose fixed-unit factories and reject nanosecond conversion overflow.
    /// @notest{Compiled and executed documentation demo.}
    void factories() {
        const auto intervals = std::array{
            el::TimeDelta::nanoseconds(1),
            el::TimeDelta::microseconds(1),
            el::TimeDelta::milliseconds(1),
            el::TimeDelta::seconds(1),
            el::TimeDelta::minutes(1),
            el::TimeDelta::hours(1),
            el::TimeDelta::days(1),
            el::TimeDelta::weeks(1)};
        // Compare the output for each choice using the same input.
        for (const auto interval : intervals) {
            el::io::printLine(interval.toString());
        }

        // Each coarser unit also has a checked factory with the same conversion policy.
        const auto checkedUnits = std::array{
            el::TimeDelta::microsecondsOrThrow(1),
            el::TimeDelta::millisecondsOrThrow(1),
            el::TimeDelta::secondsOrThrow(1),
            el::TimeDelta::minutesOrThrow(1),
            el::TimeDelta::hoursOrThrow(1),
            el::TimeDelta::daysOrThrow(1),
            el::TimeDelta::weeksOrThrow(1)};
        el::io::printLine(el::StringFormat{"Checked week: {}"_el}.build(checkedUnits.back().toString()));

        // A value that fits in whole seconds can exceed the nanosecond range.
        const auto large = el::Seconds::maximum();
        const auto saturated = el::TimeDelta::seconds(large.toRawValue());
        el::io::printLine(
            el::StringFormat{"Would saturate: {}; stored ns: {}"_el}.build(
                large.wouldConvertSaturate<el::Nanoseconds>(), saturated.toNanoseconds().toRawValue()));
        try {
            const auto checked = el::TimeDelta::secondsOrThrow(large.toRawValue());
            el::io::printLine(checked.toString());
        } catch (const el::err::OverflowError &) {
            el::io::printLine("Checked factory rejected overflow."_el);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    1 ns
    1 us
    1 ms
    1 s
    1 min
    1 h
    1 d
    1 w
    Checked week: 1 w
    Would saturate: true; stored ns: 9223372036854775807
    Checked factory rejected overflow.

.. erbsland-demo-end::

Combining and Comparing Intervals
=================================

Addition and subtraction combine intervals without dropping fractional precision.
Multiplication scales an interval by an integer, and unary negation reverses its direction.
Scaling and integer division take ``TimeDelta::IntegerValue``, so construct the wrapper explicitly, for example
``interval * TimeDelta::IntegerValue{3}``.
The in-place forms ``+=``, ``-=``, ``*=``, and ``/=`` update an existing value.
The demo combines a preparation interval with a per-turn interval and then adjusts the remaining budget.

Comparison orders the signed nanosecond totals.
You can compare a measured interval with a budget directly, or inspect
:cpp:func:`isZero() <erbsland::time::TimeDelta::isZero>`, ``isPositive()``, and ``isNegative()`` when the sign itself
matters.
Zero is neither positive nor negative.
:cpp:func:`toAbsolute() <erbsland::time::TimeDelta::toAbsolute>` returns the magnitude as a nonnegative interval,
subject to the endpoint rule explained below.

Division has two useful meanings.
An interval divided by an integer returns another ``TimeDelta``: dividing a budget among two participants retains the
interval unit.
An interval divided by an interval returns an integer ratio: dividing a budget by a turn length counts complete turns.
That ratio has type ``TimeDelta::IntegerValue``, a saturating integer wrapper; ``toRawValue()`` extracts its native
count.
The two results answer different questions: one gives the time available for each share, while the other counts how many
complete intervals fit.
Both forms truncate toward zero, so dividing minus five nanoseconds by two becomes minus two nanoseconds, and a ratio
discards any incomplete share.

:cpp:func:`minimum() <erbsland::time::TimeDelta::minimum>` imposes a lower bound.
``delay.minimum(Seconds{1})`` returns at least one second, leaving a larger delay unchanged.
This is useful when a calculated delay should never become too short.
A delay already above the floor is returned unchanged, so the same expression handles both cases.

.. erbsland-demo::
    :source: time/TimeDelta/Calculate.cpp
    :exec: time/time_delta --demo Calculate
    :source-sha256: bd6910f2bf99a1bc5a8817b89d825a50c73d3c937b19fa2a7e3c8fa01fd05393

.. code-block:: cpp

    /// Combine intervals, scale a budget, and distinguish interval division from ratios.
    /// @notest{Compiled and executed documentation demo.}
    void calculate() {
        using namespace el::time::literals;

        const auto turn = el::TimeDelta{1250_ms};
        const auto setup = el::TimeDelta{250_ms};
        const auto budget = (turn + setup) * el::TimeDelta::IntegerValue{3};
        auto remaining = budget;
        remaining -= turn;
        remaining += setup;
        remaining *= el::TimeDelta::IntegerValue{2};
        remaining /= el::TimeDelta::IntegerValue{3};
        el::io::printLine(
            el::StringFormat{"Budget: {}; remaining: {}; setup fits: {}"_el}.build(
                budget.toString(), remaining.toString(), setup < remaining));
        el::io::printLine(
            el::StringFormat{"Per player: {}; complete turns: {}"_el}.build(
                (budget / el::TimeDelta::IntegerValue{2}).toString(), (budget / turn).toRawValue()));

        const auto correction = -setup;
        el::io::printLine(
            el::StringFormat{"Negative: {}; positive: {}; zero difference: {}; absolute: {}"_el}.build(
                correction.isNegative(), turn.isPositive(), (turn - turn).isZero(), correction.toAbsolute().toString()));
        // minimum imposes a floor; it does not cap a value.
        el::io::printLine(
            el::StringFormat{"Minimum delay: {}; negative integer division: {} ns"_el}.build(
                setup.minimum(1_s).toString(),
                (el::TimeDelta::nanoseconds(-5) / el::TimeDelta::IntegerValue{2}).toNanoseconds().toRawValue()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Budget: 4 s 500 ms; remaining: 2 s 333 ms 333 us 333 ns; setup fits: true
    Per player: 2 s 250 ms; complete turns: 3
    Negative: true; positive: true; zero difference: true; absolute: 250 ms
    Minimum delay: 1 s; negative integer division: -2 ns

.. erbsland-demo-end::

Handling Arithmetic Boundaries
==============================

Interval arithmetic saturates at the supported bounds.
Adding one nanosecond to the maximum keeps the maximum; subtracting one from the minimum keeps the minimum.
Multiplication and division use the same bounded integer behavior.
Saturation occurs at each operation, so a later subtraction cannot recover information lost in an earlier overflowing
sum.

The signed range has one more negative value than positive values.
Negating the minimum, taking its absolute value, or dividing it by minus one therefore produces the positive maximum.
The interval-ratio overload also saturates this extreme quotient rather than overflowing a native signed integer.
If your calculation needs an exact result at these boundaries, check the corresponding amount operations before
combining the intervals; :doc:`working_with_time_amounts` introduces their overflow predicates.

Before dividing, check that the divisor expresses a usable interval or count.
A zero divisor is a separate error: dividing by integer zero or by a zero interval terminates the process.
These operators are ``noexcept`` and do not throw an exception you can catch.
Check a user-supplied integer divisor before division, and check ``isZero()`` on an interval divisor before calculating
a ratio.
The demo deliberately guards that condition instead of executing the invalid division.

.. erbsland-demo::
    :source: time/TimeDelta/Boundaries.cpp
    :exec: time/time_delta --demo Boundaries
    :source-sha256: 7468fb699729e7df5364784ff4f870411e018ee7e1d988d83ab42cfb2b6105b8

.. code-block:: cpp

    /// Observe saturation, guard zero divisors, and handle the asymmetric signed bounds.
    /// @notest{Compiled and executed documentation demo.}
    void boundaries() {
        const auto low = el::TimeDelta{el::Nanoseconds::minimum()};
        const auto high = el::TimeDelta{el::Nanoseconds::maximum()};
        const auto tick = el::TimeDelta::nanoseconds(1);
        el::io::printLine(
            el::StringFormat{"High + tick: {}; low - tick: {}; high * 2: {}"_el}.build(
                (high + tick).toNanoseconds().toRawValue(),
                (low - tick).toNanoseconds().toRawValue(),
                (high * el::TimeDelta::IntegerValue{2}).toNanoseconds().toRawValue()));
        el::io::printLine(
            el::StringFormat{"Negated low: {}; absolute low: {}; low / -1: {}"_el}.build(
                (-low).toNanoseconds().toRawValue(),
                low.toAbsolute().toNanoseconds().toRawValue(),
                (low / el::TimeDelta::IntegerValue{-1}).toNanoseconds().toRawValue()));
        const auto divisor = el::TimeDelta::zero();
        if (divisor.isZero()) {
            el::io::printLine("Reject a zero divisor before calculating a ratio."_el);
        } else {
            el::io::printLine(el::StringFormat{"Ratio: {}"_el}.build((high / divisor).toRawValue()));
        }
        el::io::printLine(el::StringFormat{"Extreme ratio: {}"_el}.build((low / -tick).toRawValue()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    High + tick: 9223372036854775807; low - tick: -9223372036854775808; high * 2: 922337203685
    4775807
    Negated low: 9223372036854775807; absolute low: 9223372036854775807; low / -1: 92233720368
    54775807
    Reject a zero divisor before calculating a ratio.
    Extreme ratio: 9223372036854775807

.. erbsland-demo-end::

Converting Totals to Another Unit
=================================

The conversion methods return total amounts, rather than the components of a formatted interval.
:cpp:func:`toNanoseconds() <erbsland::time::TimeDelta::toNanoseconds>` retains the exact stored amount.
``toMilliseconds()`` and ``toSeconds()`` return complete milliseconds or seconds, discarding the smaller part toward
zero.
For the value below, ``toMilliseconds()`` returns 1,234, rather than the displayed millisecond component 234. A negative
1,500 milliseconds becomes minus one second, and either sign of 999 milliseconds becomes zero seconds.
That truncation is an intentional unit conversion, not an overflow error.

:cpp:func:`toDuration() <erbsland::time::TimeDelta::toDuration>` applies the same whole-second truncation.
Every ``TimeDelta`` fits into ``Duration`` after that conversion, but a nonzero fractional interval can become a zero
duration.
``toStdNanoseconds()`` instead preserves the nanosecond count for ``std::chrono`` interoperability.

``toSecondsWithFractions()`` and ``toDaysWithFractions()`` return ``double`` values.
They are convenient for approximate presentation or calculations that already use floating-point numbers.
A ``double`` cannot preserve every nanosecond over the full interval range, so keep the integer amount when exact
comparisons or round trips matter.
The demo shows both total integer conversions and the approximate fractional values.

.. erbsland-demo::
    :source: time/TimeDelta/Convert.cpp
    :exec: time/time_delta --demo Convert
    :source-sha256: 1c11229001a2611971f3bcb6ecbbf93ecffc285e52779b51744a6366cb6dbb78

.. code-block:: cpp

    /// Convert total interval amounts and preserve or deliberately discard fractional precision.
    /// @notest{Compiled and executed documentation demo.}
    void convert() {
        // Keep the interval exact; choose its presentation or conversion separately.
        const auto interval = el::TimeDelta::nanoseconds(1'234'567'890);
        el::io::printLine(
            el::StringFormat{"Totals: {} ns; {} ms; {} s"_el}.build(
                interval.toNanoseconds().toRawValue(),
                interval.toMilliseconds().toRawValue(),
                interval.toSeconds().toRawValue()));
        el::io::printLine(
            el::StringFormat{"Approximate seconds: {}; approximate days: {}"_el}.build(
                interval.toSecondsWithFractions(), interval.toDaysWithFractions()));
        el::io::printLine(el::StringFormat{"Chrono ns: {}"_el}.build(interval.toStdNanoseconds().count()));
        // Compare the output for each choice using the same input.
        for (const auto input : {-1500, -999, 999, 1500}) {
            const auto value = el::TimeDelta::milliseconds(input);
            el::io::printLine(
                el::StringFormat{"{} ms: {} seconds; Duration {} seconds"_el}.build(
                    input, value.toSeconds().toRawValue(), value.toDuration().toSeconds().toRawValue()));
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Totals: 1234567890 ns; 1234 ms; 1 s
    Approximate seconds: 1.23456789; approximate days: 1.4288980208333334e-05
    Chrono ns: 1234567890
    -1500 ms: -1 seconds; Duration -1 seconds
    -999 ms: 0 seconds; Duration 0 seconds
    999 ms: 0 seconds; Duration 0 seconds
    1500 ms: 1 seconds; Duration 1 seconds

.. erbsland-demo-end::

Presenting an Interval
======================

By default, :cpp:func:`toString() <erbsland::time::TimeDelta::toString>` splits an interval into nonzero fixed
components from weeks down to nanoseconds, using short unit names.
For example, ``1,234,567,890 ns`` is displayed as ``1 s 234 ms 567 us 890 ns``.
This decomposition is useful for reading a value, but it does not change the stored total.

A :cpp:class:`TimeDeltaFormat <erbsland::time::TimeDeltaFormat>` lets you choose a smallest displayed unit and expose
its remainder as a fraction.
The example selects seconds, enables fractions, and allows three digits, producing ``1.234 s``.
The discarded text precision remains present in the interval.
:doc:`customizing_time_interval_output` explains every format option, their combinations, and calendar-delta output.

.. erbsland-demo::
    :source: time/TimeDelta/Display.cpp
    :exec: time/time_delta --demo Display
    :source-sha256: ea1434c9b2c7411a73c4f4c07e5bb8f0e20a418492d603f8e5239f091caef4d6

.. code-block:: cpp

    /// Present a fixed interval with default components or a chosen fractional unit.
    /// @notest{Compiled and executed documentation demo.}
    void display() {
        // Keep the interval exact; choose its presentation or conversion separately.
        const auto interval = el::TimeDelta::nanoseconds(1'234'567'890);
        const auto format = el::TimeDeltaFormat{}
                                .setSmallestUnit(el::TimeDeltaUnit::Seconds)
                                .setShowFractions(true)
                                .setMaximumFractionDigits(3);
        el::io::printLine(
            el::StringFormat{"Default: {}; customized: {}; unchanged ns: {}"_el}.build(
                interval.toString(), interval.toString(format), interval.toNanoseconds().toRawValue()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Default: 1 s 234 ms 567 us 890 ns; customized: 1.234 s; unchanged ns: 1234567890

.. erbsland-demo-end::

Measuring and Applying an Interval
==================================

A :cpp:class:`ElapsedTimer <erbsland::time::ElapsedTimer>` returns a ``TimeDelta`` when you measure elapsed time, while
:cpp:class:`TimePoint <erbsland::time::TimePoint>` supports monotonic checkpoints and deadlines.
Their interfaces are in the :doc:`date and time reference <../../reference/time/date_and_time>`.
The nanosecond interval gives you a common representation for comparing the results of those measurements.

:doc:`working_with_times` demonstrates adding a precise interval to a daily clock reading while keeping its day carry.
:doc:`working_with_timestamps` explains precise distances between UTC instants.
For combined calendar changes, the ``CalendarDelta`` interface and application rules are in the same reference.

