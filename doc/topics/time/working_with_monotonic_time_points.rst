..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Time; Monotonic Time Points
    single: TimePoint; Checkpoints and Deadlines
    single: Steady Clock; Interoperability

**********************************
Working with Monotonic Time Points
**********************************

A timeout often belongs to an entire operation rather than one function call.
For example, preparation and processing may need to share one total allowance.
Recording a deadline lets each part ask whether time remains, without starting a new timeout whenever control moves
elsewhere.
:cpp:class:`TimePoint <erbsland::time::TimePoint>` provides those deadlines and checkpoints on a monotonic clock.
This page explains recording points, calculating signed intervals, adjusting deadlines, and passing them to standard
library APIs that use the same clock.

Choosing the Clock for Your Task
================================

A ``TimePoint`` records a reading of ``std::chrono::steady_clock``.
Successive readings do not move backwards, so changing the system's civil clock does not invalidate an interval or shift
a deadline based on this clock.
For a simple measurement from one start, :doc:`measuring_elapsed_time` shows how ``ElapsedTimer`` keeps the point for
you.
An explicit ``TimePoint`` is useful when your code needs several checkpoints or one deadline shared by several calls.

A monotonic clock has an unspecified epoch: its zero point has no portable calendar meaning.
A ``TimePoint`` consequently cannot tell you the date or time of day at which something happened.
Keep these points within the same clock domain, and do not persist them as event timestamps or compare their values
across machines or restarts.
:doc:`working_with_timestamps` covers stored and exchanged instants; :doc:`working_with_datetime` adds calendar fields
and a display zone.

Distances between monotonic points are :cpp:class:`TimeDelta <erbsland::time::TimeDelta>` values, with nanosecond
resolution.
That representation retains the available precision, but the actual granularity and accuracy of a measurement depend on
the clock and the environment.
Choosing nanoseconds for output does not make the clock more precise.

Recording Checkpoints
=====================

:cpp:func:`TimePoint::now() <erbsland::time::TimePoint::now>` captures the current monotonic reading.
Call it at the boundary you want to remember, such as before starting work or immediately after receiving a result.
The type is declared in ``<erbsland/time/TimePoint.hpp>`` and is available through the flattened ``el`` namespace used
in the demos.

Default construction has a different purpose: ``TimePoint{}`` selects the steady clock's epoch.
It does not sample the clock and is not a missing-value marker.
If a checkpoint is optional in your application, represent that separately, for example with
``std::optional<TimePoint>``.
An explicit constructor also accepts a ``std::chrono::steady_clock::time_point`` when another API has already captured
the reading.

.. erbsland-demo::
    :source: time/TimePoint/Checkpoints.cpp
    :exec: time/time_point --demo Checkpoints
    :source-sha256: a42fffe1ecf6acbc606af002a98bcbf5747c20fc632bb6918951450ab67cc473

