..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Streams; Text Input
    single: Streams; Text Output
    single: Streams; Encoding
    single: Streams; Lines

********************************
Text Input, Output, and Encoding
********************************

Operating systems and file formats store text as bytes, while application code should work with Unicode characters.
Text streams provide the boundary between these two views.
They keep encoding details, incomplete byte sequences, and byte-order marks out of parsers and report writers, so those
components can concentrate on the meaning of the text.

This boundary also preserves the safety rules of the stream API.
Input is returned only at decoded code-point boundaries, allocating operations have explicit limits, and every read or
write has a bounded wait.
Timeout remains an ordinary result that you can retry or turn into an application-level cancellation; malformed text and
failed I/O remain distinct errors.

This page explains how to choose a text operation, process blocks and lines without losing data, produce atomic output,
capture output in memory, and select an encoding and recovery policy.
The first step is to understand how the text interfaces fit together.

Choose the Text Operation That Matches the Task
===============================================

:cpp:class:`TextInputStream <erbsland::stream::TextInputStream>` adds Unicode decoding to the common
:cpp:class:`InputStream <erbsland::stream::InputStream>` contract.
:cpp:class:`TextOutputStream <erbsland::stream::TextOutputStream>` performs the opposite transformation: callers write
characters and strings, and the stream encodes complete requests for its byte destination.
:cpp:class:`AnyStringBuilderStream <erbsland::stream::AnyStringBuilderStream>` implements the output interface without a byte
destination and collects the written text in memory.

.. mermaid::

    classDiagram
        class InputStream {
            +isReady() bool
            +waitForReady() StreamWaitStatus
            +close() StreamCloseStatus
        }
        class TextInputStream {
            +encoding() StringEncoding
            +effectiveEncoding() StringEncoding
            +readChar() StreamReadResult
            +read(maximum) StreamReadResult
            +readLine(maximum) StreamReadResult
            +readAll(maximum) StreamReadResult
        }
        class OutputStream {
            +isReady() bool
            +waitForReady() StreamWaitStatus
            +flush() StreamWriteStatus
            +close() StreamCloseStatus
        }
        class TextOutputStream {
            +encoding() StringEncoding
            +effectiveEncoding() StringEncoding
            +write(text) StreamWriteStatus
            +writeLine(text) StreamWriteStatus
            +print(values) StreamWriteStatus
            +printLine(values) StreamWriteStatus
        }
        class AnyStringBuilderStream {
            +toString() String
            +takeString() String
            +clear()
        }
        InputStream <|-- TextInputStream
        OutputStream <|-- TextOutputStream
        TextOutputStream <|-- AnyStringBuilderStream

Choose the input operation from the structure you expect, not merely from what is convenient at the call site:

``readChar()``
    Reads one decoded code point.
    Use it for tokenizers and parsers in which individual characters have meaning.

``read(maximum)``
    Returns one available block with at most the requested number of code points.
    Use it for incremental transforms, search, display, and other work that does not require line boundaries.

``readLine(maximum)``
    Collects one line, a maximum-length line fragment, or the final unterminated line.
    Use it for line-oriented formats while keeping a strict memory bound.

``readAll(maximum)``
    Collects text until normal end or the limit is reached.
    Reserve it for sources that are expected and required to remain small.

All four operations return a :cpp:type:`StreamReadResult <erbsland::stream::StreamReadResult>`.
``Data`` makes the payload available, ``Finished`` reports normal end without another payload, and ``Timeout`` returns
control because no complete result became available before the configured deadline.
As with :doc:`byte_input_and_output`, only access ``data()`` after ``hasData()`` is true.
Failures of the stream or backing source throw :cpp:class:`StreamError <erbsland::stream::StreamError>` instead of
masquerading as a normal result.

The distinction is deliberate.
A timeout often means “try again later.” Normal end means the document is complete.
A stream error means the source can no longer satisfy the operation.
Keeping these outcomes separate lets a caller choose an appropriate retry, completion, or diagnostic path.

Process Unicode Without Splitting Encoded Characters
====================================================

