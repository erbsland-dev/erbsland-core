..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Streams; Overview
    single: Input Streams
    single: Output Streams

********************
Working with Streams
********************

The stream API moves binary data and decoded Unicode text through a uniform set of bounded operations.
It gives files, standard I/O, temporary files, memory-backed output, and custom data sources the same vocabulary.
Every public operation has a configured time bound, reads distinguish data from timeout and normal completion, and
writes accept a complete request or none of it.

The following sections map the complete API by task.
Use them to identify the part you need, then follow the linked topic for the detailed contracts and executable examples.

Creating Streams and Managing Ownership
=======================================

Start here when you need to choose a stream type, open a file, configure it, or decide how a component should receive a
stream.
The four central interfaces separate raw bytes from decoded text and input from output:

.. list-table::
    :header-rows: 1

    *   - Data
        - Read
        - Write
    *   - Exact binary representation
        - :cpp:class:`ByteInputStream <erbsland::stream::ByteInputStream>`
        - :cpp:class:`ByteOutputStream <erbsland::stream::ByteOutputStream>`
    *   - Decoded Unicode text
        - :cpp:class:`TextInputStream <erbsland::stream::TextInputStream>`
        - :cpp:class:`TextOutputStream <erbsland::stream::TextOutputStream>`

File streams are opened through :cpp:func:`Path::content() <erbsland::path::Path::content>` and returned with shared
ownership.
Read :doc:`creating_streams` for the opening methods, path and stream settings, explicit cleanup, and best practices for
passing the narrowest useful interface.

Byte Input and Output
=====================

Use byte streams for file formats, network frames, checksums, compressed content, or any operation where every byte is
significant.
They support bounded reads into spans, retained exact reads, explicitly limited aggregate reads, atomic writes, and
endian-aware integer operations.

If you are deciding how to handle a short read, resume after a timeout, cap memory use, or retry a write without
duplicating data, continue with :doc:`byte_input_and_output`.

Text Input, Output, and Encoding
================================

Text streams place the encoding boundary below your application logic.
They decode complete Unicode code points, read lines and bounded text blocks, write strings and formatted values, and
can capture generic text output in memory.

Use this API for configuration files, reports, logs, or any data whose meaning is text rather than bytes.
Read :doc:`text_input_and_output` for line semantics, UTF-8/16/32, byte order and BOM handling, encoding errors, and the
difference between configured and effective encoding.

Standard Streams
================

Standard input, output, and error connect a command-line application to its surrounding process.
The API provides stable ``stdIn()``, ``stdOut()``, and ``stdErr()`` proxies plus concise ``io`` formatting helpers.

Use these streams for interactive input, normal program output, diagnostics, and tests that temporarily capture or
provide process I/O.
Read :doc:`standard_streams` before redirecting them: redirects are scoped but process-wide, and standard proxies have
different ownership rules from ordinary files.

Readiness and Timeouts
======================

Bounded operations let an application remain responsive when a source is slow or a destination cannot immediately accept
work.
A timeout is a normal result, not end-of-stream and not a failed stream; readiness checks help decide whether retrying
can make progress.

Use this part of the API for pipes, terminals, concurrent producers, event-style loops, and any workflow that must
periodically regain control.
Read :doc:`readiness_and_timeouts` for complete input and output retry loops and the exact roles of ``isReady()`` and
``waitForReady()``.

Buffering and Back Pressure
===========================

Buffering smooths the difference between the size and timing of application requests and native I/O.
For output, accepted data may still be queued; back pressure prevents an application from growing that queue without a
limit.

Configure this area when moving large data, choosing bounded chunk sizes, controlling memory use, or coordinating
multiple producers.
Read :doc:`buffering_and_back_pressure` to understand front and back buffers, oversized requests, readiness, and when a
successful ``flush()`` matters.

Stream Lifecycle
================

Streams move through ``Open``, ``Closing``, ``Closed``, and ``Failed`` states.
Writes, flushing, graceful close, immediate abort, and destruction each make a different guarantee about queued work.

You need these contracts whenever output must not be lost, shutdown can time out, or a stored native failure must be
reported reliably.
Read :doc:`lifecycle` for close loops, failure propagation, abort behavior, and deterministic resource release.

Temporary Streams
=================

Temporary streams create a uniquely named byte or text file and combine its stream with an explicit cleanup policy.
They are useful for staging an export, materializing intermediate data for another tool, or producing a file that should
disappear unless ownership is deliberately transferred.

Read :doc:`temporary_streams` to configure location, names, access, encoding, and removal; retrieve the path; use
``release()``; and understand cleanup after close, abort, destruction, or failure.

Positioning Streams
===================

Positioning provides random access to streams whose backing target can support it.
Positions are logical encoded-byte offsets, so buffered read-ahead and queued writes do not change the position visible
to the caller.

Use positioning for fixed-size records, trailers, indexes, sparse output, or carefully designed random access in encoded
text.
Read :doc:`positioning` before seeking: not every stream supports it, append output is excluded, and text boundaries,
BOMs, retained input, and timeout behavior require deliberate handling.

Stream Errors and Diagnostics
=============================

The API keeps timeout, normal end-of-stream, encoding failure, and stream failure separate so recovery code does not
confuse ordinary flow control with broken I/O.
When a stream does fail, :cpp:class:`StreamError <erbsland::stream::StreamError>` carries a structured diagnostic with
operation, path, help, and native platform context.

Use :doc:`stream_errors` when designing logging, user-facing reports, recovery boundaries, or wrapped streams that must
preserve the original source context.

Coroutine Streams
=================

Coroutine operations execute blocking stream work through the worker service without changing the familiar read and
write result states.
Eager :cpp:class:`CoTask <erbsland::util::CoTask>` objects handle one operation, while lazy generators yield successive
blocks or lines, including timeout results.

Use them when a coroutine-based component must await I/O or consume a stream incrementally without occupying the caller
thread.
Read :doc:`coroutine_streams` for shared ownership across suspension, generator completion, cancellation by destruction,
single-consumer rules, and continuation-thread expectations.

Writing Custom Streams
======================

Custom streams connect the common API to an application-specific source or target: an archive entry, protocol channel,
device, generated data set, or test double.
The base classes supply higher-level behavior only when the implementation obeys the bounded-read, atomic-write,
lifecycle, readiness, failure, and optional-positioning contracts.

Use :doc:`custom_streams` when no built-in stream represents your boundary.
It develops complete custom byte input and output streams, explains error context and coroutine ownership, and finishes
with a contract-testing checklist.
