.. index::
    single: Memory; Reading byte streams
    single: Binary data; Parsing
    single: ByteReader

********************************
Reading and Parsing Byte Streams
********************************

Binary records often contain several fields whose meaning depends on their order: a marker, a length, an identifier, and
perhaps some text.
:cpp:class:`ByteReader <erbsland::mem::ByteReader>` keeps a position in a :cpp:class:`ByteBlock
<erbsland::mem::ByteBlock>` and advances it as you decode these fields.
This page shows how to navigate a record, choose between tolerant and strict reads, handle integer byte order, and
decode framed text.

Choose a reader when the data has a sequential structure.
For one value at a known offset, direct access on the block is simpler and avoids maintaining a cursor.
Likewise, :cpp:func:`ByteBlock::forEach() <erbsland::mem::ByteBlock::forEach>` is a better fit when you merely want to
visit every byte, while :cpp:func:`ByteBlock::get() <erbsland::mem::ByteBlock::get>` handles an individual byte.

Building a Parser Around One Cursor
===================================

For a short record, create a local reader and decode fields in their wire order.
Larger parsers can pass the reader by reference to focused functions.
Those functions then contribute to one parse without returning or manually synchronizing an offset.
The shared cursor also makes variable-length fields natural: the next parser starts exactly where the previous one
finished.

A reader shares the underlying block data, so creating one does not copy the complete input.
It does add parsing state—principally the cursor and integer endianness—which is useful only when the record contains a
sequence of fields.

.. erbsland-demo::
    :source: mem/ByteStreams/ReadRecord.cpp
    :exec: mem/byte_streams --demo ReadRecord
    :source-sha256: 376ce21cf011bfa5ebcafe5af64ce703e560c9ea2b5eb3513c8c4211eed9fef2

