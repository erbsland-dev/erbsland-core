..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Streams; Byte Input
    single: Streams; Byte Output
    single: Streams; Short Reads
    single: Streams; Atomic Writes

*********************
Byte Input and Output
*********************

Byte streams let you read and write data without text encoding or newline conversion.
They preserve every bit, which makes them the right foundation for binary file formats, network protocols, compressed
data, checksums, and components that perform their own decoding.

Erbsland Core provides high-level operations for common tasks, such as reading an exact record, collecting a small
source, and reading or writing fixed-width integers.
It also provides bounded chunk operations for algorithms that must control memory and process large sources
incrementally.
This page introduces both levels and explains the result, timeout, and buffering rules that keep byte-stream code
reliable.

The Byte Stream Interfaces
==========================

The two central APIs are :cpp:class:`ByteInputStream <erbsland::stream::ByteInputStream>` and
:cpp:class:`ByteOutputStream <erbsland::stream::ByteOutputStream>`.
They extend the common input and output interfaces with operations for raw bytes and fixed-width integers.

.. mermaid::

    classDiagram
        class InputStream {
            +isReady() bool
            +waitForReady() StreamWaitStatus
            +close() StreamCloseStatus
        }
        class ByteInputStream {
            +read(destination) StreamReadResult
            +read(maximumLength) StreamReadResult
            +readExact(length) StreamReadResult
            +readAll(maximumLength) StreamReadResult
            +readByte() StreamReadResult
            +readInteger() StreamReadResult
        }
        class OutputStream {
            +isReady() bool
            +waitForReady() StreamWaitStatus
            +flush() StreamWriteStatus
            +close() StreamCloseStatus
        }
        class ByteOutputStream {
            +write(byte) StreamWriteStatus
            +write(bytes) StreamWriteStatus
            +writeInteger(value) StreamWriteStatus
        }
        InputStream <|-- ByteInputStream
        OutputStream <|-- ByteOutputStream

Choose an input method according to the shape of the data you need:

``readByte()``
    Reads one byte.
    This is convenient for tags, flags, and small parsers where individual bytes have meaning.

``read(maximumLength)``
    Returns one owned block with at most the requested length.
    Use it when ownership is useful but the exact chunk size is not important.

``readExact(length)``
    Returns data only after the complete requested length is available.
    This is the usual choice for fixed-size headers and records.

``readAll(maximumLength)``
    Collects input until the source finishes or the limit is reached.
    Use it for sources that are expected and required to remain small.

``read(destination)``
    Reads directly into caller-owned memory and reports how many bytes were initialized.
    This is the low-level option for bounded processing loops.

The output side deliberately has one simple rule: every ``write()`` accepts the complete byte, span, or
:cpp:class:`ByteBlockView <erbsland::mem::ByteBlockView>`, or accepts nothing.
The integer helpers build on the same read and write contracts and add configurable byte order.

Handle Every Read Result
========================

Every byte read returns a :cpp:type:`StreamReadResult <erbsland::stream::StreamReadResult>`.
The status tells you whether the result contains data, reached the normal end of the stream, or reached its timeout.
Only call ``data()`` or ``takeData()`` after ``hasData()`` returned true.

.. list-table::
    :header-rows: 1
    :widths: 18 38 44

    *   - Result
        - Promise
        - Typical response
    *   - ``Data``
        - The payload or reported destination length is valid.
        - Process exactly that data.
    *   - ``Finished``
        - The source reached its normal end without returning a payload.
        - Finish the loop, or report a truncated structure if more data was required.
    *   - ``Timeout``
        - The bounded operation did not produce a complete result in time.
        - Keep the stream open and retry, reschedule, or cancel according to the application policy.

``Data`` and ``Finished`` are successful states.
``Timeout`` is a normal bounded-flow result, not an exception and not end-of-stream.
Failures such as a closed stream, inaccessible device, or native read error throw
:cpp:class:`StreamError <erbsland::stream::StreamError>` instead.
Catch that exception where you can add useful application context, recover, or report the failed source.

The following demo reads a four-byte geometry-file marker with ``readExact()``.
It checks timeout and premature end before accessing the payload, and translates a ``StreamError`` into an
application-level error with more context.
In a larger application, the timeout branch could schedule another attempt instead of returning to its caller.

