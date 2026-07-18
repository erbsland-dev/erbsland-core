..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Streams; Readiness
    single: Streams; Timeouts
    single: Streams; Retry Loops

**********************
Readiness and Timeouts
**********************

Byte and text streams give application code one interface for data that may come from a file, pipe, terminal, memory, or
another native source.
The byte variants preserve the original representation, while the text variants add decoding, encoding, and
line-oriented operations.
In both cases, the application should not need a different control-flow model merely because the source or destination
changes.

The timing of these streams can still differ considerably.
A memory stream is ready immediately, a pipe may wait for its producer, and an output target may temporarily stop
accepting data.
An unrestricted read or write could therefore hold a thread forever when a device stalls or another process stops
responding.
Erbsland Core avoids this hidden blocking: every public stream operation waits for at most the timeout fixed when the
stream was opened, then returns control to the caller.

This page explains how to use readiness as a hint, how to wait without giving up control indefinitely, and how to build
complete input and output loops around timeout results.

Overview: Keep Every Wait Under Application Control
===================================================

:cpp:class:`InputStream <erbsland::stream::InputStream>` and
:cpp:class:`OutputStream <erbsland::stream::OutputStream>` define the readiness contract shared by byte and text
streams.
Their immutable :cpp:class:`InputStreamSettings <erbsland::stream::InputStreamSettings>` and
:cpp:class:`OutputStreamSettings <erbsland::stream::OutputStreamSettings>` contain the timeout for one public
operation.
The default is one second, but stream creation options can select a value appropriate for the application.

.. mermaid::

    classDiagram
        class InputStreamSettings {
            +timeout() TimeDelta
        }
        class OutputStreamSettings {
            +timeout() TimeDelta
        }
        class InputStream {
            +inputSettings() InputStreamSettings
            +isReady() bool
            +waitForReady() StreamWaitStatus
        }
        class OutputStream {
            +outputSettings() OutputStreamSettings
            +isReady() bool
            +waitForReady() StreamWaitStatus
            +flush() StreamWriteStatus
        }
        class ByteInputStream
        class TextInputStream
        class ByteOutputStream
        class TextOutputStream
        class StreamWaitStatus {
            Ready
            Timeout
        }
        class StreamReadStatus {
            Data
            Finished
            Timeout
        }
        class StreamWriteStatus {
            Success
            Timeout
        }
        InputStream <|-- ByteInputStream
        InputStream <|-- TextInputStream
        OutputStream <|-- ByteOutputStream
        OutputStream <|-- TextOutputStream
        InputStream --> InputStreamSettings : exposes
        OutputStream --> OutputStreamSettings : exposes
        InputStream ..> StreamWaitStatus : readiness wait
        OutputStream ..> StreamWaitStatus : readiness wait
        InputStream ..> StreamReadStatus : read result
        OutputStream ..> StreamWriteStatus : write or flush result

The APIs answer different questions and are intentionally kept separate:

.. list-table:: Readiness and operation results
    :header-rows: 1
    :widths: 24 28 48

    *   - API
        - Result
        - Meaning
    *   - ``isReady()``
        - ``bool``
        - A non-blocking snapshot says whether the stream can make progress now.
    *   - ``waitForReady()``
        - :cpp:class:`StreamWaitStatus <erbsland::stream::StreamWaitStatus>`
        - A bounded wait reports whether readiness was reached before this operation's deadline.
    *   - Input operation
        - :cpp:class:`StreamReadStatus <erbsland::stream::StreamReadStatus>`
        - The operation reports data, normal end, or timeout.
    *   - Output operation
        - :cpp:class:`StreamWriteStatus <erbsland::stream::StreamWriteStatus>`
        - The complete request was accepted, or none of it was accepted before timeout.

A timeout is ordinary bounded flow control.
It does not fail the stream, and on input it does not mean end-of-stream.

Check Whether a Stream Is Ready
===============================

Use ``isReady()`` when your code must decide immediately whether to perform an operation or continue with other work.
The check never waits, consumes input, or modifies output.
It is a snapshot of the stream at that moment, not a replacement for the read or write operation.

Input Streams
-------------

:cpp:func:`InputStream::isReady() <erbsland::stream::InputStream::isReady>` is useful when polling a stream for new
data, for example from an event loop, or when you want to avoid a read that clearly has to wait.
If it returns ``false``, no read can make immediate progress, so continue other work and try again later.
If it returns ``true``, the next read can immediately return some data or a final state such as end-of-stream.

