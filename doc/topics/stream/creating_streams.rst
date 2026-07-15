..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Streams; Creating
    single: Streams; Ownership
    single: Streams; File Streams

***************************************
Creating Streams and Managing Ownership
***************************************

This page shows how to select a byte or text interface, open file-backed streams through
:cpp:class:`Path <erbsland::path::Path>`, configure them before the first operation, and pass them safely through an
application.
The goal is to keep components independent of a particular destination while making ownership and cleanup explicit.

Choose a Narrow Stream Interface
================================

The stream API deliberately separates byte from text data and input from output.
Choose the interface at the boundary where your application decides what the data *means*.
A binary inspector needs :cpp:type:`ByteInputStreamPtr <erbsland::stream::ByteInputStreamPtr>` because the encoded bytes
matter; a document reader needs :cpp:type:`TextInputStreamPtr <erbsland::stream::TextInputStreamPtr>` because it works
with decoded Unicode.

The following example opens the same UTF-8 file twice.
The two visible functions receive only the interface they need: one exposes the exact file bytes as hexadecimal, while
the other receives decoded text.

.. erbsland-demo::
    :source: stream/CreatingStreams/ChooseStreamFamily.cpp
    :exec: stream/creating_streams --demo ChooseStreamFamily
    :source-sha256: 12516c1a60ba5a984f70533db3b0940f900fe6e74f4d6235ed1f6095833d66f8

