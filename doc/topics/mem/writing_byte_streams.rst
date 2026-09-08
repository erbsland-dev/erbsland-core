.. index::
    single: Memory; Writing byte streams
    single: Binary data; Serialization
    single: ByteWriter

********************
Writing Byte Streams
********************

When a binary format contains several consecutive fields, :cpp:class:`ByteWriter <erbsland::mem::ByteWriter>` provides
one place to keep the growing byte block, current position, and integer byte order.
This page shows how to assemble records, revisit placeholders, manage the writer's storage, and encode bytes, integers,
and text.

A writer is designed for serialization: building a new structured record from values in your application.
For a few simple appends or local edits to existing data, :cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>`
is more direct.
Creating a stateful writer merely to encode one integer is also unnecessary because byte containers provide
``setInteger`` methods for that case.

Building One Record Together
============================

A local writer works well for a compact serializer whose fields fit naturally in one function.
As the format grows, pass the writer by reference to functions responsible for individual sections.
They all append to the same block and share the selected byte order, while the caller retains ownership of the complete
serialization operation.

:cpp:func:`ByteWriter::toByteBlock() <erbsland::mem::ByteWriter::toByteBlock>` returns the written data as a read-only,
shared block.
This is usually the final step before handing the record to a stream, socket, or storage layer.

.. erbsland-demo::
    :source: mem/ByteStreams/BuildRecord.cpp
    :exec: mem/byte_streams --demo BuildRecord
    :source-sha256: 68e67b9467cc63bfdbb3d1be570b67c071e797022a5a109142abab92aa1cd51d

.. code-block:: cpp

    /// Add the fields of a nocturnal wildlife observation to a byte stream.
    ///
    /// Passing a `ByteWriter` by reference lets several serialization functions
    /// contribute to one record while sharing its position and byte order.
    void writeObservation(el::ByteWriter &writer, const uint16_t count, const uint8_t confidence) {
        writer.writeUInt16(count).writeUInt8(confidence);
    }

    /// Build a short binary record with a local byte writer.
    ///
    /// The local writer owns the overall operation. Helpers receive it by reference
    /// when a format is divided into logical sections.
    void buildRecord() {
        auto writer = el::ByteWriter{};
        writer.setEndianness(el::Endianness::Big);

        // Write the record header locally, then delegate the observation fields.
        writer.writeByte(el::Byte{0x4eU});
        writeObservation(writer, 12U, 94U);
        const auto record = writer.toByteBlock();

        el::io::printLine("Waarneming        : Bosuil"_el);
        el::io::printLine("Encoded record    : "_el, el::ByteFormat::separated(), record);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Waarneming        : Bosuil
    Encoded record    : 4e 00 0c 5e

.. erbsland-demo-end::

Revisiting a Field
==================

:cpp:func:`ByteWriter::position() <erbsland::mem::ByteWriter::position>` reports where the next write begins and
:cpp:func:`ByteWriter::length() <erbsland::mem::ByteWriter::length>` reports the complete constructed size.
They normally advance together as bytes are appended.

Some formats place a checksum or count before the data needed to calculate it.
You can first write a correctly sized placeholder, append the body, then use
:cpp:func:`ByteWriter::setPosition() <erbsland::mem::ByteWriter::setPosition>` to return and overwrite that field.
Setting the position beyond the current length clamps it to the end; the writer does not create an implicit gap.
When you return to the end, subsequent writes append as usual.

.. erbsland-demo::
    :source: mem/ByteStreams/PositionWriter.cpp
    :exec: mem/byte_streams --demo PositionWriter
    :source-sha256: 983c0f0beb45e511ad591356f67958954f9c3c668d6f4b392eba4b875873c388

.. code-block:: cpp

    /// Revisit fields while assembling a byte stream.
    ///
    /// Writing at an existing position overwrites bytes. Setting a position beyond
    /// the current length clamps it to the end, where subsequent writes append.
    void positionWriter() {
        auto writer = el::ByteWriter{};

        // Leave a placeholder count, append events, then return to fill the count.
        writer.writeByte(el::Byte{}).writeByte(el::Byte{0x31U}).writeByte(el::Byte{0x32U});
        writer.setPosition(el::ByteIndex{0U});
        writer.writeByte(el::Byte{2U});
        writer.setPosition(el::ByteIndex{100U});
        writer.writeByte(el::Byte{0xffU});

        el::io::printLine("Final position     : "_el, writer.position().toSizeT());
        el::io::printLine("Record length      : "_el, writer.length().toSizeT());
        el::io::printLine("Encoded record     : "_el, el::ByteFormat::separated(), writer.toByteBlock());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Final position     : 4
    Record length      : 4
    Encoded record     : 02 31 32 ff

.. erbsland-demo-end::

Managing Allocation and Reuse
=============================

If you know a useful upper bound for a record, one call to
:cpp:func:`ByteWriter::reserve() <erbsland::mem::ByteWriter::reserve>` before writing can avoid growth allocations.
Repeatedly reserving before each field obscures the serialization and usually signals that the capacity estimate belongs
at a higher level.
Reservation changes capacity, not length or position.

:cpp:func:`ByteWriter::takeByteBlockEditor() <erbsland::mem::ByteWriter::takeByteBlockEditor>` transfers the constructed
data when the caller needs an editable result.
The writer is empty and positioned at the beginning afterward.
:cpp:func:`ByteWriter::reset() <erbsland::mem::ByteWriter::reset>` instead discards the constructed bytes and returns the
writer to its initial position for reuse.

.. erbsland-demo::
    :source: mem/ByteStreams/StorageManagement.cpp
    :exec: mem/byte_streams --demo StorageManagement
    :source-sha256: 72f989da0be83dd4c157547d3c7f7f3737e9d63600da7474e5f58a417334af53

.. code-block:: cpp

    /// Prepare, transfer, and reuse a byte writer's storage.
    ///
    /// Reserve once when a useful upper bound is known. `takeByteBlockEditor()`
    /// transfers editable output, and `reset()` discards bytes before reuse.
    void storageManagement() {
        auto writer = el::ByteWriter{};

        // Reserve once for the expected record, then transfer the finished editor.
        writer.reserve(el::ByteLength{64U});
        writer.writeByte(el::Byte{0x4eU}).writeByte(el::Byte{0x01U});
        auto editableRecord = writer.takeByteBlockEditor();

        // Reuse the empty writer for another record and explicitly discard it.
        writer.writeByte(el::Byte{0x4eU});
        writer.reset();
        el::io::printLine("Transferred bytes : "_el, editableRecord.length().toSizeT());
        el::io::printLine("Writer is empty   : "_el, el::BooleanFormat::yesNo(), writer.length().isZero());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Transferred bytes : 2
    Writer is empty   : yes

.. erbsland-demo-end::

Writing Bytes and Existing Blocks
=================================

:cpp:func:`ByteWriter::writeByte() <erbsland::mem::ByteWriter::writeByte>` writes one
:cpp:class:`Byte <erbsland::mem::Byte>`.
:cpp:func:`ByteWriter::writeBytes() <erbsland::mem::ByteWriter::writeBytes>` accepts an Erbsland byte span, a
``std::span<const std::byte>``, or a :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>`.
At the end these operations append; at an earlier position they overwrite existing bytes and extend the block only when
the new range reaches beyond its old end.