.. erbsland-demo::
    :source: stream/ByteInputAndOutput/ReadBinaryHeader.cpp
    :exec: stream/byte_input_and_output --demo ReadBinaryHeader
    :source-sha256: c94c4e69965255cc5175dd02f03f718c110fcc113b6b2c37dc34ef1851a903dc

.. code-block:: cpp

    /// Handle every result of an exact binary-header read.
    /// `Data`, `Finished`, and `Timeout` are expected result states, while a `StreamError` reports a failed source.
    void inspectBinaryHeader(el::ByteInputStream &input) {
        try {
            const auto result = input.readExact(el::ByteLength{4U});

            if (result.isTimeout()) {
                el::io::printLine("The geometry-file header is temporarily unavailable."_el);
                return;
            }
            if (result.isFinished()) {
                el::io::printLine("The geometry file does not contain a complete header."_el);
                return;
            }

            const auto &header = result.data();
            el::io::printLine("Geometry-file marker: "_el, el::String::fromByteBlock(header, el::ByteFormat::separated()));
        } catch (const el::StreamError &) {
            throw el::RuntimeError{"The geometry-file header could not be read."_el, std::current_exception()};
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Geometry-file marker: 47 45 4f 31

.. erbsland-demo-end::

Continue Reads After a Timeout
==============================

High-level reads can consume several native chunks before they have a complete result.
If :cpp:func:`readExact() <erbsland::stream::ByteInputStream::readExact>` times out after receiving only part of a
record, the stream retains those bytes internally and returns an empty ``Timeout`` result.
Repeat ``readExact()`` with the same length to continue that logical read.
The next successful result contains the complete record, including the retained prefix.

:cpp:func:`readAll() <erbsland::stream::ByteInputStream::readAll>` follows the same rule.
It retains the bytes collected before a timeout and continues when you repeat ``readAll()`` with the same limit.
You never have to expose, join, or account for partial aggregate input yourself.
This avoids subtle bugs where a retry loses or duplicates bytes.

Selecting a different read operation or changing the requested aggregate length cancels the logical read.
The retained bytes are not discarded; the stream replays them in their original order to the newly selected operation.
The timeout applies once to each public call, even if that call performs several native reads internally.

Timeouts are fixed when a stream is opened.
For a file-backed input stream, set one explicitly through
:cpp:func:`PathReadDataOptions::setTimeout() <erbsland::path::PathReadDataOptions::setTimeout>`:

.. code-block:: cpp

    auto options = el::PathReadDataOptions{};
    options.setTimeout(el::TimeDelta::seconds(1));
    const auto input = path.content().openByteInputStream(options);

Choose a value that keeps one operation responsive, then use a bounded retry policy for the complete application
operation.
The demo below permits three attempts.
After a timeout it repeats the unchanged four-byte read; it does not ask for the missing suffix because the stream
already owns the incomplete prefix.
It also treats premature end, repeated timeouts, and ``StreamError`` as distinct failures.

.. erbsland-demo::
    :source: stream/ByteInputAndOutput/ResumeExactRead.cpp
    :exec: stream/byte_input_and_output --demo ResumeExactRead
    :source-sha256: 19d09c85473d251e5beda0e9bc1c00a02b850331b30136a4ccff6aa4c6815489

.. code-block:: cpp

    /// Retry a complete logical read after a timeout.
    /// `readExact()` retains incomplete input internally, so always repeat the same operation with the same length.
    auto readRecordWithTimeouts(el::ByteInputStream &input) -> el::ByteBlock {
        constexpr auto cMaximumAttempts = 3U;

        try {
            for (auto attempt = 0U; attempt < cMaximumAttempts; ++attempt) {
                auto result = input.readExact(el::ByteLength{4U});
                if (result.hasData()) {
                    return result.takeData();
                }
                if (result.isFinished()) {
                    throw el::RuntimeError{"The geometry record ended before it was fully read."_el};
                }

                el::io::printLine("Reading timed out; retrying the complete record."_el);
            }
        } catch (const el::StreamError &) {
            throw el::RuntimeError{"The geometry record could not be read."_el, std::current_exception()};
        }
        throw el::RuntimeError{"Waiting for the geometry record took too long."_el};
    }

.. erbsland-ansi::
    :escape-char: ␛

    Reading timed out; retrying the complete record.
    Complete record bytes: 4

.. erbsland-demo-end::

Read a Small Source with a Limit
================================

Sometimes a function receives only a stream but needs the remaining content as one value.
For example, a decoder may accept a small embedded resource through a generic stream interface and reject anything
larger than its format allows.
That is the purpose of ``readAll()``: it offers a convenient aggregate read without assuming that every stream is safe
to collect without a bound.

The no-argument ``readAll()`` uses
:cpp:var:`ByteInputStream::cDefaultByteReadMaximum <erbsland::stream::ByteInputStream::cDefaultByteReadMaximum>`, which
is 10 MiB.
Pass an explicit finite maximum when your format or application has a different limit.
If the source finishes first, the returned ``Data`` contains all remaining bytes.
If the limit is reached first, ``Data`` contains exactly the bytes collected up to that limit, and a later read
continues at the next byte.

The result does not guess whether a source that exactly fills the limit has more data.
When oversize input must be rejected, allow one additional byte and reject a result longer than the accepted maximum.
For large or attacker-controlled sources, prefer the bounded chunk loop at the end of this page so memory use does not
grow with the source.

When you already have a :cpp:class:`Path <erbsland::path::Path>`, use
:cpp:func:`PathContent::readData() <erbsland::path::PathContent::readData>` or
:cpp:func:`PathContent::readDataOrThrow() <erbsland::path::PathContent::readDataOrThrow>` instead.
These methods express the whole-file intent directly and apply the maximum in
:cpp:class:`PathReadDataOptions <erbsland::path::PathReadDataOptions>`.
``readAll()`` remains useful for reusable functions that receive a stream and deliberately accept only small input.

Write Complete Atomic Requests
==============================

Erbsland Core output streams use a high-level asynchronous delivery model.
A write first queues the complete request in stream-managed memory.
A background worker then transfers queued bytes to the file, pipe, terminal, or other native destination while your code
continues.
This design gives every write an atomic acceptance result and absorbs short periods in which the destination stalls.

The queue combines a fixed front buffer with a growing but bounded back buffer.
The hard bound is
:cpp:func:`OutputStreamSettings::backBufferLimit() <erbsland::stream::OutputStreamSettings::backBufferLimit>` and can be
changed with ``setBackBufferLimit()`` before the stream is opened.
A single request larger than this limit throws :cpp:class:`StreamError <erbsland::stream::StreamError>` before any byte
is accepted.
Split larger transfers into application-level chunks.

For each write, handle these two results:

``Success``
    The complete unchanged request was accepted in order.
    It may still be buffered, so use ``flush()`` or a successful ``close()`` when native delivery must be confirmed.

``Timeout``
    The stream could not accept the complete request within the configured timeout, and accepted none of it.
    You can safely retry the same unchanged request.

Always inspect the write result, even after a readiness check.
:cpp:func:`OutputStream::isReady() <erbsland::stream::OutputStream::isReady>` is a non-blocking snapshot that tells a
single producer there is currently no queued back-buffer data.
It can help a producer avoid preparing more work while output is congested, but it does not reserve capacity.
Another producer or a stream-state change can invalidate the observation before ``write()``.
See :doc:`readiness_and_timeouts` for readiness waits and concurrency details.

The demo opens a real temporary file with a one-second timeout.
It keeps the record unchanged and retries at most 60 times, so stalled acceptance can consume at most about one minute.
Each timed-out call accepted no bytes; therefore a later success writes the record exactly once.
The ``isReady()`` check reports congestion without replacing the mandatory status check, while the ``StreamError``
handler aborts queued work and adds application context.
Finally, a successful ``close()`` confirms that the accepted bytes reached the file before the hidden setup verifies its
length.

.. erbsland-demo::
    :source: stream/ByteInputAndOutput/WriteAtomicRecord.cpp
    :exec: stream/byte_input_and_output --demo WriteAtomicRecord
    :source-sha256: f21b09d5cc7a1b4b84f448a2cab8ed3eaba0d2821db2ce3490daf3ca99a623f1

.. code-block:: cpp

    /// Retry a complete byte write for at most one minute.
    /// A timeout accepts no bytes, so keep the record unchanged and repeat the complete call.
    void writeAtomicRecord(const el::Path &path) {
        constexpr auto cMaximumAttempts = 60U;

        auto options = el::PathWriteDataOptions{};
        options.setTimeout(el::TimeDelta::seconds(1));
        const auto output = path.content().openByteOutputStream(options);
        const auto record = el::ByteBlock{std::vector<uint8_t>{4U, 8U, 15U, 16U, 23U, 42U}};

        try {
            for (auto attempt = 0U; attempt < cMaximumAttempts; ++attempt) {
                if (!output->isReady()) {
                    el::io::printLine("The output is still processing earlier data."_el);
                }
                if (output->write(el::ByteBlockView{record}).isSuccess()) {
                    if (output->close().isTimeout()) {
                        output->abort();
                        throw el::RuntimeError{"Closing the geometry-record file timed out."_el};
                    }
                    return;
                }
            }
        } catch (const el::StreamError &) {
            output->abort();
            throw el::RuntimeError{"The geometry record could not be written."_el, std::current_exception()};
        }

        output->abort();
        throw el::RuntimeError{"Waiting for the geometry-record output took longer than one minute."_el};
    }

.. erbsland-ansi::
    :escape-char: ␛

    Written bytes: 6

.. erbsland-demo-end::

Read and Write Fixed-Width Integers
===================================

Binary formats usually describe integers by signedness, exact width, and byte order rather than by a platform C++ type.
:cpp:class:`ByteInputStream <erbsland::stream::ByteInputStream>` provides ``readInt8()``, ``readUInt8()``,
``readInt16()``, ``readUInt16()``, ``readInt32()``, ``readUInt32()``, ``readInt64()``, and ``readUInt64()``.
:cpp:class:`ByteOutputStream <erbsland::stream::ByteOutputStream>` provides matching ``write`` methods.
The generic ``readInteger<T>()`` and ``writeInteger<T>()`` variants support other integral call sites while preserving
the same fixed ``sizeof(T)`` representation.

For multi-byte values, set :cpp:enum:`Endianness <erbsland::mem::Endianness>` to the order specified by the format.
Little endian stores the least significant byte first; big endian stores the most significant byte first.
The stream defaults to little endian, but explicit setup makes format code easier to verify and independent of
assumptions about the host platform.
Input and output streams keep their own setting, so configure both sides when they exchange the same format.
Byte order has no effect on eight-bit values.

Integer input uses ``readExact()`` internally.
A ``Data`` result therefore contains one complete value, ``Finished`` means the source ended before that value could be
completed, and ``Timeout`` exposes no partial integer.
After timeout, repeat the same integer helper: the stream retains any bytes already read for that value.
Integer output encodes the complete value into one atomic write request.
``Success`` accepts every encoded byte, while ``Timeout`` accepts none and allows the unchanged value to be retried.
Native input or output failures still throw :cpp:class:`StreamError <erbsland::stream::StreamError>`.

The demo reads a big-endian 16-bit geometry angle and writes a big-endian signed 32-bit coordinate offset.
Its deterministic stream setup is hidden.
The visible input function limits retries, distinguishes premature end from timeout, and translates a ``StreamError``
with geometry-specific context.
The output function retries the same signed value after timeout, which is safe because none of its four encoded bytes
were accepted.

.. erbsland-demo::
    :source: stream/ByteInputAndOutput/ReadAndWriteIntegers.cpp
    :exec: stream/byte_input_and_output --demo ReadAndWriteIntegers
    :source-sha256: 0b50b4621041c909164d43561948b0b328c57084a9cd851c43c8471c443b9abf

.. code-block:: cpp

    /// Read a fixed-width integer with the byte order defined by the binary format.
    /// Integer reads use exact-read semantics.
    /// Retry the same helper after timeout and handle premature end separately.
    auto readGeometryAngle(el::ByteInputStream &input) -> uint16_t {
        constexpr auto cMaximumAttempts = 3U;
        input.setEndianness(el::Endianness::Big);

        try {
            for (auto attempt = 0U; attempt < cMaximumAttempts; ++attempt) {
                const auto result = input.readUInt16();
                if (result.hasData()) {
                    return result.data();
                }
                if (result.isFinished()) {
                    throw el::RuntimeError{"The geometry angle is incomplete."_el};
                }
                el::io::printLine("Reading the angle timed out; retrying."_el);
            }
        } catch (const el::StreamError &) {
            throw el::RuntimeError{"The geometry angle could not be read."_el, std::current_exception()};
        }
        throw el::RuntimeError{"Waiting for the geometry angle took too long."_el};
    }

    /// Write a fixed-width integer as one atomic request.
    /// A timeout accepts none of the encoded bytes, so retry the same value without changing it.
    void writeGeometryOffset(el::ByteOutputStream &output, const int32_t offset) {
        constexpr auto cMaximumAttempts = 3U;
        output.setEndianness(el::Endianness::Big);

        try {
            for (auto attempt = 0U; attempt < cMaximumAttempts; ++attempt) {
                if (output.writeInt32(offset).isSuccess()) {
                    return;
                }
                el::io::printLine("Writing the offset timed out; retrying."_el);
            }
        } catch (const el::StreamError &) {
            throw el::RuntimeError{"The geometry offset could not be written."_el, std::current_exception()};
        }

        throw el::RuntimeError{"Waiting for the geometry offset output took too long."_el};
    }

.. erbsland-ansi::
    :escape-char: ␛

    Reading the angle timed out; retrying.
    Writing the offset timed out; retrying.
    Angle units: 300
    Coordinate offset: -720

.. erbsland-demo-end::

Process Large Input in Bounded Chunks
=====================================

Use :cpp:func:`read(span) <erbsland::stream::ByteInputStream::read>` when an algorithm must process large input without
collecting it in memory.
It fills as much of the destination as is currently available and returns the initialized length.
A ``Data`` result can therefore be shorter than the span without indicating a problem.
This behavior is often called a *short read*.

Choose a reusable buffer that suits the workload.
Tens of KiB are a practical starting point for ordinary file processing, while a protocol or codec may have a natural
block size of its own.
After each ``Data`` result, pass only the reported prefix to the next stage.
On ``Timeout``, do not process the buffer because the result reports no new bytes; retry or return control according to
your scheduling policy.
Stop only when the stream reports ``Finished``, and handle ``StreamError`` separately from normal result states.

The final demo sketches a streaming compressor for a prime-number sequence.
The 64 KiB buffer is reused for every call, but the fictive compressor sees only the prefix named by the ``Data``
result.
A timeout restarts the bounded read without compressing stale buffer contents, while 60 consecutive timeouts stop a
source that remains stalled.
A source failure is rethrown with context.

.. erbsland-demo::
    :source: stream/ByteInputAndOutput/ProcessPayloadInChunks.cpp
    :exec: stream/byte_input_and_output --demo ProcessPayloadInChunks
    :source-sha256: 23da4ff22afa8c6388b97df310fdd82f9c546b0ba7151efaaa016dafb624e8dc

.. code-block:: cpp

    /// Feed large binary input into a compressor without collecting the complete source in memory.
    /// A short `Data` result is normal; only the reported prefix of the buffer contains new input.
    void compressPrimeSequence(el::ByteInputStream &input, PrimeSequenceCompressor &compressor) {
        constexpr auto cMaximumConsecutiveTimeouts = 60U;
        auto buffer = std::array<el::Byte, 64U * 1024U>{};
        auto consecutiveTimeouts = 0U;

        try {
            while (true) {
                const auto result = input.read(buffer);
                if (result.isFinished()) {
                    break;
                }
                if (result.isTimeout()) {
                    if (++consecutiveTimeouts == cMaximumConsecutiveTimeouts) {
                        throw el::RuntimeError{"The prime sequence did not provide data for too long."_el};
                    }
                    continue;
                }

                consecutiveTimeouts = 0U;
                compressor.compress(std::span<const el::Byte>{buffer.data(), result.data().toSizeT()});
            }
        } catch (const el::StreamError &) {
            throw el::RuntimeError{"The prime sequence could not be compressed."_el, std::current_exception()};
        }
        compressor.finish();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Compressed input: 7 bytes

.. erbsland-demo-end::
