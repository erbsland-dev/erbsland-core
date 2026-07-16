..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Streams; Lifecycle
    single: Streams; Close
    single: Streams; Abort
    single: Streams; Failed State

****************
Stream Lifecycle
****************

When you finish reading from or writing to a stream, you decide how its work ends.
Call ``close()`` on an owned stream to finish normally.
For output, this delivers data that the stream has already accepted before it closes its target.
Call ``abort()`` when the work has been cancelled or its output must be abandoned instead.

This page explains who makes that decision, how to close output without waiting forever, and what to do after a native
failure.
You will learn how to give every stream an explicit success path and a bounded cleanup path.

Decide Who Finishes a Stream
============================

:cpp:class:`InputStream <erbsland::stream::InputStream>` and
:cpp:class:`OutputStream <erbsland::stream::OutputStream>` define the lifecycle API inherited by byte and text streams.
An input stream releases its source when it closes.
An output stream may have accepted data that still needs to reach its target, so it also provides
:cpp:func:`flush() <erbsland::stream::OutputStream::flush>` and can spend time in the ``Closing`` state.

The following diagram shows the common lifecycle API.
It leaves out read and write methods so you can focus on the choices available once a stream is in use.

.. mermaid::

    classDiagram
        class InputStream {
            +state() StreamState
            +isOpen() bool
            +close() StreamCloseStatus
            +abort()
        }
        class OutputStream {
            +state() StreamState
            +isOpen() bool
            +flush() StreamWriteStatus
            +close() StreamCloseStatus
            +abort()
        }
        class ByteInputStream
        class TextInputStream
        class ByteOutputStream
        class TextOutputStream
        class StreamState {
            Open
            Closing
            Closed
            Failed
        }
        class StreamCloseStatus {
            Closed
            Timeout
        }
        InputStream <|-- ByteInputStream
        InputStream <|-- TextInputStream
        OutputStream <|-- ByteOutputStream
        OutputStream <|-- TextOutputStream
        InputStream --> StreamState : reports
        OutputStream --> StreamState : reports
        InputStream ..> StreamCloseStatus : close result
        OutputStream ..> StreamCloseStatus : close result

:cpp:enum:`StreamState <erbsland::stream::StreamState>` tells you which part of a stream's lifecycle is active:

.. list-table:: Lifecycle states
    :header-rows: 1
    :widths: 18 34 48

    *   - State
        - What it means
        - What to do
    *   - ``Open``
        - The stream accepts normal read or write operations.
        - Perform bounded operations and inspect every result.
    *   - ``Closing``
        - An output close is still delivering previously accepted data.
        - Do not write more data. Call ``close()`` again only when your application can wait another bounded interval.
    *   - ``Closed``
        - The stream has finished its lifecycle and accepts no further data operations.
        - Release it, or keep it only to inspect its final state.
    *   - ``Failed``
        - A native operation failed and the stream retained that failure.
        - Report the diagnostic and discard every alias to this stream.

:cpp:func:`InputStream::isOpen() <erbsland::stream::InputStream::isOpen>` and
:cpp:func:`OutputStream::isOpen() <erbsland::stream::OutputStream::isOpen>` test for exactly ``Open``.
They are useful for assertions and observations, but they do not reserve the stream against a concurrent close or
failure.
The result or exception from the operation you perform remains authoritative.

Only the owner should close or abort a stream.
A function that receives a :cpp:type:`ByteOutputStreamPtr <erbsland::stream::ByteOutputStreamPtr>` or another stream
pointer usually borrows it: it may read or write, but leaves the lifecycle decision to its caller.
Passing the shared pointer by ``const`` reference keeps the stream alive during that operation without suggesting a
transfer of ownership.
Shared ownership keeps the object alive; it does not decide when all users have finished with it.

When Writing Is Finished, Close the Owned Output
================================================

After the final successful write, call :cpp:func:`OutputStream::close() <erbsland::stream::OutputStream::close>` on the
output stream you own.
This is the normal successful end of writing: the stream stops accepting new output, delivers the data it already
accepted, and then closes its target.
When ``close()`` returns :cpp:member:`Closed <erbsland::stream::StreamCloseStatus::Closed>`, you know that this work is
complete.

You do not need to call ``flush()`` before a normal close.
Use :cpp:func:`OutputStream::flush() <erbsland::stream::OutputStream::flush>` while the stream remains open when a
receiver must observe data earlier; still call ``close()`` when writing is finished.

One ``close()`` call waits only for the timeout configured for the stream.
If a workflow has no more time after a timeout, its owner can call ``abort()`` to stop waiting and report that delivery
could not be confirmed.
If it can wait longer, continue the same close operation as described in the next section.

The example writes through a helper that borrows a shared stream pointer.
The caller owns the pointer and therefore performs the close.
It aborts only when this particular workflow has no time for another close attempt; a native close failure is reported
to the caller instead.

