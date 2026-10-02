..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Time; Whole-Second Durations
    single: Duration; Construction and Conversion
    single: DurationPart

***********************************
Working with Whole-Second Durations
***********************************

A timeout or a long interval often needs a clear count of seconds without fractional detail.
:cpp:class:`Duration <erbsland::time::Duration>` keeps that interval as a signed whole-second value, so you can combine
it with other intervals, compare it with a budget, and split it into readable components.
This page takes you from choosing the precision to constructing a duration and passing it to APIs with different range
or unit requirements.

Choosing a Fixed Interval
=========================

A ``Duration`` expresses an amount of elapsed time, without a starting date, clock reading, or time zone.
Positive and negative values are equally useful: a positive amount can be a delay, while a negative one can describe a
correction or a difference in the opposite direction.
Default construction represents zero; there is no invalid sentinel.

The stored seconds cover the signed 64-bit range, from ``-9,223,372,036,854,775,808`` through
``9,223,372,036,854,775,807``.
:cpp:class:`TimeDelta <erbsland::time::TimeDelta>` uses the same size of integer for nanoseconds, so its finer precision
comes with a much smaller range: approximately 292 years in either direction.
``Duration`` can represent far longer whole-second intervals, but it cannot preserve a fractional second.
A whole-second timeout can use ``Duration`` directly, while a latency measurement usually needs ``TimeDelta``.
That tradeoff also matters later: a value that fits in the first type may be too large for the second.

The fixed units through :cpp:type:`Weeks <erbsland::time::Weeks>` are compatible with ``Duration``.
A :cpp:type:`Days <erbsland::time::Days>` amount means 86,400 seconds, and a week means seven such days.
Months and years need a calendar to determine their elapsed length, so they belong in
:cpp:class:`CalendarDelta <erbsland::time::CalendarDelta>` rather than in this seconds count.
The amount types are listed in the :doc:`date and time reference <../../reference/time/date_and_time>`.
For a local recurrence, remember that a fixed 24-hour interval can cross a daylight-saving change and end at a different
local clock time; :doc:`working_with_datetime` develops that distinction.

Building a Duration
===================

A typed amount puts the unit next to the number.
For example, ``Duration{Seconds{30}}`` represents thirty seconds, while ``Duration{Minutes{2}}`` converts two minutes to
120 stored seconds.
The constructor accepts all seconds-based amount types, from nanoseconds through weeks.
Larger units convert to seconds with saturation if they exceed the stored range.
Smaller units discard their fractional seconds.
Both ``Duration{}`` and :cpp:func:`zero() <erbsland::time::Duration::zero>` create the same zero interval.

When the input already comes in several components, :cpp:struct:`Duration::Parts <erbsland::time::Duration::Parts>`
keeps ``seconds``, ``minutes``, ``hours``, ``days``, and ``weeks`` in named typed fields.
Omitted fields default to zero.
Constructing a duration from the parts combines their fixed second amounts with saturating arithmetic.
These fields are convenient when a configuration or input form supplies an interval in several units.
They are quantities, so ninety minutes is a valid input rather than an invalid clock field.
Each component can also have its own sign.
If you combine very large components of opposite signs, an intermediate sum can saturate before they cancel; the final
value then cannot recover the lost amount.

A ``std::chrono::duration`` can be passed directly to the constructor as well.
Its conversion uses ``std::chrono::duration_cast<std::chrono::seconds>``: supply a finite value whose converted count
fits that representation.
For typed amounts with a seconds-based unit, the library's conversion instead follows its saturating amount rules.

Sub-second conversion truncates toward zero in either path.
A positive 2,500 milliseconds becomes two seconds, a negative 2,500 milliseconds becomes minus two, and both positive
and negative 999 milliseconds become zero.
This is useful when you intentionally retain only complete seconds.
It also means ``isZero()`` can become true after conversion of a nonzero sub-second input.
Keep the original precise amount if that distinction still matters to your calculation.
If a timeout must wait at least the supplied fractional amount, choose that rounding policy explicitly before creating
the ``Duration``; truncation does not round upward for you.

