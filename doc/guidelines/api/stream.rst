*********************
Stream API Guidelines
*********************

These guidelines extend the Common API Guidelines for public APIs in the ``stream`` namespace.
This namespace provides explicit byte and text input/output interfaces.
If you introduce new vocabulary or stream types, update this page.

Core Semantics
==============

*   Streams are explicit read/write objects.
*   Input and output are separate interfaces.
*   Byte and text streams are separate interfaces.
*   Public operations have a bounded wait. The timeout is fixed when the stream is created and defaults to one
    second.
*   A caller that needs blocking behavior repeats bounded calls and handles timeout results in its own loop.
*   ``isReady()`` only observes state and never processes events or waits for native I/O.
*   Stream status types derive from ``util::Result``. Payload-bearing results derive from their specialized status
    through ``util::ResultWithData``.
*   End-of-stream is a successful final result, not an error.
*   Public stream methods are thread-safe. Concurrent operations have no guaranteed ordering unless the caller
    provides it.
*   Positioning is an optional, stable capability. Positions and offsets use encoded bytes for byte and text streams.
*   Coroutine stream methods use the ``co`` prefix so suspension is visible at the call site.
*   Coroutine operations require shared stream ownership and keep that ownership until their bounded operation
    completes. A coroutine call on a non-shared stream throws ``LogicError``.

Lifecycle
---------

*   ``close()`` starts or continues graceful close and may return a timeout while closing continues in the
    background.
*   ``abort()`` abandons pending work and returns immediately.
*   Stream destruction never calls ``close()`` and never waits. Concrete destructors call ``abort()``.
*   A failed native operation changes the stream state to ``Failed`` and is rethrown by the next applicable public
    operation.

Input Streams
-------------

*   Input uses fixed-capacity read-ahead rings. Native APIs write directly into contiguous writable ring sections.
*   A data result may contain fewer bytes or characters than requested.
*   ``Timeout`` and ``Finished`` read results never expose partial payloads.
*   Exact, line, and aggregate reads retain incomplete input internally. Repeating the same operation resumes it;
    selecting another read operation replays retained input in order.
*   Encoded text input streams preserve incomplete byte sequences in a bounded decode buffer.
*   Allocating read methods require finite maxima. ``readAll()`` is bounded by its explicit or 10 MiB default maximum;
    large inputs must be processed through repeated chunk reads.

Output Streams
--------------

*   Every ``write()``, ``writeLine()``, ``print()``, and ``printLine()`` request is atomic from the caller's point of
    view. It is either accepted completely or not accepted.
*   Output uses a fixed front ring and a bounded, dynamically growing back ring. Native APIs read directly from
    contiguous front-ring sections.
*   ``Timeout`` from a write means nothing from that request was accepted, so the whole request can be retried.
*   With a single producer, ``isReady()`` permits ignoring the result of a following write whose encoded data fits in
    ``backBufferLimit()`` unless stream state changes. Concurrent producers inspect every result.
*   A single request larger than ``backBufferLimit()`` throws ``StreamError`` without accepting a prefix.
*   Large producers call ``isReady()`` or ``waitForReady()`` between chunks to apply back pressure.

Coroutine Operations
--------------------

*   A ``coRead...`` or ``coWrite...`` call returns an eager :cpp:class:`CoTask <erbsland::util::CoTask>` and performs
    the matching bounded synchronous operation on the independent coroutine worker service.
*   Tasks preserve synchronous result types and statuses, including ``Timeout``. Stream and encoding errors propagate
    through task result retrieval or ``co_await``.
*   Owned payloads cross coroutine boundaries: byte writes take ``ByteBlock``, text writes take ``String``, and
    allocating reads return owned blocks or strings. Span, view, single-byte, integer, print, flush, positioning,
    close, and abort operations have no coroutine variants.
*   ``coReadBlocks()`` and ``coReadLines()`` return lazy
    :cpp:class:`CoAsyncGenerator <erbsland::util::CoAsyncGenerator>` sequences. They yield data and timeout results;
    end-of-stream terminates the sequence without yielding ``Finished``.
*   Text-block generators never split a code point and do not expose empty decoder-progress chunks.
*   Continuations have no caller-thread affinity. Callers explicitly dispatch to an event or UI thread when required.
*   Library-provided concrete streams are obtained from shared-pointer factories. Custom implementations may remain
    stack-backed for synchronous use, but inherited coroutine calls require shared ownership.

Positioning
-----------

*   ``supportsPositioning()`` reports whether a stream has a logical byte cursor that can be changed.
*   Native input read-ahead does not advance the logical position. Input retained by an incomplete read does advance
    it, and replaying retained input does not advance it again.
*   Accepted output advances the logical position immediately, including output still queued for native delivery.
*   Text-input positions count original encoded bytes. Consumed byte-order marks count; undecoded look-ahead does not.
*   Positioning input discards retained and buffered data and resets end-of-stream state.
*   Positioning output first delivers accepted output in order. It does not imply a native flush.
*   Append-only output streams do not support positioning because native append semantics override the cursor.
*   Positioning may return ``Timeout`` without changing the logical position.

Primary Types
=============

.. code-block:: text

    InputStream // common base class for readable streams
    OutputStream // common base class for writable streams
    ByteInputStream // reads raw bytes and endian integer values
    ByteOutputStream // writes raw bytes and endian integer values
    TextInputStream // reads incrementally decoded Unicode text
    TextOutputStream // atomically writes encoded Unicode text
    InputStreamSettings // timeout and input ring capacity
    OutputStreamSettings // timeout, front ring capacity, and back-ring limit
    StreamPositioning // optional byte-position capability shared by input and output streams
    StreamPositionOrigin // Start, Current, or End reference for relative positioning
    StreamErrorSource // stream error-context propagation extension point
    StreamError // failed stream operation with structured diagnostic context
    StreamErrorContext // title, description, help, path, and native failure details
    TempByteOutputStream // byte output stream with temporary-file cleanup
    TempTextOutputStream // text output stream with temporary-file cleanup
    StandardStreamRedirect // scoped replacement for standard stream targets

