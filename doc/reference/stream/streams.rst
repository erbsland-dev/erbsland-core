.. index::
    single: Streams

*******
Streams
*******

Bounded Operations
==================

All public stream operations have a bounded wait.
The timeout and buffer sizes are selected through
:cpp:class:`erbsland::stream::InputStreamSettings <erbsland::stream::InputStreamSettings>` or :cpp:class:`erbsland::stream::OutputStreamSettings <erbsland::stream::OutputStreamSettings>` when the
stream is opened.
The default timeout is one second.

Use :cpp:func:`erbsland::stream::InputStream::isReady <erbsland::stream::InputStream::isReady>` or
:cpp:func:`erbsland::stream::OutputStream::isReady <erbsland::stream::OutputStream::isReady>` for a pure readiness check. Use ``waitForReady()`` to apply
back pressure for up to the configured timeout.
Fully blocking behavior belongs in caller code as a loop over these bounded operations.

File Positioning
================

Streams opened for regular files support optional byte positioning unless the output file was opened in append mode.
Check :cpp:func:`erbsland::stream::StreamPositioning::supportsPositioning <erbsland::stream::StreamPositioning::supportsPositioning>` before using the positioning methods.
Pipes, terminals, standard-stream proxies, append-only output streams, and in-memory text builders do not promise this
capability.

The :cpp:func:`erbsland::stream::StreamPositioning::position <erbsland::stream::StreamPositioning::position>` method
returns a logical encoded-byte position.
Native input read-ahead does not change it, while bytes consumed into an incomplete retained read do.
Replaying such retained input does not count the bytes twice.
Accepted output advances the position immediately even while it is still queued.
For text input, a consumed byte-order mark and the original bytes of decoded characters count toward the position;
undecoded look-ahead does not.

Use ``setPosition()`` for an absolute position or ``movePosition()`` with
:cpp:enum:`erbsland::stream::StreamPositionOrigin <erbsland::stream::StreamPositionOrigin>` for a movement relative to the start, logical current position, or
current end.
Positions beyond the end are allowed.
A later write can therefore create a sparse file.
Negative, invalid, overflowing, or non-native positions raise
:cpp:class:`erbsland::err::ParameterError <erbsland::err::ParameterError>`.

Position changes are bounded operations.
A timeout leaves the logical position unchanged.
Successful input positioning discards read-ahead, decoder state, retained aggregate input, and end-of-stream state.
Output positioning waits for all accepted output to reach the native stream in order, but does not replace ``flush()``.

Text positions must lie on an encoded code-point boundary.
A misaligned position is handled by the configured encoding-error mode on the next read.
Generic UTF-16 and UTF-32 input must resolve its byte order before moving away from byte zero; use an explicit endian
encoding when immediate random access is required.
Returning to byte zero enables normal byte-order-mark detection or output again, while movement to a nonzero position
never inserts or consumes an interior byte-order mark.

Read Results
============

Read methods return :cpp:type:`erbsland::stream::StreamReadResult <erbsland::stream::StreamReadResult>` values.
The result derives from :cpp:class:`erbsland::stream::StreamReadStatus <erbsland::stream::StreamReadStatus>` and
provides typed tests such as ``hasData()``, ``isFinished()``, and ``isTimeout()``.
It also works with ``isSuccessful()`` and ``isFailure()``.
``Data`` and ``Finished`` are successful states; ``Timeout`` is a failure state.
Only ``Data`` has a caller-visible payload through ``data()`` or ``takeData()``.

Short reads are normal.
``readExact()``, text ``readLine()``, and ``readAll()`` retain incomplete input internally on timeout.
Repeat the same operation to resume it without joining fragments.
Selecting another read operation replays retained input in order.
``read()`` returns one chunk, ``readExact()`` completes only at its exact length, and ``readAll()`` aggregates until
end-of-stream or its maximum.
The no-argument byte and text ``readAll()`` methods have a 10 MiB default maximum.
Process large inputs with repeated bounded chunk reads instead of collecting them with ``readAll()``.

Atomic Output and Back Pressure
===============================

Text and byte writes never report a partially accepted request.
Output streams enqueue the complete request into a fixed front ring and a bounded growing back ring.
A request larger than the configured hard limit throws
:cpp:class:`erbsland::stream::StreamError <erbsland::stream::StreamError>` before accepting data.

When ``isReady()`` is true, there is no queued back-buffer data.
For a single producer, a following write whose encoded size fits ``backBufferLimit()`` can be accepted without waiting
unless the stream state changes.
Concurrent producers inspect every result.
``Success`` means the complete request was queued for asynchronous delivery; it does not mean the native target was
flushed.
``Timeout`` means nothing from that write was accepted, so retry the whole unchanged call.
Large producers should wait for readiness between chunks.
``flush()`` and ``close()`` can return timeout results while background work continues.

Coroutine Operations
====================

Byte and text streams provide ``co`` -prefixed methods for owned block, aggregate, line, and atomic write operations.
Each method returns an eager :cpp:class:`CoTask <erbsland::util::CoTask>` and runs the matching bounded synchronous
operation away from the caller thread.
Its result keeps the same stream status, including ``Timeout``, and stream or encoding errors are rethrown when the task
result is retrieved or awaited.