.. erbsland-demo::
    :source: time/Duration/Create.cpp
    :exec: time/duration --demo Create
    :source-sha256: af7aca1b6441ab7f2e9681fed87d4aefddea80dae5204f699dd33fd27099e02a

.. code-block:: cpp

    /// Build whole-second intervals from typed amounts, parts, and chrono values.
    /// @notest{Compiled and executed documentation demo.}
    void create() {
        const auto prototype = "試作センサー"_el;
        const auto empty = el::Duration{};
        const auto timeout = el::Duration{el::Seconds{30}};
        const auto warmup = el::Duration{el::Minutes{2}};
        const auto fixed = el::Duration{el::Days{2}};
        el::io::printLine(
            el::StringFormat{"{}: timeout {} s; warmup {} s; fixed interval {} s"_el}.build(
                prototype,
                timeout.toSeconds().toRawValue(),
                warmup.toSeconds().toRawValue(),
                fixed.toSeconds().toRawValue()));
        el::io::printLine(
            el::StringFormat{"Default is zero: {}; zero factory matches: {}"_el}.build(
                empty.isZero(), empty == el::Duration::zero()));

        // Components are combined as fixed amounts, even beyond customary field limits.
        const auto combined = el::Duration{el::Duration::Parts{.seconds = el::Seconds{5}, .minutes = el::Minutes{90}}};
        el::io::printLine(el::StringFormat{"90 minutes and 5 seconds: {} s"_el}.build(combined.toSeconds().toRawValue()));

        // Sub-second inputs truncate toward zero, including negative inputs.
        for (const auto milliseconds : {2500, -2500, 999, -999}) {
            const auto typed = el::Duration{el::Milliseconds{milliseconds}};
            const auto chrono = el::Duration{std::chrono::milliseconds{milliseconds}};
            el::io::printLine(
                el::StringFormat{"{} ms: typed {} s; chrono {} s"_el}.build(
                    milliseconds, typed.toSeconds().toRawValue(), chrono.toSeconds().toRawValue()));
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    試作センサー: timeout 30 s; warmup 120 s; fixed interval 172800 s
    Default is zero: true; zero factory matches: true
    90 minutes and 5 seconds: 5405 s
    2500 ms: typed 2 s; chrono 2 s
    -2500 ms: typed -2 s; chrono -2 s
    999 ms: typed 0 s; chrono 0 s
    -999 ms: typed 0 s; chrono 0 s

.. erbsland-demo-end::

Combining Intervals and Checking a Budget
=========================================

Addition and subtraction produce another ``Duration`` with the same whole-second precision.
You can keep the original values with ``+`` and ``-``, or update a running total with ``+=`` and ``-=``.
Unary minus reverses the direction of an interval.
The demo combines two phases, subtracts their total from a budget, then revises that total in place.

Equality and ordering compare signed total seconds, so a negative duration sorts before zero and a longer positive
interval sorts after a shorter one.
:cpp:func:`isZero() <erbsland::time::Duration::isZero>`,
:cpp:func:`isPositive() <erbsland::time::Duration::isPositive>`, and
:cpp:func:`isNegative() <erbsland::time::Duration::isNegative>` make sign decisions readable without extracting a raw
count.
A negative remaining budget can therefore be handled as a meaningful result rather than an invalid duration.

Arithmetic saturates at the supported bounds.
Adding one second to the maximum keeps the maximum; subtracting one second from the minimum keeps the minimum.
Negating the minimum yields the maximum because the positive counterpart would be one second beyond the range.
These operations do not throw or wrap around, but a saturated result no longer represents the full mathematical total.
If that distinction matters, check the underlying :cpp:type:`Seconds <erbsland::time::Seconds>` amounts before the
operation, using the boundary checks documented for
:cpp:class:`IntegerAmount <erbsland::unit::IntegerAmount>` in the :doc:`unit reference <../../reference/unit/units_and_versions>`.

.. erbsland-demo::
    :source: time/Duration/Calculate.cpp
    :exec: time/duration --demo Calculate
    :source-sha256: 1ced8526d72fbf5f911eb78a64b0dbda9d7f6917f627b569501c325812abccc6

.. code-block:: cpp

    /// Combine intervals, compare budgets, and observe saturation at second bounds.
    /// @notest{Compiled and executed documentation demo.}
    void calculate() {
        const auto warmup = el::Duration{el::Minutes{2}};
        const auto measurement = el::Duration{el::Seconds{45}};
        const auto budget = el::Duration{el::Minutes{3}};
        const auto required = warmup + measurement;
        const auto remaining = budget - required;
        el::io::printLine(
            el::StringFormat{"Required: {} s; remaining: {} s; within budget: {}"_el}.build(
                required.toSeconds().toRawValue(), remaining.toSeconds().toRawValue(), required <= budget));
        auto revised = required;
        revised += el::Duration{el::Seconds{10}};
        revised -= el::Duration{el::Seconds{5}};
        el::io::printLine(
            el::StringFormat{"Revised: {} s; equals original: {}; positive: {}; negative: {}; zero: {}"_el}.build(
                revised.toSeconds().toRawValue(),
                revised == required,
                revised.isPositive(),
                (-revised).isNegative(),
                (revised - revised).isZero()));

        // Arithmetic clamps rather than wrapping or throwing at a representable bound.
        const auto maximum = el::Duration{el::Seconds::maximum()};
        const auto minimum = el::Duration{el::Seconds::minimum()};
        const auto one = el::Duration{el::Seconds{1}};
        el::io::printLine(
            el::StringFormat{"Adding one to maximum would saturate: {}"_el}.build(
                maximum.toSeconds().wouldAddSaturate(one.toSeconds())));
        el::io::printLine(el::StringFormat{"Maximum plus one: {}"_el}.build((maximum + one).toSeconds().toRawValue()));
        el::io::printLine(el::StringFormat{"Minimum minus one: {}"_el}.build((minimum - one).toSeconds().toRawValue()));
        el::io::printLine(el::StringFormat{"Negated minimum: {}"_el}.build((-minimum).toSeconds().toRawValue()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Required: 165 s; remaining: 15 s; within budget: true
    Revised: 170 s; equals original: false; positive: true; negative: true; zero: true
    Adding one to maximum would saturate: true
    Maximum plus one: 9223372036854775807
    Minimum minus one: -9223372036854775808
    Negated minimum: 9223372036854775807

.. erbsland-demo-end::

Splitting a Total into Components
=================================

A total and a component answer different questions.
:cpp:func:`toSeconds() <erbsland::time::Duration::toSeconds>` gives the complete signed seconds count.
The :cpp:func:`seconds() <erbsland::time::Duration::seconds>` accessor gives the remainder within a minute;
:cpp:func:`minutes() <erbsland::time::Duration::minutes>` gives the minute remainder within an hour;
and :cpp:func:`hours() <erbsland::time::Duration::hours>` gives the hour remainder within a day.
:cpp:func:`days() <erbsland::time::Duration::days>` returns total whole days, truncated toward zero, rather than the
remainder within a week.

For nine days, three hours, four minutes, and five seconds, the total is 788,645 seconds.
The accessors return 9, 3, 4, and 5 in their corresponding units.
On the negative counterpart they return -9, -3, -4, and -5. In particular, a negative component keeps its sign instead
of being normalized into a positive clock field.
The seconds and minutes remainders therefore lie in ``-59..59``, and the hour remainder lies in ``-23..23``.
These values describe an interval; they are not positions within a clock day.

:cpp:func:`parts() <erbsland::time::Duration::parts>` gathers the same information in ``Duration::Parts`` and lets you
choose the largest displayed unit with :cpp:enum:`DurationPart <erbsland::time::DurationPart>`.
Every choice keeps the entire interval in the returned parts, leaving fields above the selected unit at zero.
The largest included field holds the total in that unit; smaller fields hold signed remainders.

.. list-table::
    :header-rows: 1
    :widths: 22 45 33

    * - Largest part
      - Total held in
      - Remaining components
    * - ``Seconds``
      - Seconds
      - None
    * - ``Minutes``
      - Whole minutes
      - Seconds
    * - ``Hours``
      - Whole hours
      - Minutes and seconds
    * - ``Days`` (default)
      - Whole days
      - Hours, minutes, and seconds
    * - ``Weeks``
      - Whole weeks
      - Days, hours, minutes, and seconds

For example, choosing ``Minutes`` gives 13,144 minutes and five seconds for the interval above, while ``Weeks`` gives
one week, two days, three hours, four minutes, and five seconds.
You can then construct ``Duration{parts}`` to recover the original total, including its sign.
This makes ``parts()`` convenient for a display that needs several fields together or for an API that expects separate
components.
The demo prints every largest-unit choice and rebuilds a negative duration from its default parts.

.. erbsland-demo::
    :source: time/Duration/Parts.cpp
    :exec: time/duration --demo Parts
    :source-sha256: fef97b89a75a4a0d5d01d3d1ac4ed63e7d61218daa07afb9e1450a1c2a6f2373

.. code-block:: cpp

    /// Split totals into components with a chosen largest unit and retain signed parts.
    /// @notest{Compiled and executed documentation demo.}
    void parts() {
        const auto interval = el::Duration{el::Duration::Parts{
            .seconds = el::Seconds{5}, .minutes = el::Minutes{4}, .hours = el::Hours{3}, .days = el::Days{9}}};
        el::io::printLine(
            el::StringFormat{"Total: {} s; components: {} d, {} h, {} m, {} s"_el}.build(
                interval.toSeconds().toRawValue(),
                interval.days().toRawValue(),
                interval.hours().toRawValue(),
                interval.minutes().toRawValue(),
                interval.seconds().toRawValue()));
        const auto choices = std::array{
            std::pair{"Seconds"_el, el::DurationPart::Seconds},
            std::pair{"Minutes"_el, el::DurationPart::Minutes},
            std::pair{"Hours"_el, el::DurationPart::Hours},
            std::pair{"Days"_el, el::DurationPart::Days},
            std::pair{"Weeks"_el, el::DurationPart::Weeks}};
        for (const auto &[name, largest] : choices) {
            const auto parts = interval.parts(largest);
            el::io::printLine(
                el::StringFormat{"{}: {} w, {} d, {} h, {} m, {} s"_el}.build(
                    name,
                    parts.weeks.toRawValue(),
                    parts.days.toRawValue(),
                    parts.hours.toRawValue(),
                    parts.minutes.toRawValue(),
                    parts.seconds.toRawValue()));
        }
        const auto negative = -interval;
        const auto parts = negative.parts();
        el::io::printLine(
            el::StringFormat{"Negative: {} d, {} h, {} m, {} s; rebuilt equal: {}"_el}.build(
                parts.days.toRawValue(),
                parts.hours.toRawValue(),
                parts.minutes.toRawValue(),
                parts.seconds.toRawValue(),
                el::Duration{parts} == negative));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Total: 788645 s; components: 9 d, 3 h, 4 m, 5 s
    Seconds: 0 w, 0 d, 0 h, 0 m, 788645 s
    Minutes: 0 w, 0 d, 0 h, 13144 m, 5 s
    Hours: 0 w, 0 d, 219 h, 4 m, 5 s
    Days: 0 w, 9 d, 3 h, 4 m, 5 s
    Weeks: 1 w, 2 d, 3 h, 4 m, 5 s
    Negative: -9 d, -3 h, -4 m, -5 s; rebuilt equal: true

.. erbsland-demo-end::

Passing the Interval to Another API
===================================

Whole Seconds and a Day Split
-----------------------------

For a standard-library API, :cpp:func:`toStdSeconds() <erbsland::time::Duration::toStdSeconds>` returns an equivalent
``std::chrono::seconds`` value on platforms whose seconds representation can hold the count.
``toSeconds()`` instead retains the library's typed amount, and ``toSeconds().toRawValue()`` exposes the native count
when an external interface needs a number.
Keep the unit attached for as long as your own code can use it.

Some APIs need both a day count and a fine remainder.
:cpp:func:`toDaysAndNanoseconds() <erbsland::time::Duration::toDaysAndNanoseconds>` returns
:cpp:struct:`Duration::DaysAndNanoseconds <erbsland::time::Duration::DaysAndNanoseconds>` with named ``days`` and
``nanoseconds`` fields.
The whole days truncate toward zero, and the signed remainder is strictly smaller than one fixed day in magnitude.
For minus two days and three hours, the fields are minus two days and minus three hours expressed as nanoseconds.
This differs from a representation that puts a negative input on the previous day with a positive remainder.

The split retains the exact whole-second interval even when the entire total would not fit in nanoseconds: only the
sub-day remainder is converted to nanoseconds.
The remainder is therefore always a multiple of one billion nanoseconds.
:cpp:func:`toDaysWithFractions() <erbsland::time::Duration::toDaysWithFractions>` provides another view as a ``double``
count of fixed days.
It is approximate, especially for large intervals, and is appropriate for a graph or a rough display rather than an
exact round trip.

.. erbsland-demo::
    :source: time/Duration/Convert.cpp
    :exec: time/duration --demo Convert
    :source-sha256: 388a14506df5d61c489ff177c654044255d9a3bfdd8b21ffef2783ade9790287

.. code-block:: cpp

    /// Pass whole seconds or a signed day and nanosecond split to another API.
    /// @notest{Compiled and executed documentation demo.}
    void convert() {
        const auto interval =
            el::Duration{el::Duration::Parts{.seconds = el::Seconds{5}, .hours = el::Hours{3}, .days = el::Days{2}}};
        const auto chrono = interval.toStdSeconds();
        el::io::printLine(
            el::StringFormat{"Chrono seconds: {}; round trip equal: {}"_el}.build(
                chrono.count(), el::Duration{chrono} == interval));
        for (const auto value : {interval, -interval}) {
            const auto split = value.toDaysAndNanoseconds();
            el::io::printLine(
                el::StringFormat{"{} s: {} d, {} ns; approximate days: {}"_el}.build(
                    value.toSeconds().toRawValue(),
                    split.days.toRawValue(),
                    split.nanoseconds.toRawValue(),
                    value.toDaysWithFractions()));
        }
        // Even the largest duration can be split without converting its entire total to nanoseconds.
        const auto maximum = el::Duration{el::Seconds::maximum()}.toDaysAndNanoseconds();
        el::io::printLine(
            el::StringFormat{"Maximum split: {} d, {} ns"_el}.build(
                maximum.days.toRawValue(), maximum.nanoseconds.toRawValue()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Chrono seconds: 183605; round trip equal: true
    183605 s: 2 d, 10805000000000 ns; approximate days: 2.1250578703703704
    -183605 s: -2 d, -10805000000000 ns; approximate days: -2.1250578703703704
    Maximum split: 106751991167300 d, 55807000000000 ns

.. erbsland-demo-end::

Converting to Nanosecond Precision
----------------------------------

:cpp:func:`toTimeDelta() <erbsland::time::Duration::toTimeDelta>` expresses the same fixed interval in nanoseconds when
it fits.
It adds no measured precision: thirty stored seconds become exactly thirty billion nanoseconds, with no recovered
fractional reading.
The result saturates at the nanosecond bounds when the duration is too large in either direction.

A ten-billion-second duration is a useful example: it fits comfortably in ``Duration`` but exceeds the nanosecond range
of ``TimeDelta``.
The largest positive whole-second count that converts exactly is 9,223,372,036; the negative exact limit is
-9,223,372,036 seconds.
One more whole second in either direction overflows the target.

:cpp:func:`wouldConvertToTimeDeltaSaturate() <erbsland::time::Duration::wouldConvertToTimeDeltaSaturate>` lets you choose
another representation before converting.
:cpp:func:`toTimeDeltaOrThrow() <erbsland::time::Duration::toTimeDeltaOrThrow>` instead rejects an overflowing conversion
with :cpp:class:`OverflowError <erbsland::err::OverflowError>`.
The checked form is helpful when an external operation requires the exact interval and a shorter saturated value would
be misleading.

.. erbsland-demo::
    :source: time/Duration/Precise.cpp
    :exec: time/duration --demo Precise
    :source-sha256: 93f0067cbcbaa9b4fb253a750809c9eb90df57c6d4700b87c7232c712f144bee

.. code-block:: cpp

    /// Check whether a whole-second interval fits a nanosecond representation.
    /// @notest{Compiled and executed documentation demo.}
    void precise() {
        const auto timeout = el::Duration{el::Seconds{30}};
        const auto precise = timeout.toTimeDeltaOrThrow();
        el::io::printLine(
            el::StringFormat{"30 seconds: {} ns; round trip equal: {}"_el}.build(
                precise.toNanoseconds().toRawValue(), precise.toDuration() == timeout));

        // The last exact whole-second values fit; one more second overflows in either direction.
        for (const auto seconds : {9'223'372'036LL, 9'223'372'037LL, -9'223'372'036LL, -9'223'372'037LL}) {
            const auto interval = el::Duration{el::Seconds{seconds}};
            el::io::printLine(
                el::StringFormat{"{} s would saturate: {}"_el}.build(seconds, interval.wouldConvertToTimeDeltaSaturate()));
        }

        // Ten billion seconds fit Duration but exceed TimeDelta's nanosecond range.
        const auto longInterval = el::Duration{el::Seconds{10'000'000'000}};
        el::io::printLine(
            el::StringFormat{"Conversion would saturate: {}; saturated nanoseconds: {}"_el}.build(
                longInterval.wouldConvertToTimeDeltaSaturate(), longInterval.toTimeDelta().toNanoseconds().toRawValue()));
        try {
            const auto checked = longInterval.toTimeDeltaOrThrow();
            el::io::printLine(el::StringFormat{"Checked nanoseconds: {}"_el}.build(checked.toNanoseconds().toRawValue()));
        } catch (const el::err::OverflowError &) {
            el::io::printLine("Checked conversion rejected the overflowing interval."_el);
        }
    }

    }

.. erbsland-ansi::
    :escape-char: ␛

    30 seconds: 30000000000 ns; round trip equal: true
    9223372036 s would saturate: false
    9223372037 s would saturate: true
    -9223372036 s would saturate: false
    -9223372037 s would saturate: true
    Conversion would saturate: true; saturated nanoseconds: 9223372036854775807
    Checked conversion rejected the overflowing interval.

.. erbsland-demo-end::

Applying Whole-Second Intervals
===============================

:doc:`working_with_times` applies a duration to a daily clock reading and returns the crossed days.
:doc:`working_with_datetime` and :doc:`working_with_timestamps` use fixed intervals to move dated instants, with their
own calendar boundaries and overflow choices.
Those boundaries can be reached long before the duration's seconds count reaches its limit.

The time literal suffixes, such as ``_s`` and ``_m``, produce typed amounts that you can pass to the ``Duration``
constructor; the :doc:`overview` introduces their units and namespace.
For sub-second measurements or unit-based interval text, continue with ``TimeDelta`` and ``TimeDeltaFormat`` in the
:doc:`date and time reference <../../reference/time/date_and_time>`.
The same reference lists the complete ``Duration`` interface and both split-result structures.
