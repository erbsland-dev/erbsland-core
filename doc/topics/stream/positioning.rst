..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Streams; Positioning
    single: Streams; Sparse Files
    single: Streams; Text Position

*******************
Positioning Streams
*******************

.. erbsland-draft::

Positioning provides optional random access in encoded bytes.
This page explains capability checks, logical positions, absolute and relative movement, sparse output, decoder resets,
text alignment, append mode, and timeout-safe changes.

Checking Stream Positioning Support
===================================

Call :cpp:func:`supportsPositioning() <erbsland::stream::StreamPositioning::supportsPositioning>` before any position
operation.
Regular files normally support it.
Pipes, terminals, standard proxies, string builders, and append-only output do not promise random access.
Calling a positioning method when unsupported throws ``StreamError``.

.. erbsland-demo::
    :source: stream/Positioning/CheckPositioning.cpp
    :exec: stream/positioning --demo CheckPositioning
    :sha256: 9155fe7c9f2710c9d1af6b32058846fb33e6ff6b5b13ad01f69caa104a3692bf

.. code-block:: cpp

    /// Check `supportsPositioning()` before using byte positions.
    /// Regular files normally support it, while append-only files, pipes, terminals, and standard-stream proxies do not.
    void checkPositioning() {
        const auto directory = createStreamDemoDirectory("野生动物"_el);
        const auto path = directory->path() / "observations.bin"_el;
        const auto regular = path.content().openByteOutputStream();
        el::io::printLine("Regular file supports positioning: "_el, regular->supportsPositioning());
        regular->close();

        auto appendOptions = el::PathWriteDataOptions{};
        appendOptions.setCreationMode(el::PathCreateMode::CreateOrAppend);
        const auto append = path.content().openByteOutputStream(appendOptions);
        el::io::printLine("Append stream supports positioning: "_el, append->supportsPositioning());
        append->close();
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mRegular file supports positioning: true
        Append stream supports positioning: false␛[0m

.. erbsland-demo-end::

Use Logical Byte Positions
==========================

``position()`` reports the logical encoded-byte position.
Native input read-ahead does not advance it, while bytes consumed into a retained exact or aggregate read do.
Accepted output advances the position immediately even when it remains queued for native delivery.

Successful input repositioning discards read-ahead, decoder state, retained logical input, and end-of-stream state.
The next read begins from the requested location.

.. erbsland-demo::
    :source: stream/Positioning/ReadRecordAtOffset.cpp
    :exec: stream/positioning --demo ReadRecordAtOffset
    :sha256: 15c452d006903bf39db3a24094f42d0675f6083746f15d1738b63caf590fac48

.. code-block:: cpp

    /// Set an absolute encoded-byte position before reading a fixed record.
    /// Successful input positioning discards read-ahead and retained aggregate state, so the next read starts cleanly.
    void readRecordAtOffset() {
        const auto directory = createStreamDemoDirectory("记录"_el);
        const auto path = directory->path() / "animals.bin"_el;
        path.content().writeDataOrThrow(el::ByteBlock({10U, 11U, 12U, 20U, 21U, 22U}));
        const auto input = path.content().openByteInputStream();

        input->setPosition(el::ByteIndex{3U});
        const auto record = input->readExact(el::ByteCount{3U});
        el::io::printLine("First byte of the second record: "_el, record.data().get(el::ByteIndex{0U}).toUInt8());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mFirst byte of the second record: 20␛[0m

.. erbsland-demo-end::

Move Relative to a Reference Point
==================================

Use ``setPosition()`` for an absolute byte index.
Use ``movePosition()`` with :cpp:enum:`StreamPositionOrigin <erbsland::stream::StreamPositionOrigin>` to move relative
to the start, logical current position, or current end.
Negative, overflowing, invalid, or non-native positions throw ``ParameterFailure``.

.. erbsland-demo::
    :source: stream/Positioning/MoveRelativeToEnd.cpp
    :exec: stream/positioning --demo MoveRelativeToEnd
    :sha256: 65ee59d35c516fb6c8a535ccb98d8c5cfa4b541d82949e47646802151ef2581e

.. code-block:: cpp

    /// Move relative to the start, logical current position, or current end.
    /// Relative-to-end movement is useful for fixed trailers without first querying the native file length.
    void moveRelativeToEnd() {
        const auto directory = createStreamDemoDirectory("尾部"_el);
        const auto path = directory->path() / "tracks.bin"_el;
        path.content().writeDataOrThrow(el::ByteBlock({1U, 2U, 3U, 4U, 90U, 91U}));
        const auto input = path.content().openByteInputStream();

        input->movePosition(el::StreamPositionOrigin::End, el::ByteOffset{-2});
        const auto trailer = input->readExact(el::ByteCount{2U});
        el::io::printLine(
            "Trailer marker: "_el,
            trailer.data().get(el::ByteIndex{0U}).toUInt8(),
            " / "_el,
            trailer.data().get(el::ByteIndex{1U}).toUInt8());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mTrailer marker: 90 / 91␛[0m

.. erbsland-demo-end::

Create Sparse Output Deliberately
=================================

Positions beyond the current end are allowed.
A later write can create a sparse file, subject to platform and filesystem behavior.
Output positioning waits for already accepted output to reach the native stream in order, but does not replace an
explicit flush durability boundary.

.. erbsland-demo::
    :source: stream/Positioning/CreateSparseRecord.cpp
    :exec: stream/positioning --demo CreateSparseRecord
    :sha256: dde72cb698830673e1a2e17426bb7f254bf7dc7948ed2b6391b4a5b32dfe9f37

.. code-block:: cpp

    /// Position beyond the current end before writing to create a sparse region where the platform supports it.
    /// The logical output position advances as soon as the complete write is accepted, even while bytes remain queued.
    void createSparseRecord() {
        const auto directory = createStreamDemoDirectory("稀疏"_el);
        const auto path = directory->path() / "habitat.bin"_el;
        const auto output = path.content().openByteOutputStream();
        output->setPosition(el::ByteIndex{8U});
        output->writeUInt8(42U);
        el::io::printLine("Logical position after acceptance: "_el, output->position().toSizeT());
        output->close();
        el::io::printLine("File length: "_el, path.content().readDataOrThrow().length().toSizeT());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mLogical position after acceptance: 9
        File length: 9␛[0m

.. erbsland-demo-end::

Position Encoded Text Carefully
===============================

Text positions count original encoded bytes, including a consumed or written BOM.
They must land on a code-point boundary; a misaligned next read is handled by the configured input encoding mode.
Generic UTF-16 and UTF-32 must resolve byte order at byte zero before moving elsewhere, so explicit endian encodings are
preferable for immediate random access.

Returning to zero enables normal BOM detection or emission.
Moving to a nonzero location never consumes or writes an interior BOM.

.. erbsland-demo::
    :source: stream/Positioning/PositionEncodedText.cpp
    :exec: stream/positioning --demo PositionEncodedText
    :sha256: b109a3aa3f962d4cea192e5f5ece7eae3b0011ab899ede05bb4217fa2287c2e2

.. code-block:: cpp

    /// Text positions count encoded bytes and must land on a code-point boundary.
    /// Explicit UTF-16 or UTF-32 byte order is best for immediate random access because no BOM must first resolve it.
    void positionEncodedText() {
        const auto directory = createStreamDemoDirectory("文本"_el);
        const auto path = directory->path() / "species.txt"_el;
        auto writeOptions = el::PathWriteTextOptions{el::StringEncoding::Utf16LittleEndian};
        writeOptions.setBomMode(el::StringBomMode::Reject);
        const auto output = path.content().openTextOutputStream(writeOptions);
        output->write("鹿狐熊"_el);
        output->close();

        auto readOptions = el::PathReadTextOptions{el::StringEncoding::Utf16LittleEndian};
        readOptions.setBomMode(el::StringBomMode::Reject);
        const auto input = path.content().openTextInputStream(readOptions);
        input->setPosition(el::ByteIndex{2U});
        el::io::printLine("Second character: "_el, input->readChar().data());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mSecond character: 狐␛[0m

.. erbsland-demo-end::

Retry Positioning After Timeout
===============================

Position changes are bounded and can time out while waiting to serialize with a read or drain queued output.
A timeout leaves the logical position unchanged.
Retry the complete request rather than adjusting from an assumed partial movement.

.. erbsland-demo::
    :source: stream/Positioning/HandlePositionTimeout.cpp
    :exec: stream/positioning --demo HandlePositionTimeout
    :sha256: 1251e444de87f040fb62b13a9b026366207364d6d6a788e252c4e72966587ab6

.. code-block:: cpp

    /// A positioning timeout leaves the logical position unchanged.
    /// Retry the complete positioning request after the stream becomes ready instead of guessing how far it moved.
    void handlePositionTimeout() {
        auto output = ScriptedByteOutputStream{};
        output.enablePositioning(1U);
        const auto before = output.position();
        const auto first = output.setPosition(el::ByteIndex{12U});
        el::io::printLine("First positioning attempt timed out: "_el, first.isTimeout());
        el::io::printLine("Position is unchanged: "_el, output.position() == before);
        output.setPosition(el::ByteIndex{12U});
        el::io::printLine("Position after retry: "_el, output.position().toSizeT());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mFirst positioning attempt timed out: true
        Position is unchanged: true
        Position after retry: 12␛[0m

.. erbsland-demo-end::