Readiness does not tell you how much input is available.
An aggregate operation such as :cpp:func:`readExact() <erbsland::stream::ByteInputStream::readExact>` or
:cpp:func:`readLine() <erbsland::stream::TextInputStream::readLine>` may start with ready data and still return
``Timeout`` because the complete byte sequence or line does not arrive before the operation timeout.
Therefore, combine polling with short reads when you want to keep the current thread responsive.
A byte-oriented :cpp:func:`readByte() <erbsland::stream::ByteInputStream::readByte>` or
:cpp:func:`read(maximum) <erbsland::stream::ByteInputStream::read>` are good choices because they are guaranteed to
succeed when data is ready.

Text input requires extra care.
A ready text stream may currently have only the first byte of a multibyte encoded character.
Even :cpp:func:`readChar() <erbsland::stream::TextInputStream::readChar>` can then wait for the remaining bytes and
return ``Timeout``.
Always handle the result of a read, regardless of the preceding readiness check.

Output Streams
--------------

:cpp:func:`OutputStream::isReady() <erbsland::stream::OutputStream::isReady>` is useful before preparing or writing a
large block, or when a thread should not wait while the destination is stalled.
If it returns ``false``, output is queued in the back buffer.
You can postpone the next block instead of adding more data to this limited queue.
Read :doc:`buffering_and_back_pressure` for the complete front-buffer, back-buffer, and request-size rules.

If ``isReady()`` returns ``true``, the back buffer is empty.
For a stream with one producer, the following write can be accepted without waiting when its encoded size fits within
:cpp:func:`backBufferLimit() <erbsland::stream::OutputStreamSettings::backBufferLimit>` and the stream state does not
change between the check and the write.
In this controlled case, the returned write status is ``Success`` and may be ignored; a stream failure is still reported
as :cpp:class:`StreamError <erbsland::stream::StreamError>`.
If another producer can access the stream, or the state can otherwise change, inspect the write status as usual.

.. warning::

    Do not use ``isReady()`` to coordinate multiple threads that read from or write to the same stream.
    Another thread can consume input or fill output capacity between the check and the following operation.
    In this unusual arrangement, synchronize stream ownership or rely directly on the bounded operation and its result.

.. rubric:: Demo

The following demo polls a forest sensor without waiting when no data is available.
When the sensor reports readiness, it performs one short byte read and handles every possible result.

.. erbsland-demo::
    :source: stream/ReadinessAndTimeouts/CheckReadiness.cpp
    :exec: stream/readiness_and_timeouts --demo CheckReadiness
    :source-sha256: 2bbf901888cf92ab286d6774b9f2d772bdc431d238ce372352cf162957c5c842

