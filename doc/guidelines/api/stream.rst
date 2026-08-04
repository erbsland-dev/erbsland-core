*********************
Stream API Guidelines
*********************

Core Semantics
==============

Operation Model
---------------

.. code-block:: text

    bounded operation = completes within the configured timeout
    blocking operation = repeated bounded work until a terminal result
    readiness = observation without waiting or native input/output
    concurrency = thread-safe with caller-defined operation ordering
    close = graceful bounded drain; abort or destruction = immediate abandonment
    position = optional logical encoded-byte cursor

Result Model
------------

.. code-block:: text

    read data = successful result with a possibly partial payload
    read finished = successful end-of-stream without a payload
    read timeout = no payload consumed
    write success = complete input accepted atomically
    write timeout = no input accepted
    coroutine operation = same bounded result with owned data and retained stream ownership

Text and Sensitivity
--------------------

.. code-block:: text

    tolerant decoding = replace malformed input
    strict decoding = report an encoding error
    sensitive input = protected runtime buffers and marked generic owning results
    redirected input = independent target unaffected by native protected-input scopes

Primary Types
=============

.. code-block:: text

    InputStream, OutputStream // common readable and writable stream interfaces
    ByteInputStream, ByteOutputStream // raw-byte and endian-integer streams
    TextInputStream, TextOutputStream // decoded and encoded Unicode streams

Secondary Types
===============

.. code-block:: text

    StreamPositioning, StreamPositionOrigin // optional encoded-byte positioning capability
    InputStreamSettings, OutputStreamSettings, StreamBuffering // construction and buffering policies
    StreamReadResult❮Data❯, StreamReadStatus // payload-bearing and payload-free read results
    StreamWriteStatus, StreamCloseStatus, StreamWaitStatus // bounded operation results
    StreamPositionStatus, StreamState // positioning result and stream lifecycle
    AnyStringBuilderStream // text output adapter for a width-independent builder
    TempByteOutputStream, TempTextOutputStream // output streams with temporary-file cleanup
    StandardStreamRedirect // scoped replacement of process standard streams
    io::SensitiveInputToken, io::SensitiveInputScope // native standard-input protection handles
    StreamError, StreamErrorContext, StreamErrorSource // failure, context, and propagation interface

Pattern Definitions
===================

.. code-block:: text

    D = mem::ByteBlock/text::String // owned stream payload
    R = StreamReadResult❮Data❯ // typed read result

Lifecycle Patterns
==================

.. code-block:: text

    o.inputSettings()/outputSettings() -> T // inspect immutable construction settings
    o.state() -> StreamState // inspect lifecycle state
    o.isOpen()/isReady() -> bool // inspect acceptance or readiness without native input/output
    o.waitForReady() -> StreamWaitStatus // wait up to the configured timeout
    o.close() -> StreamCloseStatus // start or continue graceful close
    o.abort() // abandon pending work without waiting
    o.flush() -> StreamWriteStatus // drain and flush output within the timeout

Positioning Patterns
====================

.. code-block:: text

    o.supportsPositioning() -> bool // test the optional capability
    o.position() -> unit::ByteIndex // get the logical encoded-byte position
    o.setPosition(position) -> StreamPositionStatus // set an absolute position
    o.movePosition(origin, offset) -> StreamPositionStatus // move relative to start, current, or end

Read Patterns
=============

.. code-block:: text

    o.read(destination-or-maximum) -> R // read a partial byte or decoded-text payload
    o.readExact(length) -> R // read exactly one owned byte block or return a terminal status
    o.readByte/readChar() -> R // read one byte or decoded character
    o.readLine([maximum]) -> R // read through line feed, a limit, or end-of-stream
    o.readAll([maximum]) -> R // aggregate to end-of-stream under a caller bound
    o.read❮Integer❯() -> R // read one integer using configured byte order
    o.encoding()/effectiveEncoding() -> text::StringEncoding // inspect configured and BOM-resolved encoding
    o.setSensitive(enabled) -> InputStreamSettings& // protect retained input and mark ordinary read payloads

Write Patterns
==============

.. code-block:: text

    o.write(byte-or-text) -> StreamWriteStatus // atomically accept one complete value
    o.write❮Integer❯(value) -> StreamWriteStatus // atomically accept one endian integer
    o.writeLine([text]) -> StreamWriteStatus // atomically accept text and a line feed
    o.print/printLine(arguments) -> StreamWriteStatus // atomically accept formatted output
    o.endianness()/setEndianness(value) // inspect or configure integer byte order
    o.encoding() -> text::StringEncoding // inspect output encoding

Coroutine Patterns
==================

.. code-block:: text

    o.coRead/coReadExact/coReadLine/coReadAll([maximum]) -> util::CoTask❮R❯ // perform one owned bounded read
    o.coReadBlocks/coReadLines([maximum]) -> util::CoAsyncGenerator❮R❯ // lazily read until end-of-stream
    o.coWrite/coWriteLine(data) -> util::CoTask❮StreamWriteStatus❯ // atomically accept owned output

Standard Stream Patterns
========================

.. code-block:: text

    stdIn() -> TextInputStreamPtr // access the process standard-input proxy
    stdOut()/stdErr() -> TextOutputStreamPtr // access process standard-output proxies
    redirectStd❮Target❯(stream) -> StandardStreamRedirect // replace one standard target for a scope
    redirectStandardStreams(output, error) -> StandardStreamRedirect // replace both output targets for a scope
    o.isActive() -> bool // test whether a redirect is active
    o.reset() // release a redirect early
    startSensitiveInput() -> io::SensitiveInputToken // request protected native input buffers
    stopSensitiveInput(token) // release one protected-input request in any order