.. rubric:: Demo: Close a completed score

.. erbsland-demo::
    :source: stream/Lifecycle/CloseGracefully.cpp
    :exec: stream/lifecycle --demo CloseGracefully
    :source-sha256: da2ef6d9c12a0b3343010c70972de9f28c90f0079dc6302951d9d8fbc389f17c

.. code-block:: cpp

    /// Write one orchestra score into a stream that remains owned by the caller.
    /// The borrowed stream pointer keeps the stream alive while this operation runs, but this function does not close or
    /// abort it. The owner decides when all writing is finished and how to handle its close result.
    void writeOrchestraScore(const el::ByteOutputStreamPtr &output) {
        output->writeUInt16(72U);
    }

    /// Finish owned output explicitly so every accepted byte is delivered before the stream is released.
    /// A close call is bounded by the stream timeout. If this workflow cannot wait beyond the first deadline, abort the
    /// still-closing stream and report incomplete delivery instead of silently relying on destruction.
    void closeGracefully() {
        const el::ByteOutputStreamPtr output = std::make_shared<ScriptedByteOutputStream>();
        writeOrchestraScore(output);

        try {
            if (output->close().isTimeout()) {
                output->abort();
                throw el::RuntimeError{"The orchestra score could not be saved within the close timeout."_el};
            }
            el::io::printLine("Orchestra score closed safely."_el);
        } catch (const el::StreamError &) {
            throw el::RuntimeError{"The orchestra score could not be closed."_el, std::current_exception()};
        }

        el::io::printLine("Final state Closed: "_el, output->state() == el::StreamState::Closed);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Orchestra score closed safely.
    Final state Closed: true

.. erbsland-demo-end::

Continue a Slow Close Within Your Application Deadline
======================================================

A :cpp:member:`Timeout <erbsland::stream::StreamCloseStatus::Timeout>` result means that this one bounded wait ended
before graceful close completed.
The stream stays ``Closing`` and continues the close operation.
It does not become open again, and the data it has already accepted must not be written again.

Call ``close()`` again when your application can spend another configured timeout waiting for the same close.
The next call can return ``Closed``, another ``Timeout``, or a :cpp:class:`StreamError <erbsland::stream::StreamError>`
when the target has failed.
Do not repeat the preceding write or call ``flush()`` as a workaround: doing so could duplicate an application record.

A per-call timeout does not by itself bound the whole workflow.
Set an application deadline, maximum attempt count, cancellation token, or scheduler policy around repeated calls.
Between attempts, your application can update progress, react to cancellation, or let other work run.
If that wider budget expires, the owner can call ``abort()`` and report that delivery was not confirmed.

The example's first close attempt times out and its second attempt finishes.
The write helper does not change the stream lifecycle.
The owning caller keeps the close alive for up to three attempts, then aborts only if its own limit is exhausted.

.. rubric:: Demo: Continue a close after a timeout

.. erbsland-demo::
    :source: stream/Lifecycle/ContinueCloseAfterTimeout.cpp
    :exec: stream/lifecycle --demo ContinueCloseAfterTimeout
    :source-sha256: 5f18a0dc21b2523bb2192e8db4962b36c141fe79a338e5d1fc52fca86d24570c

.. code-block:: cpp

    /// Write one orchestra score into a stream that remains owned by the caller.
    /// This helper only writes data. The caller keeps responsibility for closing or aborting the shared stream.
    void writeSlowOrchestraScore(const el::ByteOutputStreamPtr &output) {
        output->writeUInt8(88U);
    }

    /// Continue one graceful close across bounded calls when accepted output must be preserved.
    /// `Timeout` leaves the stream in `Closing`; repeat `close()` without writing again. An attempt limit bounds the
    /// workflow, and `abort()` provides a non-blocking exit when the destination never completes.
    void continueCloseAfterTimeout() {
        constexpr auto cMaximumCloseAttempts = 3U;
        const el::ByteOutputStreamPtr output = std::make_shared<ScriptedByteOutputStream>(0U, 0U, 1U);
        writeSlowOrchestraScore(output);

        try {
            for (auto attempt = 1U; attempt <= cMaximumCloseAttempts; ++attempt) {
                if (output->close().isClosed()) {
                    el::io::printLine("Score closed after "_el, attempt, " attempts."_el);
                    return;
                }
                el::io::printLine("Close is still in progress; check cancellation and progress."_el);
            }
        } catch (const el::StreamError &) {
            throw el::RuntimeError{"The orchestra score could not be closed."_el, std::current_exception()};
        }

        output->abort();
        throw el::RuntimeError{"The orchestra score remained in closing for too long."_el};
    }

.. erbsland-ansi::
    :escape-char: ␛

    Close is still in progress; check cancellation and progress.
    Score closed after 2 attempts.

.. erbsland-demo-end::

Abort Work That Must End Now
============================

Call :cpp:func:`OutputStream::abort() <erbsland::stream::OutputStream::abort>` or
:cpp:func:`InputStream::abort() <erbsland::stream::InputStream::abort>` when the work has been cancelled or its result
must not be published.
``abort()`` is ``noexcept`` and does not wait for native completion.
It closes the stream immediately.
For output, data still queued for delivery may be lost; for input, pending work and unread buffered data are discarded.

This makes ``abort()`` the right choice when an incomplete result must never be delivered, or when a graceful-close
workflow has reached its wider deadline.
It is not a normal replacement for ``close()`` after successful writing, because it cannot confirm which accepted output
reached the target.

The example deliberately rejects an incomplete recording.
Its helper only writes through the borrowed shared pointer.
The caller owns the stream and explicitly aborts it, which makes the cancellation decision visible beside the data it
discards.

.. rubric:: Demo: Discard incomplete output

.. erbsland-demo::
    :source: stream/Lifecycle/AbortPendingOutput.cpp
    :exec: stream/lifecycle --demo AbortPendingOutput
    :source-sha256: 6bbf3f3aaf656e2f587c97f6b408b3249b11f52f7e3eb4a755c471c958a0258d

.. code-block:: cpp

    /// Write an incomplete orchestra recording into a stream that remains owned by the caller.
    /// The helper borrows the shared stream and therefore leaves its lifecycle unchanged for the caller to decide.
    void writeIncompleteOrchestraRecording(const el::ByteOutputStreamPtr &output) {
        output->writeUInt32(0x4f524348U);
    }

    /// Abandon pending output explicitly when cancellation matters more than delivery.
    /// `abort()` is non-throwing and returns immediately. Queued data may be lost, so use it only when the incomplete
    /// orchestra recording must not be published.
    void abortPendingOutput() {
        const auto scriptedOutput = std::make_shared<ScriptedByteOutputStream>();
        const el::ByteOutputStreamPtr output = scriptedOutput;
        writeIncompleteOrchestraRecording(output);

        el::io::printLine("Bytes before abort: "_el, scriptedOutput->bytes().size());
        output->abort();
        el::io::printLine("Stream closed: "_el, output->state() == el::StreamState::Closed);
        el::io::printLine("Bytes after abort: "_el, scriptedOutput->bytes().size());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Bytes before abort: 4
    Stream closed: true
    Bytes after abort: 0

.. erbsland-demo-end::

Discard a Stream After a Native Failure
=======================================

A timeout is expected bounded flow control and leaves a stream usable.
A native read, write, flush, close, or positioning failure is different: the affected operation throws
:cpp:class:`StreamError <erbsland::stream::StreamError>` and the stream enters ``Failed``.
Later applicable operations surface the stored failure rather than attempting ambiguous work on the broken endpoint.

Catch the exception at a boundary that can add useful application context, and report its diagnostic there.
:doc:`stream_errors` explains how to present a stream error's title, description, path, help, and platform details.
Then discard all pointers to the failed stream.
You may call ``abort()`` when your error path needs immediate non-throwing teardown, but it is neither required nor a
recovery step; destruction already provides bounded final cleanup.

Opening a replacement is an application decision.
For input, you must know where reading can safely resume.
For output, you must know whether replaying accepted work could duplicate or corrupt the destination.
Create a new stream only when the surrounding format, transaction, or protocol defines a safe restart point.

The example makes its next write fail, reports the diagnostic, and observes the ``Failed`` state.
It neither retries the write nor calls ``abort()``: after handling the error, it simply discards the failed stream.

.. rubric:: Demo: Report a failed stream

.. erbsland-demo::
    :source: stream/Lifecycle/ObserveFailedState.cpp
    :exec: stream/lifecycle --demo ObserveFailedState
    :source-sha256: ef962be836d11e711cf9bcf7d9015fc50315de341320765ceb47a8e57859d471

.. code-block:: cpp

    /// Write an orchestra cue to a stream that remains owned by the caller.
    /// A borrowed stream pointer keeps the shared stream alive for this operation, but the caller retains its lifecycle.
    void writeOrchestraCue(const el::ByteOutputStreamPtr &output) {
        output->writeUInt8(64U);
    }

    /// Stop using a stream after a native failure and report its diagnostic at the application boundary.
    /// `StreamError` is different from a routine timeout: the stream enters `Failed`, and later operations surface the
    /// stored failure. Discard this instance and create a new stream only when the application has a recovery strategy.
    void observeFailedState() {
        const auto scriptedOutput = std::make_shared<ScriptedByteOutputStream>();
        const el::ByteOutputStreamPtr output = scriptedOutput;
        scriptedOutput->setFailure(true);

        try {
            writeOrchestraCue(output);
        } catch (const el::StreamError &error) {
            el::io::printLine("Error: "_el, error.title());
            el::io::printLine("State Failed: "_el, output->state() == el::StreamState::Failed);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Error: Failed to write the scripted stream.

.. erbsland-demo-end::