:cpp:func:`readChar() <erbsland::stream::TextInputStream::readChar>` returns one Unicode code point, while
:cpp:func:`read() <erbsland::stream::TextInputStream::read>` returns up to a finite number of code points.
The decoder may need several source bytes to produce one code point, so it retains an incomplete UTF-8, UTF-16, or
UTF-32 sequence internally until it can return a complete character.
Your processing loop never has to join partial encoded sequences.

The maximum is a :cpp:type:`CpLength <erbsland::unit::CpLength>` rather than a byte count because the caller is already
on the Unicode side of the boundary.
A block may contain fewer code points than requested when that is all the source can currently provide.
It never ends in the middle of one encoded code point, although it can still end between user-perceived grapheme
clusters made from several code points.

Use a modest block size when processing a large or open-ended source.
This bounds temporary memory and lets the application respond between operations.
Use :cpp:func:`readAll() <erbsland::stream::TextInputStream::readAll>` only after choosing a limit that is safe for the
data format; its no-argument overload still applies a library default, but an application-specific limit communicates
the expected document size more clearly.

An aggregate ``readAll()`` retains text collected before a timeout.
Repeat the same operation with the same maximum to continue it.
Plain ``read()`` is different: every ``Data`` result is already a complete block for the caller to process immediately.
In either case, combine the per-operation timeout configured when the stream was opened with an application-level
attempt or elapsed-time limit so a permanently stalled source cannot keep the workflow alive forever.

The following demo receives a generic text input stream; its hidden setup only creates the sample file and configures a
one-second operation timeout.
The visible function asks for at most eight code points, processes every available block, and resets its consecutive
timeout count whenever progress is made.
After three consecutive timeouts it stops the workflow, and a failed source is wrapped with the context that plant notes
were being read.

.. erbsland-demo::
    :source: stream/TextInputAndOutput/ReadUnicodeBlocks.cpp
    :exec: stream/text_input_and_output --demo ReadUnicodeBlocks
    :source-sha256: 596750615a03ba938fa6c3bc7a075f4f6b4cd41423ab173bd58237226a15f6e2