.. code-block:: cpp

    /// Accept a byte input stream when a component must work with the exact file representation.
    /// This function can read any byte source; it neither knows nor cares that the caller opened a file.
    void printFileAsHex(const el::ByteInputStreamPtr &input) {
        const auto result = input->readAll(el::ByteLength{1024U});
        if (result.hasData()) {
            // The UTF-8 encoding of “Ö” is visible as the final two bytes.
            el::io::printLine("Bytes: "_el, el::ByteFormat::separated(), result.data());
        }
    }

    /// Accept a text input stream when a component needs decoded Unicode characters.
    /// The same function also works with temporary, standard, redirected, or custom text input streams.
    void printFileAsText(const el::TextInputStreamPtr &input) {
        const auto result = input->readAll(el::CpLength{1024U});
        if (result.hasData()) {
            // The stream decoder turns the file encoding into an Erbsland string.
            el::io::printLine("Text: "_el, result.data());
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Bytes: 4d 6f 74 69 76 3a 20 c3 96 6c
    Text: Motiv: Öl

.. erbsland-demo-end::

Open File Streams Through a Path
================================

Use :cpp:func:`Path::content() <erbsland::path::Path::content>` to open a stream.
It returns a lightweight :cpp:class:`PathContent <erbsland::path::PathContent>` facade with four stream-opening methods:

.. mermaid::

    classDiagram
        class Path {
            +content() PathContent
        }
        class PathContent {
            +openByteInputStream(PathReadDataOptions) ByteInputStreamPtr
            +openByteOutputStream(PathWriteDataOptions) ByteOutputStreamPtr
            +openTextInputStream(PathReadTextOptions) TextInputStreamPtr
            +openTextOutputStream(PathWriteTextOptions) TextOutputStreamPtr
        }
        Path --> PathContent : content()

``openByteInputStream()``
    Opens the file for exact byte input.
    Use it for binary formats or when decoding is handled by another component.

``openByteOutputStream()``
    Opens the file for exact byte output.
    The write options decide whether the file must be new, is replaced, or is opened for append.

``openTextInputStream()``
    Opens a byte file and adds Unicode decoding.
    Its options select UTF-8, UTF-16, or UTF-32, BOM behavior, and recovery from invalid input.

``openTextOutputStream()``
    Opens a byte file and adds Unicode encoding.
    Callers write characters and strings; the stream produces the configured encoded representation.

All four methods return a shared pointer to an already open stream.
The result combines the native file handle, buffering, optional text encoding, positioning when supported, and a
path-aware error source.
Opening failures such as a missing file or rejected access throw :cpp:class:`PathError <erbsland::path::PathError>`;
failures after the stream is open surface as :cpp:class:`StreamError <erbsland::stream::StreamError>`.

For small files that must be handled as one value, ``PathContent`` also provides ``readData()``, ``readText()``,
``writeData()``, and ``writeText()`` plus throwing ``OrThrow`` variants.
These are whole-file convenience operations, not stream-opening methods.
Use a stream when data can be large, work must be incremental, or timeout and readiness states must remain visible.

Why Explicit Close Matters
--------------------------

A successful output write means that the complete request was accepted, but bytes may still be queued.
:cpp:func:`OutputStream::close() <erbsland::stream::OutputStream::close>` gracefully drains output, closes the native
file, and gives the application a chance to observe timeout or failure.
If close times out, the stream remains in ``Closing`` and the owner can call it again.

Destroying the last shared owner does **not** perform a potentially blocking graceful close.
Destruction follows the immediate abort path so it is safe during stack unwinding and shutdown; queued output may be
abandoned and no close failure can be reported to the caller.
Therefore, every owned output file should have an explicit close point on its successful path.

Input streams have no queued application output, so destruction can safely abort them.
Closing input explicitly is still useful: it stops pending work, releases the native handle at a known point, and makes
it clear that no later component may continue reading through another shared alias.

.. erbsland-demo::
    :source: stream/CreatingStreams/OpenFileStreams.cpp
    :exec: stream/creating_streams --demo OpenFileStreams
    :source-sha256: 07f7bef7df973017f217f4bf526145e4acc1efa6468037cc31184ec3c4b486a0

.. code-block:: cpp

    /// Open file-backed streams through `Path::content()`.
    /// The path factory returns a shared stream with the platform handle, buffering, encoding, and error context already
    /// connected. Always finish owned output with `close()` so queued data reaches the file.
    void openFileStreams() {
        const auto directory = createStreamDemoDirectory("skizzen"_el);
        const auto path = directory->path() / "studie.txt"_el;

        // Write a small UTF-8 sketchbook entry.
        const auto output = path.content().openTextOutputStream();
        output->writeLine("Studie 17: Licht auf dem Nordhang"_el);
        output->close();

        // Open the same path for decoded text input.
        const auto input = path.content().openTextInputStream();
        const auto result = input->readAll();
        if (result.hasData()) {
            el::io::print(result.data());
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Studie 17: Licht auf dem Nordhang

.. erbsland-demo-end::

Configure Before Opening
========================

Opening is the point where filesystem policy, text encoding, time bounds, and memory limits become fixed.
The defaults are useful for ordinary UTF-8 files, but configure explicitly when an existing file may be replaced or
appended, parent directories may be absent, content is sensitive, another encoding is required, or the stream must meet
specific latency and memory bounds.

Path Options
------------

The four option classes mirror the four opening methods.
The diagram lists their configurable properties; each property has a matching getter and chainable ``set...()`` method.

.. mermaid::

    classDiagram
        class PathReadDataOptions {
            +maximumByteLength
            +timeout
            +streamSettings
        }
        class PathReadTextOptions {
            +encoding
            +bomMode
            +encodingErrorMode
            +maximumByteLength
            +maximumCpLength
            +timeout
            +streamSettings
        }
        class PathWriteDataOptions {
            +createParents
            +creationMode
            +accessProfile
            +timeout
            +streamSettings
        }
        class PathWriteTextOptions {
            +createParents
            +creationMode
            +accessProfile
            +encoding
            +bomMode
            +encodingErrorMode
            +timeout
            +streamSettings
        }

``createParents``
    Creates missing parent directories before opening an output file.
    Leave it disabled when a missing directory should reveal a configuration or spelling error.

``creationMode``
    :cpp:enumerator:`PathCreateMode::CreateNew <erbsland::path::PathCreateMode::CreateNew>` is the safe default and fails
    if the path already exists.
    ``CreateOrOverwrite`` replaces existing content, while ``CreateOrAppend`` preserves it and writes at the end.
    Append streams are deliberately not positionable.

``accessProfile``
    Selects a portable access intent for newly created files: platform default, current user only, user and group, or
    everyone.
    It does not rewrite permissions of a file that already exists.

``encoding``
    Selects UTF-8, UTF-16, or UTF-32 and, for the latter two, automatic or explicit little/big-endian byte order.
    Byte stream options do not have an encoding because they preserve the exact representation.

``bomMode``
    Controls whether a byte-order mark is accepted or emitted automatically, required, or rejected.
    This is especially important when generic UTF-16 or UTF-32 must discover its byte order.

``encodingErrorMode``
    Chooses whether invalid input or unrepresentable output throws, is ignored when lossy recovery is possible, or is
    replaced with the Unicode replacement character.

``maximumByteLength`` and ``maximumCpLength``
    Protect the whole-file ``readData...`` and ``readText...`` convenience methods from unbounded allocation.
    They are intentionally ignored by ``open...InputStream()`` because stream callers control their own incremental
    limits.

``timeout`` and ``streamSettings``
    ``setTimeout()`` is the concise way to change only the per-operation wait.
    ``setStreamSettings()`` supplies the complete input or output settings object described below.

Stream Settings
---------------

Path options contain one immutable settings object that is copied into the new stream.
After opening, every shared user sees the same time and memory policy.

.. mermaid::

    classDiagram
        class InputStreamSettings {
            +timeout
            +bufferCapacity
        }
        class OutputStreamSettings {
            +timeout
            +bufferCapacity
            +backBufferLimit
        }

``timeout``
    Sets the maximum wait for one public operation.
    It is not a deadline for the whole file: a loop may make progress through many individually bounded operations.

``bufferCapacity``
    Sets the fixed capacity used for native read-ahead or the output front buffer.
    Larger buffers can reduce native calls, while smaller buffers reduce the stream's fixed memory footprint.

``backBufferLimit``
    Sets the hard limit for queued output beyond the fixed front buffer.
    It bounds memory and determines the largest atomic request the stream can accept under back pressure.

The following example combines filesystem options, UTF-16 encoding, BOM policy, and output buffering before opening the
file.

.. erbsland-demo::
    :source: stream/CreatingStreams/ConfigureAtCreation.cpp
    :exec: stream/creating_streams --demo ConfigureAtCreation
    :source-sha256: 43ed48965ccbacf5c3476fd876994923e1be8a3405169cb757aeeae37571d220

.. code-block:: cpp

    /// Configure a stream before opening it.
    /// File options combine filesystem policy with immutable stream settings, so every user of the resulting stream sees
    /// the same timeout, buffering, creation mode, encoding, and byte-order-mark behavior.
    void configureAtCreation() {
        auto streamSettings = el::OutputStreamSettings{};
        streamSettings.setTimeout(el::TimeDelta::milliseconds(250))
            .setBufferCapacity(el::ByteLength{16U * 1024U})
            .setBackBufferLimit(el::ByteLength{128U * 1024U});

        auto options = el::PathWriteTextOptions{el::StringEncoding::Utf16LittleEndian};
        options.setCreationMode(el::PathCreateMode::CreateOrOverwrite)
            .setBomMode(el::StringBomMode::Require)
            .setStreamSettings(streamSettings);

        const auto directory = createStreamDemoDirectory("konfiguration"_el);
        const auto output = (directory->path() / "farbenlehre.txt"_el).content().openTextOutputStream(options);
        output->writeLine("Ultramarin neben warmem Ocker"_el);
        output->close();

        el::io::printLine("Encoding configured: UTF-16 LE"_el);
        el::io::printLine("Back-buffer limit: "_el, streamSettings.backBufferLimit().toSizeT(), " bytes"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Encoding configured: UTF-16 LE
    Back-buffer limit: 131072 bytes

.. erbsland-demo-end::

Design Functions Around Stream Interfaces
=========================================

A function that consumes or produces data should normally depend on a stream interface, not on the path, temporary file,
terminal, or string builder behind it.
This makes the same logic reusable in production, tests, command-line tools, and in-memory capture.

Choose the narrowest abstraction that provides every operation the function uses:

*   Start with direction: input or output.
*   Choose bytes when the representation matters and text when the function works with Unicode characters or lines.
*   Use the generic ``InputStream`` or ``OutputStream`` base only for code that deals solely with lifecycle, settings, or
    readiness and does not read or write content.
*   Do not accept a ``Path`` or concrete stream implementation unless the function truly needs path-specific behavior or
    must open the stream itself.

For a synchronous helper, pass the shared pointer alias by const reference.
If a component stores the stream or starts work that outlives the call, store a pointer copy so ownership is explicit.
Ordinary helpers should not close or abort a borrowed stream: the owner of the complete operation should do that after
all producers or consumers have finished.

.. erbsland-demo::
    :source: stream/CreatingStreams/ShareStreamWithWriter.cpp
    :exec: stream/creating_streams --demo ShareStreamWithWriter
    :source-sha256: d79bc2e4141e125a4190dba878743e132aa3863dbde2874cddd0ed7e1690ca43

.. code-block:: cpp

    /// Pass shared stream interfaces to components that produce or consume data.
    /// Library-created streams use shared ownership so decorators and coroutine operations can safely keep their backing
    /// stream alive. The caller that owns the complete operation remains responsible for flushing or closing the stream.
    void shareStreamWithWriter() {
        const auto output = el::StringBuilderStream::create();

        // The helper depends only on text output, not on a particular destination.
        writeStudyLabel(output, "Birken im Morgennebel"_el);
        writeStudyLabel(output, "Felsen nach dem Regen"_el);

        el::io::print(output->takeString());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Motiv: Birken im Morgennebel
    Motiv: Felsen nach dem Regen

.. erbsland-demo-end::

Manage Shared Ownership Deliberately
====================================

Library factories return ``std::shared_ptr`` aliases such as ``ByteInputStreamPtr`` and ``TextOutputStreamPtr``.
Copying one of these pointers does not copy the file or create a second cursor: every alias refers to the same stream,
state, position, settings, and queued data.
The native resource remains alive until the last shared owner releases it.

Shared ownership solves lifetime, not coordination.
Callers still need a clear operation owner, a rule for which component may read or write, and one place that performs
the final close.
Concurrent producers must also inspect every result; a pointer copy does not make a sequence of operations atomic.

The standard stream proxies are process-wide objects with additional rules, and coroutine operations retain ownership
while suspended.
Those cases are covered in :doc:`standard_streams` and :doc:`coroutine_streams`.

Where to Continue
=================

With creation and ownership established, choose the guide that matches the data boundary:

*   :doc:`byte_input_and_output` for binary records, bounded block loops, exact reads, atomic writes, and endian-aware
    integers.
*   :doc:`text_input_and_output` for Unicode blocks, lines, formatting, output capture, encodings, BOMs, and encoding
    errors.

Then read :doc:`readiness_and_timeouts` and :doc:`lifecycle` before building a long-running operation or a shutdown path
that must preserve output reliably.