All write operations return the writer by reference.
Chaining adjacent fields keeps their wire order visible, but splitting a long record into a few meaningful groups is
often easier to review than one enormous expression.

.. erbsland-demo::
    :source: mem/ByteStreams/WriteBytes.cpp
    :exec: mem/byte_streams --demo WriteBytes
    :source-sha256: e263720bd243388d2e2cc51634a058b130604aba62f1dd0d2d7b5e30cef77f29

.. code-block:: cpp

    /// Append individual bytes and existing byte sequences to a stream.
    ///
    /// `writeByte()` and `writeBytes()` advance the position and return the writer,
    /// so adjacent fields can be expressed as one readable chain.
    void writeBytes() {
        const auto sensorPayload = el::ByteBlock{el::Byte{0xa1U}, el::Byte{0xb2U}, el::Byte{0xc3U}};
        auto writer = el::ByteWriter{};

        // Write a marker, a byte count, and the already assembled payload.
        writer.writeByte(el::Byte{0x4eU})
            .writeByte(el::Byte{static_cast<uint8_t>(sensorPayload.length().toSizeT())})
            .writeBytes(sensorPayload);

        el::io::printLine("Nachtmeting       : Vleermuis"_el);
        el::io::printLine("Encoded record    : "_el, el::ByteFormat::separated(), writer.toByteBlock());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Nachtmeting       : Vleermuis
    Encoded record    : 4e 03 a1 b2 c3

.. erbsland-demo-end::

Writing Integers
================

The native helpers from :cpp:func:`ByteWriter::writeInt8() <erbsland::mem::ByteWriter::writeInt8>` through
:cpp:func:`ByteWriter::writeUInt64() <erbsland::mem::ByteWriter::writeUInt64>` make the signedness and width explicit in
the call.
The templated :cpp:func:`ByteWriter::writeInteger() <erbsland::mem::ByteWriter::writeInteger>` derives those properties
from its fixed-width C++ argument type.

Multi-byte native values use the writer's :cpp:enum:`Endianness <erbsland::mem::Endianness>`.
The default is little endian, and :cpp:func:`ByteWriter::setEndianness() <erbsland::mem::ByteWriter::setEndianness>`
changes it for following fields.
Set this once near the start of a serializer unless the format itself changes byte order between fields.

:cpp:func:`ByteWriter::writeIntegerOrThrow() <erbsland::mem::ByteWriter::writeIntegerOrThrow>` accepts an explicit
:cpp:class:`ByteIntegerFormat <erbsland::mem::ByteIntegerFormat>` for widths such as 24 bits and for compact
variable-length encodings.
It reports a value that cannot be represented rather than silently narrowing it.
The complete format selection guide is in :doc:`byte_integer_formats`.

.. erbsland-demo::
    :source: mem/ByteStreams/WriteIntegers.cpp
    :exec: mem/byte_streams --demo WriteIntegers
    :source-sha256: 2738cc9ea4ba9ec74488ca0a363959773728bded3c1f9fc472f89a74a7adb9cb

.. code-block:: cpp

    /// Write native-width and explicitly formatted integers to a byte stream.
    ///
    /// Native helpers make the field width visible in the call. Explicit formats
    /// cover compact or non-standard wire representations and reject values that do
    /// not fit.
    void writeIntegers() {
        auto writer = el::ByteWriter{};
        writer.setEndianness(el::Endianness::Big);

        // Store a 16-bit count, a compact duration, and an 8-bit confidence value.
        writer.writeUInt16(12U)
            .writeIntegerOrThrow<uint32_t>(300U, el::ByteIntegerFormat::UnsignedVariableLength)
            .writeUInt8(94U);

        el::io::printLine("Integer byte order: Big endian"_el);
        el::io::printLine("Encoded fields    : "_el, el::ByteFormat::separated(), writer.toByteBlock());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Integer byte order: Big endian
    Encoded fields    : 00 0c 81 2c 5e

.. erbsland-demo-end::

Encoding Text Fields
====================

:cpp:func:`ByteWriter::writeText() <erbsland::mem::ByteWriter::writeText>` and
:cpp:func:`ByteWriter::writeTextOrThrow() <erbsland::mem::ByteWriter::writeTextOrThrow>` combine character encoding with
the framing described by :cpp:class:`ByteTextOptions <erbsland::mem::ByteTextOptions>`.
The default produces length-prefixed UTF-8; the compact factory uses a variable-length count.
The reader must use matching options to find the field boundary and decode its code units.

The tolerant method truncates only at a character boundary when a configured maximum requires it.
This is useful for deliberately bounded display fields.
The strict method rejects text that cannot fit the chosen framing and is the safer default when truncation would change
the meaning of a name, identifier, or protocol value.
See :doc:`text_byte_stream_options` for byte layouts, format selection, and security considerations.

.. erbsland-demo::
    :source: mem/ByteStreams/WriteText.cpp
    :exec: mem/byte_streams --demo WriteText
    :source-sha256: 7c8c16aeb3498085d98bb5cb3c3e00f4a65c0b9da0bd9ac7f6d5419febbc0646

.. code-block:: cpp

    /// Encode framed text into a byte stream.
    ///
    /// The default options write a length-prefixed UTF-8 field. `writeText()` may
    /// truncate at a character boundary when a configured limit requires it, while
    /// `writeTextOrThrow()` requires an exact representation.
    void writeText() {
        auto options = el::ByteTextOptions::compact();
        options.setLength(el::ByteLength{16U});
        auto writer = el::ByteWriter{};

        // Encode two compact UTF-8 fields using identical framing.
        writer.writeTextOrThrow("bosuil"_el, options).writeTextOrThrow("maanlicht"_el, options);
        const auto record = writer.toByteBlock();

        // Read the fields back with the same options.
        auto reader = el::ByteReader{record};
        el::io::printLine("Soort             : "_el, reader.readTextOrThrow(options));
        el::io::printLine("Omgeving          : "_el, reader.readTextOrThrow(options));
        el::io::printLine("Encoded fields    : "_el, el::ByteFormat::separated(), record);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Soort             : bosuil
    Omgeving          : maanlicht
    Encoded fields    : 06 62 6f 73 75 69 6c 09 6d 61 61 6e 6c 69 63 68 74

.. erbsland-demo-end::

Choosing the Smallest Suitable Abstraction
==========================================

Use a writer when multiple typed fields form one sequential binary record or several serializer functions must share
progress.
For one integer, write it directly into a :cpp:class:`ByteArray <erbsland::mem::ByteArray>`,
:cpp:class:`ByteBuffer <erbsland::mem::ByteBuffer>`, or :cpp:class:`ByteBlockEditor
<erbsland::mem::ByteBlockEditor>` with ``setInteger``.
For simple byte assembly and edits, stay with ``ByteBlockEditor``.
To parse the finished result, :doc:`reading_byte_streams` explains the matching
:cpp:class:`ByteReader <erbsland::mem::ByteReader>` workflow.