.. code-block:: cpp

    /// Read decoded text in bounded, code-point-safe blocks.
    /// Handle normal end, timeout, and stream failure separately; a finite retry limit prevents a stalled source from
    /// keeping the application in this loop forever.
    void readUnicodeBlocks(el::TextInputStream &input) {
        constexpr auto cBlockLength = el::CpLength{8U};
        constexpr auto cMaximumTimeouts = 3U;
        auto consecutiveTimeouts = 0U;

        try {
            while (true) {
                const auto result = input.read(cBlockLength);
                if (result.isFinished()) {
                    return;
                }
                if (result.isTimeout()) {
                    if (++consecutiveTimeouts == cMaximumTimeouts) {
                        throw el::RuntimeError{"Too many timeouts occurred while reading the plant notes."_el};
                    }
                    continue;
                }

                consecutiveTimeouts = 0U;
                el::io::printLine("Block: ["_el, result.data(), "]"_el);
            }
        } catch (const el::StreamError &) {
            throw el::RuntimeError{"The plant notes could not be read."_el, std::current_exception()};
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Block: [Frö 🌱 – ]
    Block: [två blad]
    Block: [ – höjd ]
    Block: [7 cm]

.. erbsland-demo-end::

Read Lines Without Losing Their Structure
=========================================

:cpp:func:`readLine() <erbsland::stream::TextInputStream::readLine>` treats a decoded line-feed character as the end of
a line and includes that character in the returned text.
For CRLF input, the preceding carriage return is preserved as part of the line as well.
Keeping the delimiter lets a formatter, proxy, or editor reproduce the original line structure instead of guessing which
returned lines were terminated.

The final line of a file does not need a delimiter.
If text remains when the source ends, ``readLine()`` first returns that unterminated text as ``Data``; only the next
call returns ``Finished``.
An empty ``Finished`` result therefore means there is no more line content, not that the preceding line should be
discarded.

Always pass a finite maximum based on the format you accept.
If no line feed appears within that maximum, the function returns a maximum-length fragment as ``Data``.
The caller can process fragments as a streaming line, reject the record as too long, or accumulate it under a separate
application limit.
This design prevents a malformed or hostile input containing one endless line from forcing unbounded allocation.

If a timeout occurs while a line is incomplete, the stream retains the decoded prefix.
Repeat ``readLine()`` with the same maximum to continue that logical line; do not request only the missing suffix.
Selecting another read operation or changing the maximum cancels the aggregate line operation, but the retained text is
replayed to the newly selected operation in order.

The demo reads a small sequence of plant measurements.
It prints the line ending already present in each complete line and adds one only for the final unterminated line.
The three-attempt policy bounds a stalled input, while ``Finished`` and ``StreamError`` take separate completion and
failure paths.

.. erbsland-demo::
    :source: stream/TextInputAndOutput/ReadObservationLines.cpp
    :exec: stream/text_input_and_output --demo ReadObservationLines
    :source-sha256: 4c05e31facc02cbd6d69f83b3ca763062e1e3aeee44c1fc51d5f488c6b020f14

.. code-block:: cpp

    /// Read lines with a finite length and a finite retry policy.
    /// `readLine()` keeps a line ending when one is present and returns a final unterminated line as data.
    /// Repeating the same call after timeout continues the pending logical line.
    void readObservationLines(el::TextInputStream &input) {
        constexpr auto cMaximumLineLength = el::CpLength{40U};
        constexpr auto cMaximumTimeouts = 3U;

        auto lineNumber = 1;
        auto consecutiveTimeouts = 0U;
        try {
            while (true) {
                const auto result = input.readLine(cMaximumLineLength);
                if (result.isFinished()) {
                    return;
                }
                if (result.isTimeout()) {
                    if (++consecutiveTimeouts == cMaximumTimeouts) {
                        throw el::RuntimeError{"Too many timeouts occurred while reading the measurement series."_el};
                    }
                    continue;
                }

                consecutiveTimeouts = 0U;
                el::io::print("Line "_el, lineNumber++, ": "_el, result.data());
                if (!result.data().endsWith("\n"_el)) {
                    el::io::writeLine();
                }
            }
        } catch (const el::StreamError &) {
            throw el::RuntimeError{"The measurement series could not be read."_el, std::current_exception()};
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Line 1: Dag 1: 2 cm
    Line 2: Dag 8: 7 cm
    Line 3: Dag 15: 13 cm

.. erbsland-demo-end::

Produce Text as Complete Atomic Requests
========================================

Use :cpp:func:`write() <erbsland::stream::TextOutputStream::write>` when a character or string is already available.
Use :cpp:func:`writeLine() <erbsland::stream::TextOutputStream::writeLine>` to append one line-feed character to an
existing string as part of the same request.
These low-level methods avoid building another formatting context and are the right choice for large, already-built
text.

:cpp:func:`print() <erbsland::stream::TextOutputStream::print>` and
:cpp:func:`printLine() <erbsland::stream::TextOutputStream::printLine>` combine strings, characters, numbers, booleans,
and formatting objects concisely.
The stream first builds the formatted text and then commits it as one request, which is useful for a small record that
must not be interleaved with output from another caller.
For large generated content, prefer several bounded ``write()`` calls so the temporary formatted value does not grow
without a limit.

Every public text write is atomic with respect to timeout.
``Success`` means the complete encoded request was accepted; ``Timeout`` means none of that request was accepted.
After timeout, retry the same unchanged call.
Do not resume at an estimated character or byte offset because there is no partial caller-visible acceptance to account
for.

Atomicity does not make an unlimited retry loop safe.
Give each stream a suitable operation timeout, then stop, reschedule, or cancel after a finite application-level budget.
A broken destination throws ``StreamError`` and should normally be caught where you can add useful context.
The component that opened an output stream also remains responsible for a successful
:cpp:func:`close() <erbsland::stream::OutputStream::close>`; a helper that merely receives ``TextOutputStream &`` should
not close a destination it does not own.

The report demo hides its in-memory destination and documents only a destination-independent writer.
Its small retry helper repeats each whole operation at most three times.
It uses ``write...`` for text that is already built, ``printLine()`` for the mixed text and numeric record, and wraps
native stream failure with report-specific context.

.. erbsland-demo::
    :source: stream/TextInputAndOutput/WriteGrowthReport.cpp
    :exec: stream/text_input_and_output --demo WriteGrowthReport
    :source-sha256: 81c9153bb7bf25f0902016178a8591b66ac7d13264d9e63e63f459da38c3db1e

.. code-block:: cpp

    /// Write a report with a bounded retry policy for every atomic text operation.
    /// Retry the complete unchanged call after timeout because the stream accepted none of it.
    /// Convert a stream failure into an application error where useful report context is available.
    void writeGrowthReport(el::TextOutputStream &output) {
        constexpr auto cMaximumAttempts = 3U;
        const auto writeWithRetry = [&](const auto &operation) -> void {
            for (auto attempt = 0U; attempt < cMaximumAttempts; ++attempt) {
                if (operation().isSuccess()) {
                    return;
                }
            }
            throw el::RuntimeError{"The growth report timed out."_el};
        };

        try {
            writeWithRetry([&]() { return output.writeLine("Tillväxtrapport"_el); });
            writeWithRetry([&]() { return output.write("Mätserie: "_el); });
            writeWithRetry([&]() { return output.writeLine("björkplantor"_el); });
            writeWithRetry([&]() { return output.printLine("Dag "_el, 21, ": "_el, 18, " cm 🌿"_el); });
        } catch (const el::StreamError &) {
            throw el::RuntimeError{"The growth report could not be written."_el, std::current_exception()};
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Tillväxtrapport
    Mätserie: björkplantor
    Dag 21: 18 cm 🌿

.. erbsland-demo-end::

Capture Reusable Text Output in Memory
======================================

``AnyStringBuilderStream`` is useful when an existing component writes to ``TextOutputStream`` but the caller needs the
result as a string.
Typical uses include report generation, templates, unit tests, and scoped capture of standard output.
The producer stays independent of whether its eventual destination is a file, terminal, redirected stream, or memory.

Unlike a file or pipe, the builder performs no external I/O.
It is always ready, and its write operations do not time out.
That makes it unnecessary to add a retry loop merely for the builder, but a reusable producer should still return or
propagate the write status so another destination can apply its own policy.

Use :cpp:func:`AnyStringBuilderStream::toString() <erbsland::stream::AnyStringBuilderStream::toString>` when the builder
must retain its content and the caller needs a copy.
Use :cpp:func:`AnyStringBuilderStream::takeString() <erbsland::stream::AnyStringBuilderStream::takeString>` when output
is complete and ownership should move out efficiently.
Taking the string resets the builder, so it can be reused for another capture without retaining the previous content.

The demo separates production from capture.
``writePlantSummary()`` emits the whole two-line summary as one atomic request and returns its status.
``buildPlantSummary()`` supplies a ``AnyStringBuilderStream``, verifies the invariant that this local destination cannot
time out, and moves the completed string to its caller.

.. erbsland-demo::
    :source: stream/TextInputAndOutput/CaptureReport.cpp
    :exec: stream/text_input_and_output --demo CaptureReport
    :source-sha256: cbffeaa051fcc593c8e261d354736d9608ca3580eacd5df036f9135c8ef0f098

.. code-block:: cpp

    /// Produce a complete summary as one atomic text-stream request.
    /// Returning the status lets a file or terminal caller apply its own retry policy without coupling this producer to
    /// a particular destination.
    auto writePlantSummary(el::TextOutputStream &output) -> el::StreamWriteStatus {
        return output.print("Art: fjällsippa\nNya blad: "_el, 5, "\n"_el);
    }

    /// Capture text-stream output in memory and move the completed string out of the builder.
    /// `AnyStringBuilderStream` is always ready and performs no external I/O, so this operation needs no timeout retry.
    /// The same stream-oriented writer can also target a file or terminal.
    auto buildPlantSummary() -> el::String {
        const auto builder = el::AnyStringBuilderStream::create();
        if (writePlantSummary(*builder).isTimeout()) {
            throw el::LogicError{"An in-memory string builder unexpectedly timed out."};
        }

        return builder->takeString();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Captured report:

.. erbsland-demo-end::

Match the Encoding and BOM Policy to the Format
===============================================

Encoding is a property of the boundary, not of the application text.
Choose it when the stream is opened through
:cpp:class:`PathReadTextOptions <erbsland::path::PathReadTextOptions>` or
:cpp:class:`PathWriteTextOptions <erbsland::path::PathWriteTextOptions>`.
After opening, readers and writers continue to use the same Erbsland string and character types regardless of the byte
representation below them.

:cpp:class:`StringEncoding <erbsland::text::StringEncoding>` supports UTF-8 plus UTF-16 and UTF-32 in generic or explicit
byte order.
Use UTF-8 for ordinary new formats unless an external specification requires something else.
Use generic ``Utf16`` or ``Utf32`` when a BOM may determine the byte order at the start of a document.
Use an explicit little- or big-endian variant when the format defines that order, or when positioning must begin away
from byte zero and no BOM can be inspected there.

:cpp:enum:`StringBomMode <erbsland::text::StringBomMode>` makes the format contract explicit:

``Automatic``
    Accepts a leading BOM on input.
    Generic UTF-16 and UTF-32 use its byte order and otherwise fall back to little-endian; output adds a BOM for every
    UTF-16 and UTF-32 variant but not for UTF-8.

``Require``
    Rejects input without a BOM and always writes one on output.
    Use it when the surrounding format relies on a signature and byte-order declaration.

``Reject``
    Rejects a BOM on input and never writes one on output.
    Use it when a protocol or embedded text field specifies the encoding out of band.

These policies apply only at the initial byte boundary.
Readers consume an accepted signature before producing text, and writers emit it through their dedicated BOM path.
A repeated or embedded encoded ``U+FEFF`` is invalid content and follows the stream's encoding-error mode; no public
string, reader, or iterator exposes an internal BOM signal.

:cpp:func:`encoding() <erbsland::stream::TextOutputStream::encoding>` reports the configured value.
:cpp:func:`effectiveEncoding() <erbsland::stream::TextOutputStream::effectiveEncoding>` reports the concrete encoding
after the byte order has been resolved.
For example, an output configured as generic ``Utf16`` reports ``Utf16LittleEndian`` as its effective encoding.
On input, code that needs the effective order should query it after enough leading data has been consumed to resolve a
possible BOM.

The demo opens a UTF-16 file with a required BOM and a one-second operation timeout.
It retries the complete line at most three times, then checks both configured and effective encodings.
Finally, it closes the owned file explicitly; a close timeout aborts the sample rather than silently abandoning queued
output, and a native stream failure is reported with encoding-test context.

.. erbsland-demo::
    :source: stream/TextInputAndOutput/ConfigureEncoding.cpp
    :exec: stream/text_input_and_output --demo ConfigureEncoding
    :source-sha256: 3c7c8a6dccf588d21470717c4e41b310dfd5347a5884ff17c60282c7ada7f1aa

.. code-block:: cpp

    /// Open a bounded UTF-16 text stream and require a byte-order mark.
    /// Retry the unchanged atomic write after timeout, verify the resolved byte order, and close the stream explicitly.
    void writeEncodedMeasurement(const el::Path &path) {
        constexpr auto cMaximumAttempts = 3U;
        auto options = el::PathWriteTextOptions{el::StringEncoding::Utf16};
        options.setBomMode(el::StringBomMode::Require);
        options.setTimeout(el::TimeDelta::seconds(1));

        try {
            const auto output = path.content().openTextOutputStream(options);
            for (auto attempt = 0U; attempt < cMaximumAttempts; ++attempt) {
                if (output->writeLine("Rötter: 12 cm"_el).isSuccess()) {
                    el::io::printLine("Configured UTF-16: "_el, output->encoding() == el::StringEncoding::Utf16);
                    el::io::printLine("Effective little-endian: "_el,
                        output->effectiveEncoding() == el::StringEncoding::Utf16LittleEndian);
                    if (output->close().isTimeout()) {
                        output->abort();
                        throw el::RuntimeError{"Closing the encoding sample timed out."_el};
                    }
                    return;
                }
            }
            output->abort();
            throw el::RuntimeError{"The encoding sample timed out."_el};
        } catch (const el::PathError &) {
            throw el::RuntimeError{"The encoding sample could not be opened."_el, std::current_exception()};
        } catch (const el::StreamError &) {
            throw el::RuntimeError{"The encoding sample could not be written."_el, std::current_exception()};
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Configured UTF-16: true
    Effective little-endian: true

.. erbsland-demo-end::

Choose Deliberately How Malformed Text Is Handled
=================================================

Malformed encoded input is different from failed I/O.
The source may be readable and complete while its bytes are not valid for the selected encoding.
:cpp:enum:`EncodingErrorMode <erbsland::text::EncodingErrorMode>` lets the boundary decide whether an application may
continue with a lossy interpretation:

``Replace``
    Inserts the Unicode replacement character for malformed input.
    This is a useful default for display, diagnostics, and imported prose where preserving progress matters most.

``Ignore``
    Skips malformed input.
    Use it only when omission is an explicit part of the data policy because the resulting text no longer identifies
    where information was lost.

``Throw``
    Raises :cpp:class:`EncodingError <erbsland::text::EncodingError>` at the first malformed sequence.
    Use it for configuration, source code, identifiers, signed text, and other formats where replacement could change
    meaning or conceal corruption.

Set the mode before opening the stream.
Choosing ``Throw`` does not turn a timeout into an exception: bounded flow control still returns ``Timeout``.
Likewise, an inaccessible or failed source still throws ``StreamError``.
Catch ``EncodingError`` when you can reject or report invalid content, and catch ``StreamError`` where you can explain
which source operation failed.

The final demo creates malformed UTF-8 in hidden setup and passes only its path to the documented function.
The visible code selects strict decoding, limits both the aggregate read size and the wait for each attempt, and repeats
the unchanged ``readAll()`` after timeout.
It treats the expected ``EncodingError``, a stalled source, unexpectedly accepted input, and failed I/O as four distinct
outcomes.

.. erbsland-demo::
    :source: stream/TextInputAndOutput/HandleInvalidEncoding.cpp
    :exec: stream/text_input_and_output --demo HandleInvalidEncoding
    :source-sha256: 00ea5ac72d9318051f19990b3c9cb788ce4b96766ac25709bdc90f18774e53a8

.. code-block:: cpp

    /// Reject malformed UTF-8 when silently repairing the input could change its meaning.
    /// Bound each aggregate read, repeat it after timeout, and distinguish decoding errors from failed I/O.
    void rejectInvalidEncoding(const el::Path &path) {
        constexpr auto cMaximumAttempts = 3U;
        auto options = el::PathReadTextOptions{el::StringEncoding::Utf8};
        options.setEncodingErrorMode(el::EncodingErrorMode::Throw);
        options.setTimeout(el::TimeDelta::seconds(1));
        try {
            const auto input = path.content().openTextInputStream(options);
            for (auto attempt = 0U; attempt < cMaximumAttempts; ++attempt) {
                if (!input->readAll(el::CpLength{100U}).isTimeout()) {
                    throw el::RuntimeError{"The damaged text was unexpectedly accepted."_el};
                }
            }
            throw el::RuntimeError{"The damaged text timed out."_el};
        } catch (const el::EncodingError &) {
            el::io::printLine("Invalid UTF-8 was rejected."_el);
        } catch (const el::PathError &) {
            throw el::RuntimeError{"The encoded text could not be opened."_el, std::current_exception()};
        } catch (const el::StreamError &) {
            throw el::RuntimeError{"The encoded text could not be read."_el, std::current_exception()};
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Invalid UTF-8 was rejected.

.. erbsland-demo-end::