Coroutine operations retain shared ownership of the stream until the bounded call finishes.
Library-provided concrete streams are therefore factory-created and shared-owned.
Custom stream subclasses can still live on the stack for synchronous tests and algorithms, but invoking an inherited
coroutine method without shared ownership throws
:cpp:class:`LogicError <erbsland::err::LogicError>`.

``coReadBlocks()`` generates owned byte or text blocks, while ``coReadLines()`` generates text lines.
These lazy
:cpp:class:`CoAsyncGenerator <erbsland::util::CoAsyncGenerator>` sequences yield both data and timeout results and
terminate at end-of-stream.
Text blocks end at decoded code-point boundaries, and lines preserve the synchronous line-ending behavior.

Owned asynchronous output is useful when a producer coroutine or event-loop callback may otherwise wait for output
back-pressure.
Successful writes still mean that the complete atomic request was accepted, not that native output was flushed.
Coroutine continuations run on the coroutine worker service without caller-thread affinity.

Lifecycle
=========

``close()`` starts or continues graceful close.
Repeating it after a timeout observes the same close operation.
``abort()`` abandons pending work and returns immediately.
Destruction always aborts and never waits for native I/O, which prevents teardown from hanging on a deadlocked file,
pipe, terminal, or network share.

Stream Errors
=============

:cpp:class:`erbsland::stream::StreamError <erbsland::stream::StreamError>` carries a
``erbsland::stream::StreamErrorContext`` with a title, description, recovery help, and the stream path when one is
available.
The title states what failed and the description explains why it failed.
Stream wrappers delegate error-context creation to their backing stream, so errors such as a text-decoding positioning
failure retain the path of the file-backed stream.
Native operating-system details are embedded directly in the context, keeping diagnostics flat instead of creating a
nested platform-error cause.

Interface
=========

.. doxygenclass:: erbsland::stream::ByteInputStream
    :members:
.. doxygenclass:: erbsland::stream::ByteOutputStream
    :members:
.. doxygenclass:: erbsland::stream::InputStream
    :members:
.. doxygenclass:: erbsland::stream::InputStreamSettings
    :members:
.. doxygenclass:: erbsland::stream::OutputStream
    :members:
.. doxygenclass:: erbsland::stream::OutputStreamSettings
    :members:
.. doxygenclass:: erbsland::stream::StandardStreamRedirect
    :members:
.. doxygenfunction:: erbsland::stream::stdIn() -> TextInputStreamPtr

.. doxygenfunction:: erbsland::stream::stdOut() -> TextOutputStreamPtr

.. doxygenfunction:: erbsland::stream::stdErr() -> TextOutputStreamPtr

.. doxygenfunction:: erbsland::stream::redirectStdIn(TextInputStreamPtr input) -> StandardStreamRedirect

.. doxygenfunction:: erbsland::stream::redirectStdOut(TextOutputStreamPtr output) -> StandardStreamRedirect

.. doxygenfunction:: erbsland::stream::redirectStdErr(TextOutputStreamPtr error) -> StandardStreamRedirect

.. doxygenfunction:: erbsland::stream::redirectStandardStreams(TextOutputStreamPtr output, TextOutputStreamPtr error) -> StandardStreamRedirect

.. doxygenfunction:: erbsland::stream::io::write(text::Char character) -> StreamWriteStatus

.. doxygenfunction:: erbsland::stream::io::write(const text::StringView &text) -> StreamWriteStatus

.. doxygenfunction:: erbsland::stream::io::writeLine() -> StreamWriteStatus

.. doxygenfunction:: erbsland::stream::io::writeLine(const text::StringView &text) -> StreamWriteStatus

.. doxygenfunction:: erbsland::stream::io::print(const tArgs &...args) -> StreamWriteStatus

.. doxygenfunction:: erbsland::stream::io::printLine(const tArgs &...args) -> StreamWriteStatus

.. doxygenfunction:: erbsland::stream::io::printError(const tArgs &...args) -> StreamWriteStatus

.. doxygenfunction:: erbsland::stream::io::printErrorLine(const tArgs &...args) -> StreamWriteStatus
.. doxygenclass:: erbsland::stream::StreamCloseStatus
    :members:
.. doxygenclass:: erbsland::stream::StreamError
    :members:
.. doxygenclass:: erbsland::stream::StreamErrorContext
    :members:
.. doxygenclass:: erbsland::stream::StreamErrorSource
    :members:
.. doxygenclass:: erbsland::stream::StreamPositioning
    :members:
.. doxygenenum:: erbsland::stream::StreamPositionOrigin
.. doxygenclass:: erbsland::stream::StreamPositionStatus
    :members:
.. doxygentypedef:: erbsland::stream::StreamReadResult
.. doxygenclass:: erbsland::stream::StreamReadStatus
    :members:
.. doxygenenum:: erbsland::stream::StreamState
.. doxygenclass:: erbsland::stream::StreamWaitStatus
    :members:
.. doxygenclass:: erbsland::stream::StreamWriteStatus
    :members:
.. doxygenclass:: erbsland::stream::StringBuilderStream
    :members:
.. doxygenclass:: erbsland::stream::TempByteOutputStream
    :members:
.. doxygenclass:: erbsland::stream::TempTextOutputStream
    :members:
.. doxygenclass:: erbsland::stream::TextInputStream
    :members:
.. doxygenclass:: erbsland::stream::TextOutputStream
    :members:
.. doxygenclass:: erbsland::stream::TextPrintContext
    :members:
