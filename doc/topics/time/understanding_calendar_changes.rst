..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Time; Calendar Changes
    single: CalendarDelta; Applying and Converting
    single: Calendar Arithmetic; Month-End Clamping

******************************
Understanding Calendar Changes
******************************

Moving a date forward by one month sounds simple until the starting date is January 31. A month is a calendar change,
whose effect depends on the date where you begin.
:cpp:class:`CalendarDelta <erbsland::time::CalendarDelta>` lets you retain that intent alongside changes expressed in days, hours, or smaller units.
This page explains how to compose these changes, apply them to :cpp:class:`DateTime <erbsland::time::DateTime>`, and
handle month ends, time zones, and range limits without losing the meaning of your calculation.

Choosing a Calendar Change
==========================

A fixed interval answers how much time should elapse.
``Duration{Days{30}}`` always represents thirty days of 86,400 seconds each, and
:cpp:class:`TimeDelta <erbsland::time::TimeDelta>` can express the same interval with nanosecond precision.
Neither expression means one calendar month.
Starting at January 31, 2024, thirty fixed days reach March 1, while one calendar month reaches February 29. The
difference matters whenever the rule refers to a calendar position rather than a measured interval.
See :doc:`working_with_durations` and :doc:`working_with_time_deltas` for fixed spans.

:cpp:class:`CalendarDelta <erbsland::time::CalendarDelta>` stores ten independent signed amounts, from nanoseconds through years.
Months and years need a starting date before they have a concrete effect; the other components describe fixed changes.
A delta itself has no date or zone.
It can represent one month and two days, or a mixed correction such as one month minus three hours, without converting
calendar units into an assumed number of seconds.

Before applying a change, decide which calendar fields your rule refers to.
:cpp:class:`DateTime <erbsland::time::DateTime>` applies a calendar delta to its UTC representation, then displays the result in the retained zone.
For a rule stated in local wall-clock terms, keep the intended local time separately and resolve each occurrence on its
own date.
The zone example below shows both approaches side by side.

Composing a Change
==================

Default construction produces a zero delta.
For one component, the amount constructor keeps the expression short: ``CalendarDelta{Months{1}}`` means one month.
It accepts every amount type from :cpp:type:`Nanoseconds <erbsland::time::Nanoseconds>` through
:cpp:type:`Years <erbsland::time::Years>`.
:doc:`working_with_time_amounts` explains those quantities and distinguishes them from calendar parts.

When several components belong together, :cpp:struct:`CalendarDeltaParts <erbsland::time::CalendarDeltaParts>` names
each field at construction.
Omitted fields default to zero, so a designated initializer makes the chosen components easy to see.
:cpp:type:`CalendarDelta::Parts <erbsland::time::CalendarDelta::Parts>` is an alias for the same structure.
The typed accessors return the independently stored values, and
:cpp:func:`parts() <erbsland::time::CalendarDelta::parts>` returns a copy of the complete structure.
Changing that copy does not change the original delta.
This is useful when you keep a reusable rule and derive a modified rule for one calculation: edit the copied parts, then
construct another delta.

The setters, from :cpp:func:`setNanoseconds() <erbsland::time::CalendarDelta::setNanoseconds>` through
:cpp:func:`setYears() <erbsland::time::CalendarDelta::setYears>`, replace one component and return the delta by
reference.
You can chain them when assembling a change from several inputs.
A setter does not add to its existing component: calling ``setMonths(Months{2})`` replaces the stored month amount with
two months.

.. erbsland-demo::
    :source: time/CalendarDelta/Compose.cpp
    :exec: time/calendar_delta --demo Compose
    :source-sha256: 92661af7acd36842182687484966e0b4cd94daf78b27a97da1fd2d862229c305

