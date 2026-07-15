..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Streams; Buffering
    single: Streams; Back Pressure
    single: Streams; Flush

***************************
Buffering and Back Pressure
***************************

Streams connect application code to files, terminals, pipes, and other native endpoints.
The two sides rarely work at the same pace: an endpoint may deliver a line in several small reads, while an application
may create output much faster than a destination can accept it.
Without buffering, every caller would have to assemble partial input and coordinate each write with the native target.

The Stream API handles these differences with bounded buffers.
For input, they support read-ahead and allow complete values to survive a timeout.
For output, they preserve atomic writes while a background worker transfers accepted data.
The limits keep this convenience predictable: a stalled endpoint cannot make a stream consume memory without a bound.

This page explains how the input and output buffers work, how they simplify timeout handling, and where their limits
matter in application code.
You will also learn when to throttle an output producer and when to flush accepted data.

Understand the Buffer Limits
============================

Stream settings provide two related size controls:

``bufferCapacity``
    The fixed capacity used by the byte stream for native input or output.
    Both :cpp:class:`InputStreamSettings <erbsland::stream::InputStreamSettings>` and
    :cpp:class:`OutputStreamSettings <erbsland::stream::OutputStreamSettings>` provide this setting.

``backBufferLimit``
    The maximum size of the growing output queue.
    Only ``OutputStreamSettings`` provides this setting because output must retain complete application writes while a
    destination is busy.

Text streams use the buffers of their backing byte streams.
Text input adds decoding after the buffered byte input, while text output encodes each request before the bytes enter
the output queue.

Input Streams Read Ahead
------------------------

An input stream uses two fixed rings of ``bufferCapacity`` bytes.
Public reads consume the front ring while a background worker fills the back ring from the native source.
When the front ring is empty, the rings exchange roles and read-ahead continues.

.. mermaid::

    flowchart LR
        source["Native source"] -->|"background read"| back["Back ring<br/>fixed capacity"]
        back -->|"exchange roles"| front["Front ring<br/>fixed capacity"]
        front --> bytes["Byte input stream"]
        bytes -->|"decode"| text["Text input stream"]
        bytes --> app["Application"]
        text --> app

This arrangement lets the source and the application make progress independently for short periods.
It also reduces native calls without exposing native chunk boundaries to callers.
For text, decoding happens at complete Unicode code-point boundaries, even when one encoded character is split across
native reads.

The trade-off is fixed memory and read-ahead.
Each byte input stream can reserve two rings of the selected capacity, and the native source may be read before the
application asks for those bytes.
A very small capacity increases worker handoffs and native calls; a very large capacity multiplies memory use across all
open streams.
The input size itself may be much larger than these rings because they are reused throughout the transfer.

Output Streams Preserve Atomic Writes
-------------------------------------

An output stream reverses the direction.
Application writes enter a queue, a background worker moves queued bytes through the fixed front ring, and the native
target consumes that ring.
The back ring grows when necessary, but never beyond ``backBufferLimit``.

.. mermaid::

    flowchart LR
        app["Application write"] --> request["Atomic bytes<br/>text is encoded here"]
        request --> back["Back ring<br/>grows to backBufferLimit"]
        back --> front["Front ring<br/>fixed bufferCapacity"]
        front -->|"background write"| target["Native target"]

The bounded back ring is what makes write results simple.
The stream either accepts the complete request in order or accepts none of it.
After ``Timeout``, the caller can retry the same unchanged write instead of calculating which suffix is still missing.
Text writes receive the same guarantee after encoding.

This convenience has two limits.
First, every atomic request must fit ``backBufferLimit`` in encoded bytes.
Second, one output stream may use memory close to its fixed front capacity plus its back-buffer limit while the target
is stalled.
Large limits can therefore hide a slow destination and become expensive when many streams are open.

Choose Sizes From Records and Concurrency
-----------------------------------------

The defaults are a good starting point for ordinary files and terminals.
Choose a different ``bufferCapacity`` when measurements show that the endpoint benefits from another native transfer
size or when the fixed memory per stream must be reduced.
For output, choose ``backBufferLimit`` from the largest indivisible record the application must write and the amount of
burst traffic it may queue safely.

Account for the number of streams that may be open at once, not only for one ideal transfer.
Also budget text limits in encoded bytes rather than Unicode code points.
The settings are copied into a stream when it is opened, so configure the path options before creation and inspect the
effective policy later through ``inputSettings()`` or ``outputSettings()``.

