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

Check the Capability
====================

Call :cpp:func:`supportsPositioning() <erbsland::stream::StreamPositioning::supportsPositioning>` before any position
operation.
Regular files normally support it.
Pipes, terminals, standard proxies, string builders, and append-only output do not promise random access.
Calling a positioning method when unsupported throws ``StreamError``.

.. erbsland-demo::
    :source: stream/Positioning/CheckPositioning.cpp
    :exec: stream/positioning --demo CheckPositioning
    :source-sha256: cfaa50f6437c79e608e0e2ab06ccd5710d9131b86444120378a21f2463b13758

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

.. erbsland-ansi::
    :escape-char: ␛

    Regular file supports positioning: true
    Append stream supports positioning: false

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
    :source-sha256: 4ca0e4a5c5a812ab525a7f05f892a599fc03df361d952dc71f981b0f1d3c81c4

.. code-block:: cpp

    /// Set an absolute encoded-byte position before reading a fixed record.
    /// Successful input positioning discards read-ahead and retained aggregate state, so the next read starts cleanly.
    void readRecordAtOffset() {
        const auto directory = createStreamDemoDirectory("记录"_el);
        const auto path = directory->path() / "animals.bin"_el;
        path.content().writeDataOrThrow(el::ByteBlock{std::vector<uint8_t>{10U, 11U, 12U, 20U, 21U, 22U}});
        const auto input = path.content().openByteInputStream();

        input->setPosition(el::ByteIndex{3U});
        const auto record = input->readExact(el::ByteLength{3U});
        el::io::printLine("First byte of the second record: "_el, record.data().get(el::ByteIndex{0U}).toUInt8());
    }

.. erbsland-ansi::
    :escape-char: ␛

    First byte of the second record: 20

.. erbsland-demo-end::

Move Relative to a Reference Point
==================================

Use ``setPosition()`` for an absolute byte index.
Use ``movePosition()`` with :cpp:enum:`StreamPositionOrigin <erbsland::stream::StreamPositionOrigin>` to move relative
to the start, logical current position, or current end.
Negative, overflowing, invalid, or non-native positions throw ``ParameterError``.

.. erbsland-demo::
    :source: stream/Positioning/MoveRelativeToEnd.cpp
    :exec: stream/positioning --demo MoveRelativeToEnd
    :source-sha256: bf04f50b6f92e3cbaa93636ef756b51262dfcfefedab394f5c1409e9b35386a6

.. code-block:: cpp

    /// Move relative to the start, logical current position, or current end.
    /// Relative-to-end movement is useful for fixed trailers without first querying the native file length.
    void moveRelativeToEnd() {
        const auto directory = createStreamDemoDirectory("尾部"_el);
        const auto path = directory->path() / "tracks.bin"_el;
        path.content().writeDataOrThrow(el::ByteBlock{std::vector<uint8_t>{1U, 2U, 3U, 4U, 90U, 91U}});
        const auto input = path.content().openByteInputStream();

        input->movePosition(el::StreamPositionOrigin::End, el::ByteOffset{-2});
        const auto trailer = input->readExact(el::ByteLength{2U});
        el::io::printLine("Trailer marker: "_el, trailer.data().get(el::ByteIndex{0U}).toUInt8(), " / "_el,
            trailer.data().get(el::ByteIndex{1U}).toUInt8());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Trailer marker: 90 / 91

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
    :source-sha256: d07478ea1f305bf2455afe7d09a594ed2a5b8bef3a9031aa1663c3c6616e7d7a

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

.. erbsland-ansi::
    :escape-char: ␛

    Logical position after acceptance: 9
    File length: 9

.. erbsland-demo-end::

Position Encoded Text Carefully
===============================

Text positions count original encoded bytes, including a consumed or written BOM.
They must land on a code-point boundary; a misaligned next read is handled by the configured encoding-error mode.
Generic UTF-16 and UTF-32 must resolve byte order at byte zero before moving elsewhere, so explicit endian encodings are
preferable for immediate random access.

Returning to zero enables normal BOM detection or emission.
Moving to a nonzero location never consumes or writes an interior BOM.

.. erbsland-demo::
    :source: stream/Positioning/PositionEncodedText.cpp
    :exec: stream/positioning --demo PositionEncodedText
    :source-sha256: 7138c654ed23776657d4fb7fea6c752243689a666bf6c07ab68276f6c0e56a4a

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

.. erbsland-ansi::
    :escape-char: ␛

    Second character: 狐

.. erbsland-demo-end::

Retry Positioning After Timeout
===============================

Position changes are bounded and can time out while waiting to serialize with a read or drain queued output.
A timeout leaves the logical position unchanged.
Retry the complete request rather than adjusting from an assumed partial movement.

.. erbsland-demo::
    :source: stream/Positioning/HandlePositionTimeout.cpp
    :exec: stream/positioning --demo HandlePositionTimeout
    :source-sha256: 37ad4fca677c3d6c85ad9a4650b3449719f342ea17ecabc3b9928eb6e8a74cdb

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

.. erbsland-ansi::
    :escape-char: ␛

    First positioning attempt timed out: true
    Position is unchanged: true
    Position after retry: 12

.. erbsland-demo-end::