.. code-block:: cpp

    /// Build a calendar change from independent amounts and inspect its stored parts.
    /// @notest{Compiled and executed documentation demo.}
    void compose() {
        const auto survey = "Tangskov"_el;
        const auto zero = el::CalendarDelta{};
        const auto monthly = el::CalendarDelta{el::Months{1}};
        const auto visit =
            el::CalendarDelta{el::CalendarDeltaParts{.hours = el::Hours{2}, .days = el::Days{1}, .months = el::Months{1}}};
        auto adjustment = el::CalendarDelta{};
        adjustment.setNanoseconds(el::Nanoseconds{1})
            .setMicroseconds(el::Microseconds{2})
            .setMilliseconds(el::Milliseconds{3})
            .setSeconds(el::Seconds{4})
            .setMinutes(el::Minutes{5})
            .setHours(el::Hours{6})
            .setDays(el::Days{7})
            .setWeeks(el::Weeks{8})
            .setMonths(el::Months{9})
            .setYears(el::Years{10});
        el::io::printLine(
            el::StringFormat{"{}: zero {}; monthly {}; visit {}"_el}.build(
                survey, zero.isZero(), monthly.toString(), visit.toString()));
        // Accessors retain each chosen unit, even when display combines fixed components.
        const auto parts = adjustment.parts();
        el::io::printLine(
            el::StringFormat{"ns {}; us {}; ms {}; s {}; min {}; h {}; d {}; wk {}; mo {}; yr {}"_el}.build(
                adjustment.nanoseconds().toRawValue(),
                adjustment.microseconds().toRawValue(),
                adjustment.milliseconds().toRawValue(),
                adjustment.seconds().toRawValue(),
                adjustment.minutes().toRawValue(),
                adjustment.hours().toRawValue(),
                adjustment.days().toRawValue(),
                adjustment.weeks().toRawValue(),
                adjustment.months().toRawValue(),
                adjustment.years().toRawValue()));
        el::io::printLine(el::StringFormat{"Rebuilt parts equal: {}"_el}.build(el::CalendarDelta{parts} == adjustment));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Tangskov: zero true; monthly 1 mo; visit 1 mo 1 d 2 h
    ns 1; us 2; ms 3; s 4; min 5; h 6; d 7; wk 8; mo 9; yr 10
    Rebuilt parts equal: true

.. erbsland-demo-end::

The stored components are not normalized.
Sixty minutes remain sixty minutes; they do not become one hour in
:cpp:func:`hours() <erbsland::time::CalendarDelta::hours>`.
:cpp:func:`toString() <erbsland::time::CalendarDelta::toString>` can combine fixed components for display, so formatted text may look different from the fields returned
by :cpp:func:`parts() <erbsland::time::CalendarDelta::parts>`.
:doc:`customizing_time_interval_output` explains that presentation separately from the stored change.

Combining Changes Before Application
====================================

Addition and subtraction combine corresponding components.
Unary negation reverses each component, while ``+=`` and ``-=`` update the delta in place.
These operations compose a change without applying it to any date.
Each component follows the saturating arithmetic of its amount type; if you combine extreme values, the component stays
at its supported bound.

Equality compares those stored components.
One hour and sixty minutes are therefore different deltas even though both can convert to the same fixed interval.
Similarly, :cpp:func:`isZero() <erbsland::time::CalendarDelta::isZero>` asks whether every component is zero.
A delta containing sixty minutes and minus one hour is not structurally zero, although its exact fixed total is zero.
This distinction also matters near a date/time boundary, where the components are applied separately.

.. erbsland-demo::
    :source: time/CalendarDelta/Combine.cpp
    :exec: time/calendar_delta --demo Combine
    :source-sha256: 864dada1c152be88f3b20664b545484780934718eae5c8121fad97dc3b5312ee

.. code-block:: cpp

    /// Combine changes by component without normalizing or imposing an ordering.
    /// @notest{Compiled and executed documentation demo.}
    void combine() {
        const auto monthly = el::CalendarDelta{el::Months{1}};
        const auto extra = el::CalendarDelta{el::Days{2}};
        const auto combined = monthly + extra;
        auto edited = combined;
        edited -= extra;
        edited += monthly;
        el::io::printLine(
            el::StringFormat{"Combined: {}; edited: {}; difference: {}; negated: {}"_el}.build(
                combined.toString(), edited.toString(), (combined - monthly).toString(), (-combined).toString()));
        const auto minutes = el::CalendarDelta{el::Minutes{60}};
        const auto hour = el::CalendarDelta{el::Hours{1}};
        const auto cancellation = minutes - hour;
        el::io::printLine(
            el::StringFormat{"Same stored parts: {}; cancellation isZero: {}; fixed total: {}"_el}.build(
                minutes == hour, cancellation.isZero(), cancellation.toTimeDeltaOrThrow().toString()));
        el::io::printLine(
            el::StringFormat{"Subtracting identical parts isZero: {}"_el}.build((combined - combined).isZero()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Combined: 1 mo 2 d; edited: 2 mo; difference: 2 d; negated: -1 mo -2 d
    Same stored parts: false; cancellation isZero: false; fixed total: 0 s
    Subtracting identical parts isZero: true

.. erbsland-demo-end::

There is no general ordering or single sign for a calendar delta.
For example, one month minus thirty days may move forward or backward depending on the starting date.
If you need to compare the effects of two calendar rules, apply each to the same valid starting instant and compare the
resulting values.
For deltas containing only fixed components, checked conversion to :cpp:class:`TimeDelta <erbsland::time::TimeDelta>`
can provide a comparable interval when its total fits.

Applying Components in Order
============================

:cpp:func:`DateTime::added() <erbsland::time::DateTime::added>` returns a changed value, while :cpp:func:`add() <erbsland::time::DateTime::add>` updates the value in place.
The corresponding :cpp:func:`subtracted() <erbsland::time::DateTime::subtracted>` and
:cpp:func:`subtract() <erbsland::time::DateTime::subtract>` operations apply the negated delta in the same unit order.
The arithmetic operators provide the same saturating behavior.
Choose the returned-value form when both the starting value and result matter, and the in-place form when advancing a
current value.
Their ``OrThrow`` alternatives give you explicit overflow handling, discussed below.

Application always proceeds from the smallest unit to the largest: nanoseconds, microseconds, milliseconds, seconds,
minutes, hours, days, weeks, months, then years.
The order in which you wrote designated fields or called setters does not change that sequence.
Each step works on the result of the previous one, including any date carry or month-end adjustment.

Starting at January 30, 2024 at 23:30 UTC, two hours first reach January 31 at 01:30. One day then reaches February 1,
one month reaches March 1, and one year reaches March 1, 2025. The demo walks through these stages and compares them
with one combined application.
It also shows why applying a month twice can differ from applying two months once.

.. erbsland-demo::
    :source: time/CalendarDelta/Apply.cpp
    :exec: time/calendar_delta --demo Apply
    :source-sha256: 661fb1bf67074a903f463450c9e71fe38ce5eafeb768140ad9bbdefa6dd3af0b

.. code-block:: cpp

    /// Apply mixed units in order and observe month-end and leap-day clamping.
    /// @notest{Compiled and executed documentation demo.}
    void apply() {
        const auto start =
            el::DateTime{el::Date{el::Year{2024}, el::Month{1}, el::Day{30}}, el::Time{el::Hour{23}, el::Minute{30}}};
        const auto change = el::CalendarDelta{el::CalendarDeltaParts{
            .hours = el::Hours{2}, .days = el::Days{1}, .months = el::Months{1}, .years = el::Years{1}}};
        auto step = start.added(el::CalendarDelta{el::Hours{2}});
        el::io::printLine(el::StringFormat{"After hours: {}"_el}.build(step.toString()));
        step.add(el::CalendarDelta{el::Days{1}});
        el::io::printLine(el::StringFormat{"After days: {}"_el}.build(step.toString()));
        step.addOrThrow(el::CalendarDelta{el::Months{1}});
        el::io::printLine(el::StringFormat{"After months: {}"_el}.build(step.toString()));
        step += el::CalendarDelta{el::Years{1}};
        el::io::printLine(
            el::StringFormat{"After years: {}; combined: {}"_el}.build(
                step.toString(), start.addedOrThrow(change).toString()));
        const auto monthEnd =
            el::DateTime{el::Date{el::Year{2024}, el::Month{1}, el::Day{31}}, el::Time{el::Hour{9}, el::Minute{}}};
        const auto month = el::CalendarDelta{el::Months{1}};
        const auto next = monthEnd + month;
        el::io::printLine(
            el::StringFormat{"Month end: {}; after month: {}"_el}.build(monthEnd.toString(), next.toString()));
        el::io::printLine(el::StringFormat{"Subtract month: {}"_el}.build(next.subtracted(month).toString()));
        el::io::printLine(
            el::StringFormat{"Two at once: {}; two separately: {}"_el}.build(
                (monthEnd + (month + month)).toString(), ((monthEnd + month) + month).toString()));
        el::io::printLine(
            el::StringFormat{"Leap day plus year: {}"_el}.build((next + el::CalendarDelta{el::Years{1}}).toString()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    After hours: 2024-01-31 01:30:00Z
    After days: 2024-02-01 01:30:00Z
    After months: 2024-03-01 01:30:00Z
    After years: 2025-03-01 01:30:00Z; combined: 2025-03-01 01:30:00Z
    Month end: 2024-01-31 09:00:00Z; after month: 2024-02-29 09:00:00Z
    Subtract month: 2024-01-29 09:00:00Z
    Two at once: 2024-03-31 09:00:00Z; two separately: 2024-03-29 09:00:00Z
    Leap day plus year: 2025-02-28 09:00:00Z

.. erbsland-demo-end::

Month and year steps keep the day when it exists in the target month, otherwise clamping it to that month's last day.
January 31 plus one month reaches February 29 in 2024. Subtracting that month returns January 29, because the original
day 31 was lost during clamping.
Adding one year to February 29 reaches February 28 in a common year.
These are calendar adjustments within the supported date range; they do not count as overflow and do not make an
``OrThrow`` operation fail.

Combining the two month changes first keeps a single two-month step, taking January 31 directly to March 31. Applying
them separately clamps in February before continuing to March 29. If an algorithm needs to preserve the original
preferred day across repeated occurrences, keep that day as part of the rule and derive each occurrence from it.
Do not assume that reversing a previous delta reconstructs information lost by clamping.
:doc:`working_with_dates` develops the same month-end rules for individual :cpp:type:`Days <erbsland::time::Days>`, :cpp:type:`Months <erbsland::time::Months>`, and :cpp:type:`Years <erbsland::time::Years>` changes
applied to a :cpp:class:`Date <erbsland::time::Date>`.

Calendar Changes and Local Time
===============================

A display zone tells you how an instant appears locally; it does not change where
:cpp:class:`DateTime <erbsland::time::DateTime>` performs arithmetic.
The UTC date and time receive every delta component.
The result retains a fixed display offset, or resolves the named zone again to obtain the offset and label for the new
instant.

Across a daylight-saving transition, adding a day therefore need not retain the local hour.
In the example, 09:00 on March 30, 2024 in ``Europe/Copenhagen`` becomes 10:00 on March 31 after a UTC day is added.
The elapsed change is one fixed day, and the zone's offset has advanced by one hour.
The same principle applies to month and year changes: they adjust UTC calendar fields rather than promising a particular
local clock reading.

.. erbsland-demo::
    :source: time/CalendarDelta/Zones.cpp
    :exec: time/calendar_delta --demo Zones
    :source-sha256: ce19c42005e7fd0dc7df0cb39573e7eacf2be932cf6c2e392bd1fe56a3850324

.. code-block:: cpp

    /// Distinguish UTC calendar arithmetic from resolving a recurring local time.
    /// @notest{Compiled and executed documentation demo.}
    void zones() {
        const auto zone = el::TimeZone::fromNameOrThrow("Europe/Copenhagen"_el);
        const auto wallTime = el::TimeWithZone{el::Time{el::Hour{9}, el::Minute{}}, zone};
        const auto start = el::DateTime{el::Date{el::Year{2024}, el::Month{3}, el::Day{30}}, wallTime};
        const auto advanced = start.added(el::CalendarDelta{el::Days{1}});
        // Resolve the intended local time again on the next local date.
        const auto recurring = el::DateTime{start.date().next(), wallTime, el::TimeOccurrenceInFold::First};
        el::io::printLine(
            el::StringFormat{"Start: {}; UTC: {} {}"_el}.build(
                start.toString(), start.utcDate().toString(), start.utcTime().toString()));
        el::io::printLine(
            el::StringFormat{"UTC day added: {}; offset seconds: {}"_el}.build(
                advanced.toString(), advanced.timeOffset().toSeconds().toRawValue()));
        el::io::printLine(
            el::StringFormat{"Next local occurrence: {}; requested hour retained: {}"_el}.build(
                recurring.toString(), recurring.hour() == wallTime.hour()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Start: 2024-03-30 09:00:00+01:00; UTC: 2024-03-30 08:00:00
    UTC day added: 2024-03-31 10:00:00+02:00; offset seconds: 7200
    Next local occurrence: 2024-03-31 09:00:00+02:00; requested hour retained: true

.. erbsland-demo-end::

To preserve an intended local time, derive the next local :cpp:class:`Date <erbsland::time::Date>` and combine it again
with the stored
:cpp:class:`TimeWithZone <erbsland::time::TimeWithZone>`.
The demo compares that construction with UTC arithmetic.
Each occurrence then resolves the zone for its own date instead of reusing yesterday's numeric offset.

Some local readings occur twice during a fold or are skipped during a gap.
Choose :cpp:enumerator:`TimeOccurrenceInFold::First <erbsland::time::TimeOccurrenceInFold::First>` or
:cpp:enumerator:`Second <erbsland::time::TimeOccurrenceInFold::Second>` deliberately when the local time is ambiguous.
For a gap, construction follows the zone database transition rule; compare the resolved local fields with the requested
fields when your recurrence policy needs to detect an adjustment.
:cpp:func:`DateTime::isValid() <erbsland::time::DateTime::isValid>` checks date validity, rather than detecting a skipped local reading.
See :doc:`working_with_datetime` for the complete resolution workflow and concrete gap and fold examples.

Handling Range Boundaries
=========================

Range handling answers a different question from month-end adjustment.
Losing January's preferred day in February is an ordinary calendar result; moving beyond the first or last supported
instant exceeds the representation.
Calendar application stays within the supported UTC date range, ``0000-01-01..9999-12-31``.
The saturating forms clamp an overflowing step to the corresponding complete date/time endpoint.
Later components still run, so an overflowing intermediate step can be followed by a component that moves the result
back inside the range.
A final value inside the range does not prove that the calculation avoided saturation.

:cpp:func:`wouldAddSaturate() <erbsland::time::DateTime::wouldAddSaturate>` and :cpp:func:`wouldSubtractSaturate() <erbsland::time::DateTime::wouldSubtractSaturate>` detect overflow in any application step, including unit
conversions required for that step.
:cpp:func:`addedOrThrow() <erbsland::time::DateTime::addedOrThrow>` and :cpp:func:`subtractedOrThrow() <erbsland::time::DateTime::subtractedOrThrow>` report :cpp:class:`OverflowError <erbsland::err::OverflowError>` at the first overflowing step.
The in-place :cpp:func:`addOrThrow() <erbsland::time::DateTime::addOrThrow>` and
:cpp:func:`subtractOrThrow() <erbsland::time::DateTime::subtractOrThrow>` forms offer the same checked policy.
Use the checked forms when clamping would silently change the meaning of your rule.

.. erbsland-demo::
    :source: time/CalendarDelta/Boundaries.cpp
    :exec: time/calendar_delta --demo Boundaries
    :source-sha256: 35785571bf9847112c8520dc9b420f46505b8876d975803bbb8063107e0c8d0c

.. code-block:: cpp

    /// Detect intermediate overflow and choose saturating or throwing application.
    /// @notest{Compiled and executed documentation demo.}
    void boundaries() {
        const auto last = el::DateTime::last();
        const auto tick = el::CalendarDelta{el::Nanoseconds{1}};
        el::io::printLine(
            el::StringFormat{"Upper overflow: {}; lower overflow: {}; clamped high: {}"_el}.build(
                last.wouldAddSaturate(tick), el::DateTime::first().wouldSubtractSaturate(tick), last.added(tick) == last));
        // The first step overflows even though a later day step moves back into range.
        const auto mixed = el::CalendarDelta{el::CalendarDeltaParts{.seconds = el::Seconds{1}, .days = el::Days{-1}}};
        el::io::printLine(
            el::StringFormat{"Intermediate overflow: {}; saturating result: {}"_el}.build(
                last.wouldAddSaturate(mixed), last.added(mixed).toString()));
        try {
            const auto checked = last.addedOrThrow(mixed);
            el::io::printLine(checked.toString());
        } catch (const el::err::OverflowError &) {
            el::io::printLine("Checked application rejects the first overflowing step."_el);
        }
        auto changed = last;
        changed.subtractOrThrow(el::CalendarDelta{el::Days{1}});
        changed.subtract(el::CalendarDelta{el::Months{1}});
        el::io::printLine(
            el::StringFormat{"Changed in place: {}; invalid remains invalid: {}"_el}.build(
                changed.toString(), !el::DateTime{}.addedOrThrow(tick).isValid()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Upper overflow: true; lower overflow: true; clamped high: true
    Intermediate overflow: true; saturating result: 9999-12-30 23:59:59.999999999Z
    Checked application rejects the first overflowing step.
    Changed in place: 9999-11-30 23:59:59.999999999Z; invalid remains invalid: true

.. erbsland-demo-end::

At the last supported instant, adding one second and then subtracting one day still contains an overflowing step.
The saturating result ends one day below the upper endpoint, while checked application rejects the overflowing seconds
component before the day component can compensate.
This is another reason to distinguish a zero fixed total from a delta whose stored fields are all zero.

An invalid starting :cpp:class:`DateTime <erbsland::time::DateTime>` remains invalid, including with the throwing forms,
and its overflow tests return false.
If your workflow requires a valid starting instant, check it before applying the change.
Overflow handling does not replace that input validation.

Converting to a Fixed Interval
==============================

A delta with nonzero months or years cannot be converted to a fixed interval without a starting date.
:cpp:func:`isValidTimeDelta() <erbsland::time::CalendarDelta::isValidTimeDelta>` tests whether conversion succeeds, including its range checks.
A delta of 200,000 days contains only fixed components but fails this test because it exceeds the roughly 292-year
nanosecond range of :cpp:class:`TimeDelta <erbsland::time::TimeDelta>`.

:cpp:func:`toTimeDelta() <erbsland::time::CalendarDelta::toTimeDelta>` returns an optional interval only when there are no calendar components and each fixed component and every intermediate sum fit
the nanosecond range.
It returns an empty optional for either kind of failure.
:cpp:func:`toTimeDeltaOrThrow() <erbsland::time::CalendarDelta::toTimeDeltaOrThrow>` instead throws :cpp:class:`OverflowError <erbsland::err::OverflowError>` in both cases.
Conversion accumulates nanoseconds through weeks in that order.
Each component must fit as nanoseconds, and each intermediate sum must fit too; a later negative component cannot rescue
an earlier overflowing conversion or sum.
It does not apply the delta to a date and does not alter the stored fields.

.. erbsland-demo::
    :source: time/CalendarDelta/Convert.cpp
    :exec: time/calendar_delta --demo Convert
    :source-sha256: 265a0324d1ce4c9f441c6470653d6863b92421e907e138fce7f07c8d0b903334

.. code-block:: cpp

    /// Convert fixed components exactly and distinguish calendar dependence from overflow.
    /// @notest{Compiled and executed documentation demo.}
    void convert() {
        const auto fixed = el::CalendarDelta{
            el::CalendarDeltaParts{.milliseconds = el::Milliseconds{250}, .hours = el::Hours{1}, .days = el::Days{2}}};
        const auto month = el::CalendarDelta{el::Months{1}};
        const auto huge = el::CalendarDelta{el::Days{200'000}};
        const auto intermediate = el::CalendarDelta{el::CalendarDeltaParts{
            .nanoseconds = el::Nanoseconds::maximum(), .seconds = el::Seconds{1}, .minutes = el::Minutes{-1}}};
        if (const auto interval = fixed.toTimeDelta()) {
            el::io::printLine(
                el::StringFormat{"Fixed change: {}; precise interval: {}"_el}.build(
                    fixed.toString(), interval->toString()));
        }
        el::io::printLine(
            el::StringFormat{"Month valid: {}; optional: {}; large valid: {}; optional: {}"_el}.build(
                month.isValidTimeDelta(),
                month.toTimeDelta().has_value(),
                huge.isValidTimeDelta(),
                huge.toTimeDelta().has_value()));
        el::io::printLine(
            el::StringFormat{"Intermediate sum overflow converts: {}"_el}.build(intermediate.isValidTimeDelta()));
        for (const auto &change : {month, huge, intermediate}) {
            try {
                el::io::printLine(change.toTimeDeltaOrThrow().toString());
            } catch (const el::err::OverflowError &) {
                el::io::printLine("No exact fixed interval fits this change."_el);
            }
        }
    }

    }

.. erbsland-ansi::
    :escape-char: ␛

    Fixed change: 2 d 1 h 250 ms; precise interval: 2 d 1 h 250 ms
    Month valid: false; optional: false; large valid: false; optional: false
    Intermediate sum overflow converts: false
    No exact fixed interval fits this change.
    No exact fixed interval fits this change.
    No exact fixed interval fits this change.

.. erbsland-demo-end::

For a calendar-dependent change, first apply it to your chosen starting value.
If you then need the precise elapsed distance between the two instants, convert them to
:cpp:class:`Timestamp <erbsland::time::Timestamp>` and use its checked distance APIs, as explained in
:doc:`working_with_timestamps`.
Keep the original calendar rule when you intend to apply it again: the elapsed distance for one occurrence does not
replace a month or year rule for future dates.