.. rubric:: Demo

The following demo opens text input and output streams for a cell-imaging workflow.
It gives the input rings 8 KiB each, selects a 4 KiB output front ring, and allows a 32 KiB output back ring.
Using different values makes the independent purposes of the two settings visible.

.. erbsland-demo::
    :source: stream/BufferingAndBackPressure/ConfigureBuffers.cpp
    :exec: stream/buffering_and_back_pressure --demo ConfigureBuffers
    :source-sha256: bf75cdddfc5a44699802344d47274e0f679507a0b52e2f0828e4fa95da5c2118

.. code-block:: cpp

    /// Configure buffer sizes before opening text streams.
    /// Text streams use the settings of their backing byte streams. Input has two fixed rings of `bufferCapacity()`, while
    /// output combines a fixed front ring with a growing back ring limited by `backBufferLimit()`.
    void openCellImagingStreams(const el::Path &sourcePath, const el::Path &targetPath) {
        auto inputSettings = el::InputStreamSettings{};
        inputSettings.setBufferCapacity(el::ByteLength{8U * 1024U});
        auto readOptions = el::PathReadTextOptions{};
        readOptions.setStreamSettings(inputSettings);
        const auto input = sourcePath.content().openTextInputStream(readOptions);

        auto outputSettings = el::OutputStreamSettings{};
        outputSettings.setBufferCapacity(el::ByteLength{4U * 1024U})
            .setBackBufferLimit(el::ByteLength{32U * 1024U});
        auto writeOptions = el::PathWriteTextOptions{};
        writeOptions.setStreamSettings(outputSettings);
        const auto output = targetPath.content().openTextOutputStream(writeOptions);

        el::io::printLine(
            "Input buffer: "_el,
            input->inputSettings().bufferCapacity().toSizeT(),
            " bytes"_el);
        el::io::printLine(
            "Output buffer: "_el,
            output->outputSettings().bufferCapacity().toSizeT(),
            " bytes"_el);
        el::io::printLine(
            "Output back buffer: "_el,
            output->outputSettings().backBufferLimit().toSizeT(),
            " bytes"_el);

        input->close();
        output->close();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Input buffer: 8192 bytes
    Output buffer: 4096 bytes
    Output back buffer: 32768 bytes

.. erbsland-demo-end::

Read Complete Values Without Manual Assembly
============================================

Short read methods expose what is currently available.
For example, :cpp:func:`ByteInputStream::read() <erbsland::stream::ByteInputStream::read>` with a destination span may
fill only part of that span.
This is useful for block processing, but a caller that needs a complete header or record must otherwise keep a separate
buffer, track the filled length, and join data from several calls.

Higher-level read methods perform that work for you:

*   :cpp:func:`readExact() <erbsland::stream::ByteInputStream::readExact>` waits for an exact byte length.
*   :cpp:func:`readLine() <erbsland::stream::TextInputStream::readLine>` waits for a decoded line.
*   :cpp:func:`readAll() <erbsland::stream::ByteInputStream::readAll>` collects the remaining input up to a finite
    maximum.
*   Text :cpp:func:`read() <erbsland::stream::TextInputStream::read>` and
    :cpp:func:`readChar() <erbsland::stream::TextInputStream::readChar>` return decoded text only at complete code-point
    boundaries.

When ``readExact()``, ``readLine()``, or ``readAll()`` times out before its value is complete, the stream keeps the
input already collected.
Retry the same call with the same limit and it continues the operation.
Application code does not need a second buffer or special handling for half-read lines and records.
This is especially useful for pipes, terminals, and other sources where a timeout can occur in the middle of a logical
value.

Always provide a finite maximum that matches the format.
The stream can preserve incomplete input only within that application limit, and repeated timeouts still need a finite
retry or deadline policy.
Use short block reads when the input is naturally incremental; use aggregate reads when the application needs one
complete semantic value.

.. rubric:: Demo

The demo reads a UTF-8 observation file one line at a time.
``readLine()`` returns each complete decoded line, so the loop needs no byte array, string builder, or partial-line
state.
After a timeout it simply repeats the same call, while a small retry count prevents a permanently stalled source from
keeping the workflow alive forever.

.. erbsland-demo::
    :source: stream/BufferingAndBackPressure/ReadBoundedChunks.cpp
    :exec: stream/buffering_and_back_pressure --demo ReadBoundedChunks
    :source-sha256: 1daf69ebc2369c8d20c8cd0f3ddecfffb87fffafb5ee33217646558111eec6f5

.. code-block:: cpp

    /// Read a text file line by line without assembling partial input in application code.
    /// `readLine()` retains an incomplete line after a timeout. Retry the same call and the stream continues collecting the
    /// line until it can return it as one value.
    void readCellObservationLog(el::TextInputStream &input) {
        constexpr auto cMaximumLineLength = el::CpLength{80U};
        constexpr auto cMaximumTimeouts = 3U;
        auto consecutiveTimeouts = 0U;

        while (true) {
            const auto result = input.readLine(cMaximumLineLength);
            if (result.hasData()) {
                consecutiveTimeouts = 0U;
                el::io::print(result.data());
                continue;
            }
            if (result.isFinished()) {
                return;
            }
            if (++consecutiveTimeouts == cMaximumTimeouts) {
                throw el::RuntimeError{"Reading the observations timed out repeatedly."_el};
            }
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Δείγμα 1: πυρήνας ορατός
    Δείγμα 2: κυτταρική διαίρεση
    Δείγμα 3: μεμβράνη ακέραιη

.. erbsland-demo-end::

Throttle Fast Output Producers
==============================

Output buffering absorbs short bursts, but it cannot solve a sustained speed difference.
If a producer continuously creates records faster than the native target can write them, the back ring grows toward its
limit and later writes must wait for capacity.
That feedback is *back pressure*.

You should think about explicit throttling when a loop produces a large file, sends many messages, or performs expensive
work to prepare the next output block.
Occasional small writes need no extra readiness check because ``write()`` already waits for bounded capacity.
For a sustained producer, however, waiting before preparing the next record avoids unnecessary work and prevents the
queue from remaining full.

Use :cpp:func:`OutputStream::isReady() <erbsland::stream::OutputStream::isReady>` for an immediate snapshot.
When it returns ``false``, queued data remains in the back ring.
:cpp:func:`OutputStream::waitForReady() <erbsland::stream::OutputStream::waitForReady>` waits up to the configured
timeout for that queue to drain.
After the stream becomes ready, prepare and submit one request within ``backBufferLimit``.

Readiness does not reserve capacity.
With multiple producers, another write may fill the queue before the following call, so always inspect the write result.
With one controlled producer, readiness is a natural pacing point between records or chunks.
If readiness or the following write times out repeatedly, stop, reschedule, or report the stalled destination instead of
retrying forever.

.. rubric:: Demo

The cell-observation demo writes 25,000 log lines quickly to a text file.
Before formatting each line, it waits whenever earlier output is still queued.
Each line remains one atomic request, and both the readiness wait and the write have clear timeout handling.

.. erbsland-demo::
    :source: stream/BufferingAndBackPressure/ApplyBackPressure.cpp
    :exec: stream/buffering_and_back_pressure --demo ApplyBackPressure
    :source-sha256: f3fe3f30ed21e59c6bf2c637710a4465f66954972e1f2333c2b41f77bfffbfdf

.. code-block:: cpp

    /// Apply back pressure while producing a large text log.
    /// Wait before preparing each record when earlier output is still queued. This keeps a fast producer close to the
    /// destination's pace instead of repeatedly filling the bounded back buffer.
    auto writeCellObservationLog(el::TextOutputStream &output) -> std::size_t {
        constexpr auto cRecordCount = std::size_t{25U * 1000U};
        constexpr auto cObservation = ": η κυτταρική μεμβράνη είναι ακέραιη"_el;

        for (auto record = std::size_t{1U}; record <= cRecordCount; ++record) {
            if (!output.isReady() && output.waitForReady().isTimeout()) {
                throw el::RuntimeError{"The log output remained busy."_el};
            }
            if (output.printLine("Παρατήρηση "_el, record, cObservation).isTimeout()) {
                throw el::RuntimeError{"The log record was not accepted in time."_el};
            }
        }
        return cRecordCount;
    }

.. erbsland-ansi::
    :escape-char: ␛

    Log lines written: 25000

.. erbsland-demo-end::

Keep Atomic Writes Within the Hard Limit
========================================

Every byte or encoded text write must fit ``backBufferLimit`` as one complete request.
If it is larger, the stream throws :cpp:class:`StreamError <erbsland::stream::StreamError>` before accepting anything.
Waiting for readiness cannot help because this is a hard request limit, not temporary congestion.

Choose a limit large enough for the largest valid indivisible record.
If a payload can be divided safely, write it as several meaningful chunks and apply back pressure between them.
For text, remember that the encoded byte length may be larger than the number of Unicode code points.

.. rubric:: Demo

The demo opens a text output stream with an 8-byte front buffer and a 16-byte back-buffer limit.
It then writes a longer observation as one atomic line.
The stream rejects the request and the output shows the resulting error title and description.

.. erbsland-demo::
    :source: stream/BufferingAndBackPressure/RejectOversizedRequest.cpp
    :exec: stream/buffering_and_back_pressure --demo RejectOversizedRequest
    :source-sha256: c204efb913b3bb80fce7aa2b7a9e6fdfb1659d763e6a358f599757202022396f

.. code-block:: cpp

    /// Keep every atomic text write within the stream's back-buffer limit.
    /// The limit applies to encoded bytes. A request that exceeds it fails before any text is accepted, even when the front
    /// buffer is empty.
    void writeOversizedCellNote(const el::Path &path) {
        auto settings = el::OutputStreamSettings{};
        settings.setBufferCapacity(el::ByteLength{8U}).setBackBufferLimit(el::ByteLength{16U});
        auto options = el::PathWriteTextOptions{};
        options.setStreamSettings(settings);
        const auto output = path.content().openTextOutputStream(options);

        output->writeLine(
            "Παρατήρηση: η κυτταρική μεμβράνη παραμένει ακέραιη."_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Failed to write text to the output stream.
    The complete encoded text request exceeds the configured output buffer limit.

.. erbsland-demo-end::

Flush at Delivery Boundaries
============================

A successful write means that the stream accepted the complete request into its ordered queue.
The background worker may still be transferring it.
Call :cpp:func:`OutputStream::flush() <erbsland::stream::OutputStream::flush>` when previously accepted data must reach
the native target before the application continues.

Useful flush points include a prompt before waiting for user input, a protocol handoff, and a checkpoint in a file that
must remain open.
Avoid flushing after every small write because that removes much of the throughput benefit of buffering.
When ownership ends, use :cpp:func:`OutputStream::close() <erbsland::stream::OutputStream::close>` to drain and close
the target instead.

``flush()`` waits only for the configured operation timeout.
After ``Timeout``, retry ``flush()`` under a finite workflow policy; do not repeat writes that were already accepted, or
you will duplicate their data.
A successful flush confirms the native endpoint's flush boundary, whose durability still depends on that endpoint and
the operating system.

.. rubric:: Demo

The final demo writes a prompt to ``stdOut()``, flushes it, and only then reads a line from ``stdIn()``.
This ordering ensures that a user sees the question before the program waits for an answer.
Both flushing and reading use a finite attempt count, while the accepted prompt is written only once.

.. erbsland-demo::
    :source: stream/BufferingAndBackPressure/FlushAcceptedOutput.cpp
    :exec: stream/buffering_and_back_pressure --demo FlushAcceptedOutput
    :source-sha256: 64136be5aa35a8237737205b2a358d024901e0432ac53e4a10d2d44a220573ac

.. code-block:: cpp

    /// Flush a prompt before waiting for standard input.
    /// A successful write only places the prompt in the output queue. `flush()` waits for accepted text to reach the
    /// terminal, so the user can see the question before the program starts reading the answer.
    auto askForCellAtlas() -> el::String {
        constexpr auto cMaximumAttempts = 3U;
        constexpr auto cMaximumAnswerLength = el::CpLength{80U};
        const auto output = el::stdOut();

        if (output->write("Cell atlas name: "_el).isTimeout()) {
            throw el::RuntimeError{"The prompt was not accepted in time."_el};
        }

        for (auto attempt = 0U; attempt < cMaximumAttempts; ++attempt) {
            if (output->flush().isSuccess()) {
                const auto result = el::stdIn()->readLine(cMaximumAnswerLength);
                if (result.hasData()) {
                    return result.data();
                }
                if (result.isFinished()) {
                    throw el::RuntimeError{"No atlas name was provided."_el};
                }
            }
        }
        throw el::RuntimeError{"The interaction did not finish in time."_el};
    }

.. erbsland-ansi::
    :escape-char: ␛

    Cell atlas name: Άτλας βρύων

.. erbsland-demo-end::