.. code-block:: cpp

    /// Use `isReady()` when the current thread must not wait.
    /// Readiness is only a snapshot: it neither consumes data nor reserves it. If the stream is ready, still inspect the
    /// following read result because another consumer or a state change can invalidate the snapshot.
    void readAvailableObservation(el::ByteInputStream &input) {
        if (!input.isReady()) {
            el::io::printLine("The sensor is not ready yet; continuing with other work."_el);
            return;
        }

        try {
            const auto result = input.readByte();
            if (result.isTimeout()) {
                el::io::printLine("Readiness changed before the read."_el);
            } else if (result.isFinished()) {
                el::io::printLine("The observation series is complete."_el);
            } else {
                el::io::printLine("Available observation: "_el, result.data().toUInt8());
            }
        } catch (const el::StreamError &) {
            throw el::RuntimeError{"The forest sensor could not be read."_el, std::current_exception()};
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    The sensor is not ready yet; continuing with other work.

.. erbsland-demo-end::

Wait Until a Stream Is Ready
============================

Use ``waitForReady()`` when waiting is useful, but the current thread must regain control after the configured stream
timeout.
The method returns :cpp:class:`StreamWaitStatus <erbsland::stream::StreamWaitStatus>` with either ``Ready`` or
``Timeout``.
An ordinary timeout is a control-flow result, not an exception.

Input Streams
-------------

:cpp:func:`InputStream::waitForReady() <erbsland::stream::InputStream::waitForReady>` lets you wait for input in a
controlled way without starting a read.
This is useful when the timeout branch should check cancellation, update progress, run scheduled work, or stop the
workflow after an application-level limit.

After ``Ready``, perform the intended read and handle its result.
As with ``isReady()``, readiness promises only that some input or a final state is available; an operation that needs
more input can still time out.
After ``Timeout``, decide whether the workflow should wait again or return to other work.
Do not repeat the wait forever without a cancellation condition, attempt limit, or wider deadline.

Output Streams
--------------

:cpp:func:`OutputStream::waitForReady() <erbsland::stream::OutputStream::waitForReady>` throttles a producer while the
back buffer is in use.
Call it before preparing the next expensive block, or between blocks of a large transfer, so a slow destination does not
keep the output queue filled.
When the result is ``Ready``, prepare and submit one request within ``backBufferLimit()``.
When it is ``Timeout``, reschedule the work, check cancellation, or apply the workflow's retry limit.

Calling ``waitForReady()`` before every read or small write is unnecessary because those operations already perform
their own bounded wait.
Also remember that ``waitForReady()`` and the following operation have separate timeouts.
If both use their complete timeout, the pair can take approximately twice the configured duration.

.. rubric:: Demo

The following demo waits for a forest sensor that misses its first deadline and becomes ready on the second attempt.
Each wait is bounded by the stream timeout, while the attempt limit keeps the complete workflow finite.

.. erbsland-demo::
    :source: stream/ReadinessAndTimeouts/WaitForInput.cpp
    :exec: stream/readiness_and_timeouts --demo WaitForInput
    :source-sha256: d482978e290fd2df8126bd58a57635c4ea8b464a37d2f88c4fa29ab081000144

.. code-block:: cpp

    /// Wait for readiness without surrendering control indefinitely.
    /// Each `waitForReady()` call uses the stream timeout. A separate attempt limit gives this workflow an overall bound
    /// and leaves a timeout available for cancellation checks, progress updates, or other scheduled work.
    void waitForNextObservation(el::ByteInputStream &input) {
        constexpr auto cMaximumWaits = 3U;

        try {
            for (auto attempt = 1U; attempt <= cMaximumWaits; ++attempt) {
                if (input.waitForReady().isReady()) {
                    el::io::printLine("Sensor ready after "_el, attempt, " waits."_el);
                    return;
                }
                el::io::printLine("Wait timed out; checking whether the work is still needed."_el);
            }
        } catch (const el::StreamError &) {
            throw el::RuntimeError{"The forest sensor could not be awaited."_el, std::current_exception()};
        }
        throw el::RuntimeError{"The sensor did not become ready within the observation limit."_el};
    }

.. erbsland-ansi::
    :escape-char: ␛

    Wait timed out; checking whether the work is still needed.
    Sensor ready after 2 waits.

.. erbsland-demo-end::

Handle Operation Timeouts
=========================

Every read, write, and flush uses the timeout configured when the stream was opened.
Choose this timeout according to how quickly the current thread must regain control, not according to the expected
duration of the complete transfer.
The surrounding workflow then decides whether to retry, reschedule, cancel, or stop after a wider limit.
This second limit can be an attempt count, an elapsed-time deadline, or a cancellation request.

Input Timeouts
--------------

A read result has three distinct states:

``Data``
    The operation completed with a payload, which is available through ``data()`` or ``takeData()``.

``Finished``
    The input ended normally without another payload.

``Timeout``
    The operation could not produce a complete result before its deadline.
    The stream may produce more input later.

Never treat ``Timeout`` as ``Finished``, because doing so can silently truncate the input.
Instead, decide whether the workflow should repeat the read, return to other work, or stop.
A count of consecutive timeouts is useful when a source should make regular progress; reset the count whenever data
arrives.
An absolute deadline or cancellation token is often a better fit for an interactive workflow.

Short read operations can simply be attempted again.
After an aggregate operation such as ``readExact()``, ``readLine()``, or ``readAll()`` times out, repeat the same method
with the same arguments.
These methods retain incomplete input internally and resume the logical operation on the next call.
Do not request only the missing suffix or switch operations to reconstruct partial progress yourself.

A failed input source is different from a timeout and throws ``StreamError``.
Catch it where you can add useful application context or decide how to recover from a failed source.

.. rubric:: Demo

The following demo reads a series of forest measurements.
It treats normal completion, temporary inactivity, and a failed source as separate events, and stops after three
consecutive timeouts.

.. erbsland-demo::
    :source: stream/ReadinessAndTimeouts/RetryTimedOutRead.cpp
    :exec: stream/readiness_and_timeouts --demo RetryTimedOutRead
    :source-sha256: 7adbb184bad0c20ce584ba6eb3b53d144698f30125a1ef1927c42c8a7fdf4d97

.. code-block:: cpp

    /// Process data, timeout, and normal end as three separate input states.
    /// Reset the stall budget whenever data arrives. A finite consecutive-timeout limit prevents a silent source from
    /// keeping the workflow alive forever, while `StreamError` remains the exceptional path for a failed source.
    auto readForestMeasurements(el::ByteInputStream &input) -> ForestReadSummary {
        constexpr auto cMaximumConsecutiveTimeouts = 3U;
        auto result = ForestReadSummary{};
        auto consecutiveTimeouts = 0U;

        try {
            while (true) {
                const auto readResult = input.readByte();
                if (readResult.isTimeout()) {
                    ++result.timeoutCount;
                    if (++consecutiveTimeouts == cMaximumConsecutiveTimeouts) {
                        throw el::RuntimeError{"The forest sensor remained inactive for too long."_el};
                    }
                    continue;
                }
                if (readResult.isFinished()) {
                    return result;
                }

                consecutiveTimeouts = 0U;
                ++result.measurementCount;
            }
        } catch (const el::StreamError &) {
            throw el::RuntimeError{"The forest measurements could not be read."_el, std::current_exception()};
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Timeout: 1
    Forest measurements: 3

.. erbsland-demo-end::

Output Timeouts
---------------

Output operations use an atomic acceptance contract.
``Success`` means that the stream accepted the complete request, while ``Timeout`` means that it accepted none of the
request before the deadline.
After a timeout, keep the source data unchanged and repeat the complete write when the workflow permits another attempt.
Never retry a guessed suffix; there is no partial accepted prefix to account for.

A request larger than ``backBufferLimit()`` is not a temporary capacity problem.
It throws ``StreamError`` without accepting data and must be split into valid chunks or rejected.
Other output failures also throw ``StreamError`` and should not enter a timeout retry loop.

A successful write confirms acceptance into the stream, not delivery to the native destination.
Use :cpp:func:`flush() <erbsland::stream::OutputStream::flush>` when delivery must complete before the workflow
continues.
If ``flush()`` times out, retry or stop according to a separate flush policy.
Do not repeat an already successful write after a flush timeout, because the complete request is already queued.
Keep separate attempt or time limits for write acceptance and flushing so each phase can report the correct failure.

.. rubric:: Demo

The following demo retries one unchanged forest-observation record until the stream accepts it.
It then enters a separate flush loop, so a flush timeout can never duplicate the accepted record.

.. erbsland-demo::
    :source: stream/ReadinessAndTimeouts/RetryAtomicWrite.cpp
    :exec: stream/readiness_and_timeouts --demo RetryAtomicWrite
    :source-sha256: 1347bb625a15f8b369ce3f1c22158b54f7cdbe93d180298ba6a3d482c934500b

.. code-block:: cpp

    /// Retry one complete output request without changing it.
    /// A timed-out write accepted none of the record, so repeating the same call cannot duplicate a partial record.
    /// Limit retries, handle stream failures separately, and flush accepted output when native delivery matters.
    auto writeObservationRecord(el::ByteOutputStream &output, const el::ByteBlock &record) -> std::size_t {
        constexpr auto cMaximumAttempts = 3U;

        try {
            for (auto attempt = 1U; attempt <= cMaximumAttempts; ++attempt) {
                if (output.write(record).isTimeout()) {
                    continue;
                }
                for (auto flushAttempt = 0U; flushAttempt < cMaximumAttempts; ++flushAttempt) {
                    if (output.flush().isSuccess()) {
                        return attempt;
                    }
                }
                throw el::RuntimeError{"The record was accepted but was not sent within the expected time."_el};
            }
        } catch (const el::StreamError &) {
            throw el::RuntimeError{"The forest observation could not be recorded."_el, std::current_exception()};
        }
        throw el::RuntimeError{"The recorder did not accept the record within the expected time."_el};
    }

.. erbsland-ansi::
    :escape-char: ␛

    Attempts: 2

.. erbsland-demo-end::

For shutdown and ownership decisions after a workflow stops, continue with :doc:`lifecycle`.