Status Types
============

.. code-block:: text

    StreamState // Open, Closing, Closed, or Failed
    StreamReadStatus // Data, Finished, or Timeout
    StreamWriteStatus // Success or Timeout
    StreamCloseStatus // Closed or Timeout
    StreamWaitStatus // Ready or Timeout
    StreamPositionStatus // Success or Timeout
    StreamReadResult<T> // read status with a payload

Common Stream Patterns
======================

.. code-block:: text

    o.inputSettings()/outputSettings() // immutable creation settings
    o.state() -> StreamState // current lifecycle state
    o.isOpen() -> bool // test if new operations are accepted
    o.isReady() -> bool // pure, non-waiting readiness test
    o.waitForReady() -> StreamWaitStatus // wait up to the configured timeout
    o.close() -> StreamCloseStatus // start or continue graceful close
    o.abort() // abandon pending work without waiting
    o.flush() -> StreamWriteStatus // output only: drain and flush within the timeout

Error Reporting
===============

Every ``StreamError`` has a user-facing title and description.
The title states what operation failed; the description explains why it failed.
Stream implementations report local failures through ``StreamErrorSource::throwError(title, description)``.
Decorators override ``createErrorContext()`` and delegate to their wrapped stream, so file paths and native platform
details remain available in the final diagnostic.

``StreamErrorSource`` is a public extension point for stream implementations and adapters.
Application code normally handles a ``StreamError`` from the operation it invoked instead of calling the error hook
directly.

Positioning Patterns
====================

.. code-block:: text

    o.supportsPositioning() -> bool // test if the stream supports byte positioning
    o.position() -> ByteIndex // get the logical encoded-byte position
    o.setPosition(position) -> StreamPositionStatus // set an absolute position
    o.movePosition(origin, offset) -> StreamPositionStatus // move relative to Start, Current, or End

Byte Stream Patterns
====================

.. code-block:: text

    o.endianness() -> mem::Endianness // byte order for integer helpers
    o.setEndianness(endianness) // set byte order for integer helpers
    o.read(span) -> StreamReadResult<ByteLength> // read into caller storage
    o.read(length) -> StreamReadResult<ByteBlock> // read one bounded chunk
    o.readExact(length) -> StreamReadResult<ByteBlock> // atomically read an exact amount across retries
    o.readByte() -> StreamReadResult<Byte> // read one byte
    o.readAll()/readAll(maximum) -> StreamReadResult<ByteBlock> // bounded convenience aggregation
    o.read❮IntType❯() -> StreamReadResult<T> // read an endian integer
    o.write(byte/span/block) -> StreamWriteStatus // atomically accept all bytes
    o.write❮IntType❯(value) -> StreamWriteStatus // atomically accept an endian integer

Text Stream Patterns
====================

.. code-block:: text

    o.encoding() -> StringEncoding // configured encoding
    o.effectiveEncoding() -> StringEncoding // concrete encoding after BOM resolution
    o.read()/read(maximum) -> StreamReadResult<String> // read a decoded chunk
    o.readChar() -> StreamReadResult<Char> // read one decoded character
    o.readLine()/readLine(maximum) -> StreamReadResult<String> // atomically read through LF, a limit, or EOF
    o.readAll()/readAll(maximum) -> StreamReadResult<String> // bounded convenience aggregation
    o.write(character/text) -> StreamWriteStatus // atomically accept text
    o.writeLine()/writeLine(text) -> StreamWriteStatus // atomically accept text and LF
    o.print(args...) -> StreamWriteStatus // atomically accept formatted convenience output
    o.printLine(args...) -> StreamWriteStatus // atomically accept formatted output and LF

Coroutine Stream Patterns
=========================

.. code-block:: text

    o.coRead(length) -> CoTask<StreamReadResult<ByteBlock-or-String>> // read one owned block.
    o.coReadExact(length) -> CoTask<StreamReadResult<ByteBlock>> // byte input only: read an exact owned block.
    o.coReadLine([maximum]) -> CoTask<StreamReadResult<String>> // text input only: read one line.
    o.coReadAll([maximum]) -> CoTask<StreamReadResult<ByteBlock-or-String>> // bounded aggregate read.
    o.coReadBlocks([maximum]) -> CoAsyncGenerator<StreamReadResult<ByteBlock-or-String>> // repeated blocks until EOF.
    o.coReadLines([maximum]) -> CoAsyncGenerator<StreamReadResult<String>> // repeated lines until EOF.
    o.coWrite(ByteBlock-or-String) -> CoTask<StreamWriteStatus> // atomically accept owned output.
    o.coWriteLine([ownedString]) -> CoTask<StreamWriteStatus> // text output only: atomically accept a line.

Standard Stream Patterns
========================

.. code-block:: text

    stdIn() -> TextInputStreamPtr // process-wide standard input proxy
    stdOut()/stdErr() -> TextOutputStreamPtr // process-wide standard output/error proxy streams
    redirectStdIn(stream) -> StandardStreamRedirect // scoped replacement for stdIn()
    redirectStdOut(stream) -> StandardStreamRedirect // scoped replacement for stdOut()
    redirectStdErr(stream) -> StandardStreamRedirect // scoped replacement for stdErr()
    redirectStandardStreams(output, error) -> StandardStreamRedirect // scoped replacement for both streams
    redirect.isActive() -> bool // test if a redirect guard still owns a replacement
    redirect.reset() // restore the previous stream targets early
