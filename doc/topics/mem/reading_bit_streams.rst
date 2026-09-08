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
The reader consumes bits most-significant first within each byte, which means bit position zero is the high bit of the
first byte.

Direct operations on :cpp:class:`Byte <erbsland::mem::Byte>` remain simpler when you only need one known flag from one
known byte.
Likewise, a :cpp:class:`ByteReader <erbsland::mem::ByteReader>` is the better fit once every field is byte-aligned.
``BitReader`` earns its place when a format is naturally described as a sequence of bits and the parser benefits from
one cursor that advances with every read.

The reader borrows a :cpp:type:`ConstByteSpan <erbsland::mem::ConstByteSpan>` rather than owning the bytes.
Keep the source storage alive and unchanged for the complete parsing operation.
If the data must outlive its original owner, first place it in an owning
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` and construct the reader from that block's span.

Reading and Navigating Bit-Packed Data
======================================

Construct the reader with a byte span and, optionally, the bit position where parsing should begin.
:cpp:func:`BitReader::bitCount() <erbsland::mem::BitReader::bitCount>` reports the total number of bits, while
:cpp:func:`BitReader::bitPosition() <erbsland::mem::BitReader::bitPosition>` identifies the next bit to consume.
Before reading a complete field, use
:cpp:func:`BitReader::canRead() <erbsland::mem::BitReader::canRead>` to reject truncated input as one unit.

:cpp:func:`BitReader::readBool() <erbsland::mem::BitReader::readBool>` returns the next bit as a Boolean.
:cpp:func:`BitReader::readInteger() <erbsland::mem::BitReader::readInteger>` returns the same bit as zero or one in the
requested native integer type, which is convenient when assembling a small multi-bit value with shifts.
Both operations advance the cursor by one bit when input remains.
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
    :source-sha256: d922d3015da32f1092ca40540bc8412921052239799d25b88b1938d4cdaf3fca

.. code-block:: cpp

    /// Read a compact classification record one bit at a time.
    ///
    /// `BitReader` borrows a byte span and keeps the position of the next bit.
    /// Boundary queries let a parser validate a field before consuming it, while
    /// `advance()` and `setBitPosition()` provide safe cursor navigation.
    void readClassification() {
        const auto record = el::ByteArray{el::Byte{0b10110110U}, el::Byte{0b11000000U}};
        auto reader = el::BitReader{record.span()};

        // Validate the fixed header, then consume its flags and three-bit class.
        const auto hasHeader = reader.canRead(8U);
        const auto potentiallyHazardous = reader.readBool();
        const auto confirmed = reader.readInteger<uint8_t>() != 0U;
        auto asteroidClass = static_cast<uint8_t>(reader.readInteger<uint8_t>() << 2U);
        asteroidClass |= static_cast<uint8_t>(reader.readInteger<uint8_t>() << 1U);
        asteroidClass |= reader.readInteger<uint8_t>();

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