.. code-block:: cpp

    /// Parse observation fields while sharing a byte reader's cursor.
    ///
    /// Passing a `ByteReader` by reference lets nested parsing functions consume
    /// their own fields from one record without synchronizing separate offsets.
    void readObservation(el::ByteReader &reader, uint16_t &count, uint8_t &confidence) {
        count = reader.readUInt16OrThrow();
        confidence = reader.readUInt8OrThrow();
    }

    /// Parse a short binary record with a local byte reader.
    ///
    /// The local reader owns the overall operation. Helpers receive it by reference
    /// when a format is divided into logical sections.
    void readRecord() {
        const auto record = el::ByteBlock{el::Byte{0x4eU}, el::Byte{0x00U}, el::Byte{0x0cU}, el::Byte{94U}};
        auto reader = el::ByteReader{record};
        reader.setEndianness(el::Endianness::Big);

        // Parse the header locally, then delegate the observation fields.
        const auto marker = reader.readByteOrThrow();
        auto count = uint16_t{};
        auto confidence = uint8_t{};
        readObservation(reader, count, confidence);

        el::io::printLine("Waarneming        : Bosuil"_el);
        el::io::printLine("Marker            : "_el, marker.toUInt32());
        el::io::printLine("Calls / confidence: "_el, count, " / "_el, confidence);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Waarneming        : Bosuil
    Marker            : 78
    Calls / confidence: 12 / 94

.. erbsland-demo-end::

Navigating the Input
====================

:cpp:func:`ByteReader::length() <erbsland::mem::ByteReader::length>` describes the complete input and
:cpp:func:`ByteReader::position() <erbsland::mem::ByteReader::position>` reports the next byte to read.
Before interpreting a fixed-size header, :cpp:func:`ByteReader::canRead() <erbsland::mem::ByteReader::canRead>` lets you
check that the entire header is available rather than discovering a partial record field by field.

:cpp:func:`ByteReader::advance() <erbsland::mem::ByteReader::advance>` skips bytes and
:cpp:func:`ByteReader::setPosition() <erbsland::mem::ByteReader::setPosition>` supports an absolute jump or a deliberate
rewind.
Both clamp the cursor to the end of the input, so an excessive position is safe and
:cpp:func:`ByteReader::isAtEnd() <erbsland::mem::ByteReader::isAtEnd>` becomes true.
Clamping is a boundary guarantee, not validation of a file offset; validate offsets from untrusted input before using
them when an out-of-range value must be reported as malformed data.

.. erbsland-demo::
    :source: mem/ByteStreams/NavigateReader.cpp
    :exec: mem/byte_streams --demo NavigateReader
    :source-sha256: 499bbfed1d0b459240b5877747d5e613bc3401f23dd6d9b25ff05507a190f30b

.. code-block:: cpp

    /// Navigate a byte stream without reading past its boundary.
    ///
    /// `position()` and `length()` describe the cursor and input size. `canRead()`
    /// validates a prospective read, while `advance()` and `setPosition()` move the
    /// cursor and clamp it to the valid range.
    void navigateReader() {
        const auto record = el::ByteBlock{el::Byte{0x4eU}, el::Byte{0x00U}, el::Byte{0x0cU}, el::Byte{94U}};
        auto reader = el::ByteReader{record};

        // Skip the marker after verifying that the complete header is present.
        const auto hasHeader = reader.canRead(el::ByteLength{3U});
        reader.advance(el::ByteLength{1U});
        const auto fieldPosition = reader.position();

        // A position beyond the input is safely clamped to its end.
        reader.setPosition(el::ByteIndex{100U});
        el::io::printLine("Header available  : "_el, el::BooleanFormat::yesNo(), hasHeader);
        el::io::printLine("Field position    : "_el, fieldPosition.toSizeT());
        el::io::printLine("Input length      : "_el, reader.length().toSizeT());
        el::io::printLine("At end            : "_el, el::BooleanFormat::yesNo(), reader.isAtEnd());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Header available  : yes
    Field position    : 1
    Input length      : 4
    At end            : yes

.. erbsland-demo-end::

Reading Bytes and Byte Blocks
=============================

Peeking is useful for discriminators and optional fields because it leaves the cursor unchanged.
:cpp:func:`ByteReader::peekByte() <erbsland::mem::ByteReader::peekByte>` returns zero at the end by default and also has
overloads for an absolute index, a relative offset, and a chosen fallback.
:cpp:func:`ByteReader::peekByteOrThrow() <erbsland::mem::ByteReader::peekByteOrThrow>` instead expresses that a byte is
required.

:cpp:func:`ByteReader::readByte() <erbsland::mem::ByteReader::readByte>` consumes one byte and returns zero at the end;
:cpp:func:`ByteReader::readByteOrThrow() <erbsland::mem::ByteReader::readByteOrThrow>` reports missing input.
For a complete region, :cpp:func:`ByteReader::readBytes() <erbsland::mem::ByteReader::readBytes>` returns an optional
block and leaves the cursor unchanged when the requested region is unavailable.
:cpp:func:`ByteReader::readBytesOrThrow() <erbsland::mem::ByteReader::readBytesOrThrow>` is the strict counterpart.

This distinction lets the surrounding parser communicate intent.
Use an optional result when incomplete input is an expected control path—for example while waiting for another network
fragment—and a strict read after the enclosing record length has already promised that the field exists.

.. erbsland-demo::
    :source: mem/ByteStreams/ReadBytes.cpp
    :exec: mem/byte_streams --demo ReadBytes
    :source-sha256: b575029b436e3decc52dca6c2a91a2a47f069a2c059eae4d5665e952052213c2

.. code-block:: cpp

    /// Inspect and consume individual bytes and complete byte ranges.
    ///
    /// Peek operations do not move the reader. Tolerant reads return a fallback or
    /// no value when data is unavailable; `OrThrow` reads express required fields.
    void readBytes() {
        const auto record =
            el::ByteBlock{el::Byte{0x4eU}, el::Byte{0x03U}, el::Byte{0xa1U}, el::Byte{0xb2U}, el::Byte{0xc3U}};
        auto reader = el::ByteReader{record};

        // Inspect the record marker, then consume the marker and payload length.
        const auto marker = reader.peekByteOrThrow();
        reader.advance(1U);
        const auto payloadLength = reader.readByteOrThrow().toUInt8();

        // Read the complete payload atomically.
        const auto payload = reader.readBytes(el::ByteLength{payloadLength});
        const auto extra = reader.readBytes(el::ByteLength{1U});
        el::io::printLine("Marker            : "_el, marker.toUInt32());
        el::io::printLine("Payload           : "_el, el::ByteFormat::separated(), *payload);
        el::io::printLine("Extra byte        : "_el, el::BooleanFormat::yesNo(), extra.has_value());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Marker            : 78
    Payload           : a1 b2 c3
    Extra byte        : no

.. erbsland-demo-end::

Reading Integers
================

The native-width helpers—:cpp:func:`ByteReader::readUInt16OrThrow() <erbsland::mem::ByteReader::readUInt16OrThrow>` and
its signed, unsigned, tolerant, and other-width companions—consume the number of bytes named by their result type.
Multi-byte values use the reader's :cpp:enum:`Endianness <erbsland::mem::Endianness>`.
The default is little endian; call :cpp:func:`ByteReader::setEndianness() <erbsland::mem::ByteReader::setEndianness>`
once before parsing a big-endian format.

The templated :cpp:func:`ByteReader::readInteger() <erbsland::mem::ByteReader::readInteger>` and
:cpp:func:`ByteReader::readIntegerOrThrow() <erbsland::mem::ByteReader::readIntegerOrThrow>` add formats that are not
implied by a C++ type, such as a 24-bit or variable-length integer.
They validate the wire representation and ensure that the decoded value fits the requested result type.
See :doc:`byte_integer_formats` for the layouts and selection guidance.

Tolerant fixed-width reads accept a default value, and ``readIntegerInto()`` preserves its destination on failure.
Formatted tolerant reads return ``std::optional`` because failure can mean incomplete, malformed, or overflowing data.
In each case a failed read leaves the cursor unchanged, which makes it safe to wait for more input or try a deliberate
alternative.

.. erbsland-demo::
    :source: mem/ByteStreams/ReadIntegers.cpp
    :exec: mem/byte_streams --demo ReadIntegers
    :source-sha256: 187daa2dc23263d985460b0f5b17fa8b135bff33b961e37c4ee0d2acfd9df1c7

.. code-block:: cpp

    /// Read native and explicitly formatted integers from a byte stream.
    ///
    /// Native-width helpers use the reader's byte order. An explicit
    /// `ByteIntegerFormat` also validates the wire representation and target range.
    void readIntegers() {
        const auto record =
            el::ByteBlock{el::Byte{0x00U}, el::Byte{0x0cU}, el::Byte{0x81U}, el::Byte{0x2cU}, el::Byte{94U}};
        auto reader = el::ByteReader{record};
        reader.setEndianness(el::Endianness::Big);

        // Read a fixed native value, a compact formatted value, and an 8-bit field.
        const auto calls = reader.readUInt16OrThrow();
        const auto flightSeconds = reader.readIntegerOrThrow<uint32_t>(el::ByteIntegerFormat::UnsignedVariableLength);
        const auto confidence = reader.readUInt8(0U);

        el::io::printLine("Roepjes gehoord   : "_el, calls);
        el::io::printLine("Flight seconds    : "_el, flightSeconds);
        el::io::printLine("Confidence        : "_el, confidence);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Roepjes gehoord   : 12
    Flight seconds    : 300
    Confidence        : 94

.. erbsland-demo-end::

Decoding Text Fields
====================

:cpp:func:`ByteReader::readText() <erbsland::mem::ByteReader::readText>` decodes both the byte-level framing and the
character encoding described by :cpp:class:`ByteTextOptions <erbsland::mem::ByteTextOptions>`.
Its default is a UTF-8 field preceded by a little-endian unsigned 32-bit code-unit count.
The optional result is empty for an incomplete or invalid frame, and the reader remains at the field's beginning.

:cpp:func:`ByteReader::readTextOrThrow() <erbsland::mem::ByteReader::readTextOrThrow>` is appropriate once the record
schema requires the field.
It distinguishes missing bytes from malformed framing through exceptions and advances only after decoding the complete
field.
Reader endianness controls integer count fields; an encoding with an explicit byte order, such as UTF-16 big endian,
controls its own code units independently.
The framing choices are covered in detail in :doc:`text_byte_stream_options`.

.. erbsland-demo::
    :source: mem/ByteStreams/ReadText.cpp
    :exec: mem/byte_streams --demo ReadText
    :source-sha256: 0a56b02e61c2d365d89a9f7f33dba48c3715939289335fd45c7a43a169b17ff3

.. code-block:: cpp

    /// Decode framed text from a byte stream.
    ///
    /// `ByteTextOptions` defines both the character encoding and the byte-level
    /// framing. Optional reading leaves the position unchanged if a complete valid
    /// field is unavailable.
    void readText() {
        const auto record = el::ByteBlock{
            el::Byte{0x06U},
            el::Byte{0x00U},
            el::Byte{0x00U},
            el::Byte{0x00U},
            el::Byte{'b'},
            el::Byte{'o'},
            el::Byte{'s'},
            el::Byte{'u'},
            el::Byte{'i'},
            el::Byte{'l'}};
        auto reader = el::ByteReader{record};

        // Decode the default length-prefixed UTF-8 field.
        const auto species = reader.readTextOrThrow();
        const auto missingField = reader.readText();
        el::io::printLine("Soort              : "_el, species);
        el::io::printLine("Complete next field: "_el, el::BooleanFormat::yesNo(), missingField.has_value());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Soort              : bosuil
    Complete next field: no

.. erbsland-demo-end::

Keeping the Tool Proportional to the Task
=========================================

A reader earns its small amount of state when it turns a sequence of fields into a clear parsing flow or when helper
functions need to share progress.
For a single integer at a known offset, use the block's ``getInteger`` family instead.
For individual bytes, use ``get``; for a complete visit, use ``forEach``.
When the inverse operation is needed, :doc:`writing_byte_streams` shows how
:cpp:class:`ByteWriter <erbsland::mem::ByteWriter>` assembles the same kind of structured record.