.. code-block:: cpp

    /// Capture monotonic checkpoints explicitly and preserve steady-clock values on construction.
    /// Default TimePoint construction selects the clock epoch, rather than the current time.
    /// @notest{Compiled and executed documentation demo.}
    void checkpoints() {
        const auto instrument = "Teleskop"_el;
        const auto epoch = el::TimePoint{};
        const auto checkpoint = el::TimePoint::now();
        const auto standardPoint = std::chrono::steady_clock::now();
        const auto imported = el::TimePoint{standardPoint};

        el::io::printLine(el::StringFormat{"{}: checkpoint recorded"_el}.build(instrument));
        el::io::printLine(
            el::StringFormat{"Default point is the steady-clock epoch: {}"_el}.build(
                epoch.toStdTimePoint() == std::chrono::steady_clock::time_point{}));
        el::io::printLine(
            el::StringFormat{"Imported point preserves its value: {}"_el}.build(
                imported.toStdTimePoint() == standardPoint));
        el::io::printLine(
            el::StringFormat{"Imported checkpoint is at least the earlier reading: {}"_el}.build(imported >= checkpoint));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Teleskop: checkpoint recorded
    Default point is the steady-clock epoch: true
    Imported point preserves its value: true
    Imported checkpoint is at least the earlier reading: true

.. erbsland-demo-end::

The demo checks the default value against the standard clock's default point, rather than assigning it a civil date.
It also compares two successive readings.
Equal readings are possible when the clock cannot distinguish such a short interval, so the later reading is at least
the earlier one rather than necessarily greater.
Copies preserve a recorded point; they do not capture a fresh reading.

Calculating Elapsed Intervals
=============================

If you have ``start`` and ``end`` checkpoints, ``end - start`` gives the interval from start to end.
:cpp:func:`start.timeDeltaTo(end) <erbsland::time::TimePoint::timeDeltaTo>` expresses the same direction.
Read ``timeDeltaTo()`` from left to right: from the point receiving the call to the point in its argument.
Swapping the endpoints changes the sign, which is useful when the same calculation must describe either time remaining
or time overdue.
Subtracting a point from itself gives a zero interval.

The demo uses a known offset to make the forward and reverse distances easy to compare.
Creating that later point does not wait for the clock to reach it.
The final reading uses :cpp:func:`timeDeltaToNow() <erbsland::time::TimePoint::timeDeltaToNow>` and therefore varies
between runs.
It is taken after printing the planned intervals, so this particular elapsed measurement includes that output work.

.. erbsland-demo::
    :source: time/TimePoint/Intervals.cpp
    :exec: time/time_point --demo Intervals
    :source-sha256: 594f721b3ccc727f4308f82c6d3c7ab8e7ffd690a15b2c0a88e8cee33f62c0c4

.. code-block:: cpp

    /// Calculate signed distances between monotonic points and from a point to now.
    /// end - start and start.timeDeltaTo(end) have the same direction.
    /// Reversing the endpoints negates the interval.
    /// @notest{Compiled and executed documentation demo.}
    void intervals() {
        using namespace el::time::literals;
        const auto checkpoint = el::TimePoint::now();

        // A planned offset gives exact examples without waiting for a particular elapsed reading.
        const auto nextCapture = checkpoint + el::TimeDelta{25_ms};
        const auto forward = nextCapture - checkpoint;
        const auto backward = nextCapture.timeDeltaTo(checkpoint);
        el::io::printLine(el::StringFormat{"Forward: {}; reverse: {}"_el}.build(forward.toString(), backward.toString()));
        el::io::printLine(
            el::StringFormat{"Subtraction and timeDeltaTo agree: {}; points ordered: {}"_el}.build(
                forward == checkpoint.timeDeltaTo(nextCapture), checkpoint < nextCapture));
        el::io::printLine(
            el::StringFormat{"Planned interval in whole milliseconds: {}"_el}.build(forward.toMilliseconds().toRawValue()));

        // This is a real clock query, so its result varies between runs.
        const auto elapsed = checkpoint.timeDeltaToNow();
        el::io::printLine(el::StringFormat{"Elapsed since the checkpoint: {}"_el}.build(elapsed.toString()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Forward: 25 ms; reverse: -25 ms
    Subtraction and timeDeltaTo agree: true; points ordered: true
    Planned interval in whole milliseconds: 25
    Elapsed since the checkpoint: 352 us 41 ns

.. erbsland-demo-end::

``start.timeDeltaToNow()`` samples the clock and returns ``TimePoint::now() - start``.
A start already reached gives a nonnegative interval; a point still in the future gives a negative one.
When several decisions must use the same instant, capture ``now()`` once and calculate all their intervals from that
value instead of taking a fresh reading for each decision.

Points support equality and ordering within their clock domain.
The resulting intervals can be compared, converted, and formatted through ``TimeDelta``.
:doc:`working_with_time_deltas` explains those calculations and whole-unit truncation;
:doc:`customizing_time_interval_output` covers the display choices.

Representing a Deadline
=======================

:cpp:func:`TimePoint::inFuture() <erbsland::time::TimePoint::inFuture>` captures the current point and adds a
``TimeDelta``.
For example, ``TimePoint::inFuture(TimeDelta{25_ms})`` represents a point twenty-five milliseconds from that reading.
The interval is signed: zero selects the current reading, while a negative value selects a point in the past.
The method records the point; waiting, scheduling work, or checking it later is the caller's responsibility.

Adding an interval moves a point forward by that interval, and subtracting one moves it backwards.
``+=`` and ``-=`` update the existing point, while ``+`` and ``-`` return a new point.
If you already have a start checkpoint, ``start + timeout`` anchors the deadline to that start, avoiding an accidental
new timeout at a later stage of the operation.

.. erbsland-demo::
    :source: time/TimePoint/Deadline.cpp
    :exec: time/time_point --demo Deadline
    :source-sha256: 82ab9ea3a145637de1aed1d592d776eaa20420774e5d9348981bce3bcaebe38c

.. code-block:: cpp

    /// Represent and adjust a monotonic deadline without waiting or scheduling work.
    /// Subtract a captured current point to obtain a signed remaining interval.
    /// A nonpositive interval means the deadline has been reached or passed.
    /// @notest{Compiled and executed documentation demo.}
    void deadline() {
        using namespace el::time::literals;
        const auto instrument = "Teleskop"_el;
        auto captureDeadline = el::TimePoint::inFuture(el::TimeDelta{25_ms});

        // Allow extra preparation time, then reserve a small interval before the capture.
        captureDeadline += el::TimeDelta{5_ms};
        captureDeadline -= el::TimeDelta{1_ms};
        const auto preparationDeadline = captureDeadline - el::TimeDelta{2_ms};
        const auto extendedDeadline = captureDeadline + el::TimeDelta{10_ms};

        // Capture now once so the comparison and remaining interval use the same reading.
        const auto now = el::TimePoint::now();
        const auto remaining = now.timeDeltaTo(captureDeadline);
        const auto expiredDeadline = now - el::TimeDelta{2_ms};
        el::io::printLine(
            el::StringFormat{"{}: deadline reached: {}; remaining: {}"_el}.build(
                instrument, now >= captureDeadline, remaining.toString()));
        el::io::printLine(
            el::StringFormat{"Preparation before capture: {}; extension after capture: {}"_el}.build(
                (preparationDeadline < captureDeadline), (extendedDeadline > captureDeadline)));
        el::io::printLine(
            el::StringFormat{"Expired deadline reached: {}; remaining: {}"_el}.build(
                now >= expiredDeadline, (expiredDeadline - now).toString()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Teleskop: deadline reached: false; remaining: 28 ms 999 us 375 ns
    Preparation before capture: true; extension after capture: true
    Expired deadline reached: true; remaining: -2 ms

.. erbsland-demo-end::

A deadline becomes useful when you compare it with a current reading.
At a captured ``now``, ``now >= deadline`` means the deadline has been reached or passed.
The remaining interval is ``deadline - now``, equivalently ``now.timeDeltaTo(deadline)``.
It is positive before the deadline, zero at the deadline, and negative after it.
The demo also constructs an already-expired deadline so that the negative result is visible regardless of how long
printing or scheduling takes.

A deadline check controls whether your code starts more work; it does not interrupt an operation already running.
Check at suitable boundaries or pass the same deadline to an API that accepts one.
The comparison and remaining interval in the demo use one captured reading, so they describe one decision consistently.
A later decision should capture a fresh reading.

``TimePoint`` arithmetic follows the underlying steady clock's representable range.
Keep the shifted point within that clock's range and the point-to-point difference within both the clock duration's
range and the ``TimeDelta`` range.
Unlike the saturating arithmetic described for ``Duration`` and ``TimeDelta``, point offsets do not clamp an
out-of-range clock reading to a supported endpoint.
Ordinary short-lived timeouts are the intended setting; extremely large offsets are not a way to express an unlimited
wait.

Interoperating with Standard Library APIs
=========================================

Some standard library APIs accept a clock-specific time point, such as ``std::this_thread::sleep_until()``.
:cpp:func:`toStdTimePoint() <erbsland::time::TimePoint::toStdTimePoint>` gives you the underlying
``std::chrono::steady_clock::time_point`` for that boundary.
Construction from the same standard type preserves its value, so a round trip keeps the original deadline.

.. erbsland-demo::
    :source: time/TimePoint/Interop.cpp
    :exec: time/time_point --demo Interop
    :source-sha256: 69042f7007956a2353ef2eaec59b83a8bd6be4a88810a698d4a4acc9c2b63f88

.. code-block:: cpp

    /// Pass a monotonic deadline to a standard-library API in the same steady-clock domain.
    /// toStdTimePoint() and the TimePoint constructor preserve a steady-clock reading.
    /// sleep_until() may return later than its requested deadline because of scheduling.
    /// @notest{Compiled and executed documentation demo.}
    void interop() {
        using namespace el::time::literals;
        const auto instrument = "Teleskop"_el;
        const auto start = el::TimePoint::now();
        const auto captureDeadline = start + el::TimeDelta{2_ms};
        const auto standardDeadline = captureDeadline.toStdTimePoint();
        const auto importedDeadline = el::TimePoint{standardDeadline};

        // The standard API receives a steady-clock point, retaining the original clock domain.
        std::this_thread::sleep_until(standardDeadline);
        const auto finished = el::TimePoint::now();
        el::io::printLine(
            el::StringFormat{"{}: deadline round trip preserved: {}"_el}.build(
                instrument, importedDeadline == captureDeadline));
        el::io::printLine(
            el::StringFormat{"Elapsed after waiting: {}; deadline reached: {}"_el}.build(
                (finished - start).toString(), finished >= captureDeadline));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Teleskop: deadline round trip preserved: true
    Elapsed after waiting: 2 ms 508 us 42 ns; deadline reached: true

.. erbsland-demo-end::

Here, ``sleep_until()`` performs the wait; constructing and converting the deadline only supplies its target point.
Thread scheduling can make the return later than requested, so the elapsed output is illustrative rather than an exact
two-millisecond promise.

Retain the steady-clock type when handing a deadline to another API.
A ``system_clock::time_point`` belongs to the civil clock and cannot be substituted by copying a numeric count from
``TimePoint``.
If an API expects a relative interval instead, calculate the remaining ``TimeDelta`` at the call boundary and handle an
already-reached deadline before passing it onward.
That keeps the original end point shared across the operation rather than renewing the full timeout at each step.
