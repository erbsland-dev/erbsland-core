..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Time; Measuring Elapsed Time
    single: ElapsedTimer; Timing Operations
    single: Monotonic Clock; Time Budgets

**********************
Measuring Elapsed Time
**********************

When you want to know how long an operation takes, the useful result is an interval: how much time passed between
starting and finishing it.
:cpp:class:`ElapsedTimer <erbsland::time::ElapsedTimer>` keeps the starting point for you and returns the elapsed
interval as a :cpp:class:`TimeDelta <erbsland::time::TimeDelta>`.
You can take several readings, start a new phase, or check a time budget while processing input.
This page develops those patterns and explains how to interpret the measurements.

Choosing an Elapsed-Time Clock
==============================

A calendar clock answers when an event happened.
It is useful for a log entry, but its readings can change when the system corrects its clock or someone adjusts it.
Subtracting two such readings can give a misleading duration if an adjustment occurs between them.

An elapsed timer uses a monotonic clock through :cpp:class:`TimePoint <erbsland::time::TimePoint>`.
Successive readings do not move backwards, and adjusting the civil clock does not change your interval measurement.
You can therefore measure an operation without interpreting dates, time zones, or daylight-saving changes.
For recording an event's dated instant, see :doc:`working_with_timestamps`; for checkpoints and shared deadlines, see
:doc:`working_with_monotonic_time_points`.

The resulting ``TimeDelta`` represents intervals in nanoseconds.
That is the resolution of the result type, rather than a promise that the clock observes every nanosecond or measures a
short operation with nanosecond accuracy.
The distinction matters when you interpret a small reading: extra digits retain the available value, but they do not
remove clock granularity or measurement noise.

Timing an Operation
===================

Create the timer immediately before the work you want to measure.
Construction captures the starting point; there is no separate start call.
:cpp:func:`elapsed() <erbsland::time::ElapsedTimer::elapsed>` returns the interval from that point to the current
clock reading.
A local ``const`` timer is useful when you only need readings from one start.
Its header is ``<erbsland/time/ElapsedTimer.hpp>``; the demos use the flattened ``el`` namespace.

Reading the timer leaves its starting point in place.
A second call therefore measures the total since construction, rather than the time since the first call.
This lets you observe progress without changing the final measurement.
Think of successive readings as checkpoints along one measurement.
In the following demo, the first reading covers the sum, and the second covers both the sum and the mean calculation.
Neither reading changes what the timer will measure next.

.. erbsland-demo::
    :source: time/ElapsedTimer/Measure.cpp
    :exec: time/elapsed_timer --demo Measure
    :source-sha256: 8c67a30195825a089a35cb7ffe1ee6fb293a7517e3cab079d205ee6dc78f3ec8

