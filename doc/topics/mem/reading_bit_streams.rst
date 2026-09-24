.. index::
    single: Memory; Reading bits
    single: Binary data; Bit-packed fields
    single: BitReader

************************
Reading a Stream of Bits
************************

Some binary formats use individual bits instead of whole bytes for flags, small numbers, and tightly packed records.
:cpp:class:`BitReader <erbsland::mem::BitReader>` gives these formats a small sequential cursor, so the parser can
follow the wire layout without repeating byte indexes, masks, and shifts.
This page explains when that cursor is useful, how its boundaries behave, and how to read and revisit fields safely.

When a Bit Reader Fits
======================

Use a bit reader when the order of fields matters and at least some fields are smaller than one byte.
Protocol flags, compact sensor records, image metadata, and entropy-coded headers are typical examples.
The default order consumes bits most-significant first within each byte, which means bit position zero is the high bit
of the first byte.
Select ``BitOrder::LeastSignificantFirst`` at construction for formats that define the opposite order.
The selected order is immutable and applies to both byte traversal and the significance of multi-bit fields.

Direct operations on :cpp:class:`Byte <erbsland::mem::Byte>` remain simpler when you only need one known flag from one
known byte.
Likewise, a :cpp:class:`ByteReader <erbsland::mem::ByteReader>` is the better fit once every field is byte-aligned.
``BitReader`` earns its place when a format is naturally described as a sequence of bits and the parser benefits from
one cursor that advances with every read.

The reader shares ownership of a :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>`.
It is move-only and keeps the data alive for the complete parsing operation.

Reading and Navigating Bit-Packed Data
======================================

Construct the reader with a byte block, bit order, and optionally the bit position where parsing should begin.
:cpp:func:`BitReader::bitCount() <erbsland::mem::BitReader::bitCount>` reports the total number of bits, while
:cpp:func:`BitReader::bitPosition() <erbsland::mem::BitReader::bitPosition>` identifies the next bit to consume.
Before reading a complete field, use
:cpp:func:`BitReader::canRead() <erbsland::mem::BitReader::canRead>` to reject truncated input as one unit.

:cpp:func:`BitReader::readBool() <erbsland::mem::BitReader::readBool>` returns the next bit as a Boolean.
:cpp:func:`BitReader::readBits() <erbsland::mem::BitReader::readBits>` reads complete unsigned fields up to 64 bits,
and :cpp:func:`BitReader::readByte() <erbsland::mem::BitReader::readByte>` processes eight consecutive bits even when
the cursor is unaligned.
At the end they return ``false`` or zero and leave the cursor at the end, so ``canRead()`` is what distinguishes a valid
zero bit from missing input.

Reserved fields can be skipped with
:cpp:func:`BitReader::advance() <erbsland::mem::BitReader::advance>`.
For a deliberate jump or rewind, use
:cpp:func:`BitReader::setBitPosition() <erbsland::mem::BitReader::setBitPosition>`.
Both operations clamp at ``bitCount()``, making an oversized position safe but not valid input.
:cpp:func:`BitReader::remainingBitCount() <erbsland::mem::BitReader::remainingBitCount>` reports what is left, and
:cpp:func:`BitReader::isAtEnd() <erbsland::mem::BitReader::isAtEnd>` makes completion explicit.

The following record contains a fixed eight-bit classification header followed by more bit-packed data.
The parser checks the complete header, consumes flags and a three-bit class, skips reserved bits, and then revisits the
payload boundary.

.. erbsland-demo::
    :source: mem/BitReader/ReadClassification.cpp
    :exec: mem/bit_reader --demo ReadClassification
    :source-sha256: 905e9c7d8d2d8df9411b595e73ecdadbe97d899366779eed34781690f5b3bcc3

.. code-block:: cpp

    /// Read a compact classification record one bit at a time.
    ///
    /// `BitReader` shares a byte block and keeps the position of the next bit.
    /// Boundary queries let a parser validate a field before consuming it, while
    /// `advance()` and `setBitPosition()` provide safe cursor navigation.
    void readClassification() {
        const auto record = el::ByteArray{el::Byte{0b10110110U}, el::Byte{0b11000000U}};
        auto reader = el::BitReader{el::ByteBlock{record}};

        // Validate the fixed header, then consume its flags and three-bit class.
        const auto hasHeader = reader.canRead(8U);
        const auto potentiallyHazardous = reader.readBool();
        const auto confirmed = reader.readBool();
        const auto asteroidClass = static_cast<uint8_t>(reader.readBits(3U));

        // Skip two reserved bits and read the final header flag.
        reader.advance(2U);
        const auto manuallyReviewed = reader.readBool();
        const auto payloadPosition = reader.bitPosition();

        // Revisit the payload and demonstrate that oversized positions clamp to the end.
        reader.setBitPosition(payloadPosition);
        const auto highConfidence = reader.readBool();
        reader.setBitPosition(100U);

        el::io::printLine("Klassifikation     : Jordnær asteroide"_el);
        el::io::printLine("Bits / header ready: "_el, reader.bitCount(), " / "_el, el::BooleanFormat::yesNo(), hasHeader);
        el::io::printLine(
            "Hazard / confirmed : "_el,
            el::BooleanFormat::yesNo(),
            potentiallyHazardous,
            " / "_el,
            el::BooleanFormat::yesNo(),
            confirmed);
        el::io::printLine("Class              : "_el, asteroidClass);
        el::io::printLine(
            "Reviewed / confidence: "_el,
            el::BooleanFormat::yesNo(),
            manuallyReviewed,
            " / "_el,
            el::BooleanFormat::yesNo(),
            highConfidence);
        el::io::printLine(
            "Remaining / at end : "_el, reader.remainingBitCount(), " / "_el, el::BooleanFormat::yesNo(), reader.isAtEnd());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Klassifikation     : Jordnær asteroide
    Bits / header ready: 16 / yes
    Hazard / confirmed : yes / no
    Class              : 6
    Reviewed / confidence: no / yes
    Remaining / at end : 0 / yes

.. erbsland-demo-end::

For input that may be malformed, validate every required group before consuming it.
Checking three bits once is clearer than accepting one successful read followed by two end-of-input zeros.
This also keeps format validation separate from the reader's intentionally tolerant boundary behavior.

Writing Bit-Packed Data
=======================

Use :cpp:class:`BitWriter <erbsland::mem::BitWriter>` to create the corresponding packed representation.
The writer is move-only and fixes its :cpp:enum:`BitOrder <erbsland::mem::BitOrder>` at construction.
``writeBool()``, ``writeBits()``, and ``writeByte()`` overwrite at the current cursor or extend the logical bit count.
Byte operations never align implicitly; call ``alignToByte()`` to explicitly write zero padding.
``toByteBlock()`` shares the physical output bytes, including a zero-filled partial final byte, while
``takeByteBlockEditor()`` transfers those bytes and resets the writer.