.. code-block:: cpp

    /// Measure an operation with a monotonic timer and read cumulative elapsed intervals.
    /// ElapsedTimer starts on construction; elapsed() does not restart it.
    /// TimeDelta preserves the measured interval for conversion and formatting.
    /// @notest{Compiled and executed documentation demo.}
    void measure() {
        const auto detector = "Algılayıcı"_el;
        const auto samples = std::array{12.0, 14.0, 11.0, 15.0, 13.0, 16.0};
        const auto timer = el::ElapsedTimer{};

        // Calculate a mean, then read elapsed time twice from the same starting point.
        auto total = 0.0;
        for (const auto sample : samples) {
            total += sample;
        }
        const auto firstReading = timer.elapsed();
        const auto mean = total / samples.size();
        const auto secondReading = timer.elapsed();

        // Whole milliseconds may be zero for a short operation; retain the precise interval.
        const auto format = el::TimeDeltaFormat::longUnits().setUnitSeparator(", "_el);
        el::io::printLine(el::StringFormat{"{}: mean {}"_el}.build(detector, mean));
        el::io::printLine(
            el::StringFormat{"First reading: {}; second reading: {}"_el}.build(
                firstReading.toString(), secondReading.toString()));
        el::io::printLine(
            el::StringFormat{"Whole milliseconds: {}; formatted: {}"_el}.build(
                secondReading.toMilliseconds().toRawValue(), secondReading.toString(format)));
        el::io::printLine(
            el::StringFormat{"Second reading is at least the first: {}"_el}.build(secondReading >= firstReading));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Algılayıcı: mean 13.5
    First reading: 333 ns; second reading: 541 ns
    Whole milliseconds: 0; formatted: 541 nanoseconds
    Second reading is at least the first: true

.. erbsland-demo-end::

Keep the returned ``TimeDelta`` while you calculate with the result.
A conversion such as :cpp:func:`toMilliseconds() <erbsland::time::TimeDelta::toMilliseconds>` returns a whole-unit
amount and discards smaller fractions.
A short measurement can consequently have zero whole milliseconds while still containing a useful nonzero interval.
The demo shows the precise result alongside the whole-millisecond conversion and a format with long unit names.
You can choose a shorter display later without discarding precision from the stored interval.
:doc:`working_with_time_deltas` explains the conversion choices, and :doc:`customizing_time_interval_output` explains
how to choose display units and fractional digits.

Printing is also work, so the demo saves both readings before writing its results.
In your own code, place the reading before any logging that should be excluded from the measured operation.
The captured output illustrates one execution; its timings will differ on another run or machine.

Timing Successive Phases
========================

When an operation has several phases, separate measurements help you see which part takes the time.
:cpp:func:`restart() <erbsland::time::ElapsedTimer::restart>` captures a new starting point in the same timer.
Save the previous elapsed interval first if you need it: ``restart()`` returns no interval.
The next ``elapsed()`` call measures from the restart, including any work between the restart and the reading.
The clock is read again when you query the timer, so a reading immediately after restarting can already be greater than
zero.

Copying a timer copies its starting point.
The two timers initially measure from the same point, but restarting one does not restart the other.
This gives you a simple way to keep an overall measurement while reusing another timer for individual phases.
Copy assignment follows the same rule: the assigned timer receives the source timer's current starting point.

.. erbsland-demo::
    :source: time/ElapsedTimer/Phases.cpp
    :exec: time/elapsed_timer --demo Phases
    :source-sha256: 03bd454cabd88245a3303b7c6c72747eea91b99803fc9d1dad043cae9367eaf1

.. code-block:: cpp

    /// Restart a timer between phases and copy its starting point for a total measurement.
    /// Save elapsed() before restart(); restart() returns no previous interval.
    /// A restart changes only the timer receiving the call, not its copies.
    /// @notest{Compiled and executed documentation demo.}
    void phases() {
        const auto sensor = "Işık sensörü"_el;
        auto samples = std::array{12.0, 14.0, 11.0, 15.0, 13.0, 16.0};
        auto phaseTimer = el::ElapsedTimer{};
        const auto totalTimer = phaseTimer;

        // First phase: remove the detector's baseline from every sample.
        for (auto &sample : samples) {
            sample -= 2.0;
        }
        const auto calibrationElapsed = phaseTimer.elapsed();
        phaseTimer.restart();

        // Second phase: combine the calibrated readings.
        auto total = 0.0;
        for (const auto sample : samples) {
            total += sample;
        }
        const auto reductionElapsed = phaseTimer.elapsed();
        const auto totalElapsed = totalTimer.elapsed();

        el::io::printLine(el::StringFormat{"{}: calibrated total {}"_el}.build(sensor, total));
        el::io::printLine(
            el::StringFormat{"Calibration: {}; reduction: {}; total: {}"_el}.build(
                calibrationElapsed.toString(), reductionElapsed.toString(), totalElapsed.toString()));
        el::io::printLine(
            el::StringFormat{"Copied timer still includes the first phase: {}"_el}.build(
                totalElapsed >= calibrationElapsed + reductionElapsed));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Işık sensörü: calibrated total 69
    Calibration: 125 ns; reduction: 84 ns; total: 583 ns
    Copied timer still includes the first phase: true

.. erbsland-demo-end::

The phase intervals cover the work inside their respective boundaries.
The total also includes the small gaps between readings and the restart, so it need not equal the sum of the two phases.
Choose those boundaries to match the question you are asking: the total describes the caller's wait, while a phase
measurement helps you investigate one part of it.

Checking a Time Budget
======================

An elapsed timer can also help you limit how much work one call attempts.
Represent the budget as a ``TimeDelta`` and compare it with ``elapsed()`` before starting another unit of work.
This uses the same monotonic clock as operation timing, so a civil-clock correction does not extend or shorten the
budget.

Checking once per batch keeps the loop readable and reduces the number of clock queries.
The demo processes finite input, retains its progress, and stops either when the input is complete or when the budget
has been reached.
Input preparation happens before timer construction, so the budget covers processing rather than allocation.

.. erbsland-demo::
    :source: time/ElapsedTimer/Budget.cpp
    :exec: time/elapsed_timer --demo Budget
    :source-sha256: 3d6161b18a6f7dda24003ac216f78557321ebe46b2c1bd773e8512fa511a091e

.. code-block:: cpp

    /// Process finite input in batches while checking a monotonic time budget.
    /// Checking elapsed() between batches lets a caller retain its progress and continue later.
    /// A time-budget check cannot interrupt the batch already in progress.
    /// @notest{Compiled and executed documentation demo.}
    void budget() {
        using namespace el::time::literals;
        const auto detector = "Algılayıcı"_el;
        const auto samples = std::vector<double>(1'000'000, 12.0);
        const auto timeBudget = el::TimeDelta{2_ms};
        constexpr auto cBatchSize = std::size_t{256};
        auto processed = std::size_t{0};
        auto total = 0.0;
        const auto timer = el::ElapsedTimer{};

        // Keep an input boundary as well as a time boundary, and do useful work in each batch.
        while (processed < samples.size() && timer.elapsed() < timeBudget) {
            const auto end = std::min(processed + cBatchSize, samples.size());
            for (; processed < end; ++processed) {
                total += samples[processed] - 2.0;
            }
        }
        const auto elapsed = timer.elapsed();

        el::io::printLine(
            el::StringFormat{"{}: processed {} of {} samples; calibrated total {}"_el}.build(
                detector, processed, samples.size(), total));
        el::io::printLine(
            el::StringFormat{"Budget: {}; elapsed: {}; complete: {}"_el}.build(
                timeBudget.toString(), elapsed.toString(), processed == samples.size()));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Algılayıcı: processed 886784 of 1000000 samples; calibrated total 8867840
    Budget: 2 ms; elapsed: 2 ms 833 ns; complete: false

.. erbsland-demo-end::

A budget check is cooperative: it decides whether another batch should begin, but it cannot interrupt a batch already in
progress.
The elapsed time can exceed the budget by the time needed for that batch, and scheduling can add a further delay.
Smaller batches let the loop check more often; larger batches spend less time reading the clock.
A completed-input condition remains useful even when the clock has coarse resolution or the work finishes quickly.

The ``processed`` count and accumulated result tell you how much input the call handled.
A caller can keep those values and resume from that position later rather than processing the same input again.
The exact number completed under the budget depends on the execution environment.

A fresh timer naturally gives each call a new budget.
If several functions must share one end point, an explicit ``TimePoint`` deadline makes that intent easier to pass
between them.
:doc:`working_with_monotonic_time_points` shows how to construct that point, compare it with the current reading,
and calculate a signed remaining interval.

Reading Measurements Sensibly
=============================

An elapsed measurement includes the time during which your operation was waiting to run, as well as the time it spent
executing.
It is useful for understanding how long the caller waits; it is not a count of CPU time consumed by your code.
Other activity, thread scheduling, and the clock queries themselves can all affect a short measurement.
In particular, two immediate clock readings may be equal; a zero interval does not establish that an operation took no
time.

For very small operations, time a useful group of repetitions and examine several measurements rather than treating a
single reading as a stable performance result.
Keep the computed result observable so that an optimizing compiler cannot simply remove the work.
Decide whether setup, allocation, and output belong inside your measurement, and keep that decision consistent when
comparing alternatives.
The small demos here teach timer use; their output is not a benchmark of the calculations they contain.
