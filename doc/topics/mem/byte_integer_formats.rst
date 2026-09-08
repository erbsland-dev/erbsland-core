.. index::
    single: Memory; Integer wire formats
    single: Binary data; Integer formats
    single: Variable-length integers
    single: Base-128 integers

******************************
Byte Integer Formats Explained
******************************

An integer in memory has a C++ type, but an integer in a file or protocol has a wire format.
:cpp:class:`ByteIntegerFormat <erbsland::mem::ByteIntegerFormat>` describes that representation independently from the
native type used by your application.
This page explains the fixed-width, prefix-coded, ZigZag, and unsigned base-128 formats and shows how to select one for
compact or interoperable binary data.

The format is used by :cpp:func:`ByteWriter::writeIntegerOrThrow() <erbsland::mem::ByteWriter::writeIntegerOrThrow>` and
the formatted overloads of
:cpp:func:`ByteReader::readInteger() <erbsland::mem::ByteReader::readInteger>` and
:cpp:func:`ByteReader::readIntegerOrThrow() <erbsland::mem::ByteReader::readIntegerOrThrow>`.
Writing rejects a value that the selected format cannot represent.
Optional reading leaves the reader position unchanged and returns no value for incomplete, malformed, or overflowing
input; strict reading reports the corresponding error.

Fixed-Width Integers
====================

A fixed-width integer always occupies the same number of bytes.
That makes record offsets predictable and permits direct access when a schema already determines every field's size.
Signed formats use two's-complement representation.
Unsigned formats with unusual widths such as 24 or 40 bits are useful for established formats that would otherwise waste
a byte or more per value.

Multi-byte fixed formats use the :cpp:enum:`Endianness <erbsland::mem::Endianness>` configured on the reader or writer.
Big endian places the most-significant byte first; little endian places the least-significant byte first:

.. code-block:: text

    Value 0x12345678

    Big endian       index  0       1       2       3
                           +-------+-------+-------+-------+
                     byte  |  12   |  34   |  56   |  78   |
                           +-------+-------+-------+-------+

    Little endian    index  0       1       2       3
                           +-------+-------+-------+-------+
                     byte  |  78   |  56   |  34   |  12   |
                           +-------+-------+-------+-------+

The available values and their attributes are:

.. list-table:: Fixed integer formats
    :header-rows: 1
    :widths: 44 14 16 26

    * - Format
      - Bytes
      - Signed
      - Value range
    * - ``UnsignedFixed8Bit``
      - 1
      - No
      - 0 through 2⁸−1
    * - ``SignedFixed8Bit``
      - 1
      - Yes
      - −2⁷ through 2⁷−1
    * - ``UnsignedFixed16Bit``
      - 2
      - No
      - 0 through 2¹⁶−1
    * - ``SignedFixed16Bit``
      - 2
      - Yes
      - −2¹⁵ through 2¹⁵−1
    * - ``UnsignedFixed24Bit``
      - 3
      - No
      - 0 through 2²⁴−1
    * - ``UnsignedFixed32Bit``
      - 4
      - No
      - 0 through 2³²−1
    * - ``SignedFixed32Bit``
      - 4
      - Yes
      - −2³¹ through 2³¹−1
    * - ``UnsignedFixed40Bit``
      - 5
      - No
      - 0 through 2⁴⁰−1
    * - ``UnsignedFixed48Bit``
      - 6
      - No
      - 0 through 2⁴⁸−1
    * - ``UnsignedFixed56Bit``
      - 7
      - No
      - 0 through 2⁵⁶−1
    * - ``UnsignedFixed64Bit``
      - 8
      - No
      - 0 through 2⁶⁴−1
    * - ``SignedFixed64Bit``
      - 8
      - Yes
      - −2⁶³ through 2⁶³−1

:cpp:func:`byteCount() <erbsland::mem::ByteIntegerFormat::byteCount>` reports the fixed width,
:cpp:func:`isSigned() <erbsland::mem::ByteIntegerFormat::isSigned>` reports signedness, and
:cpp:func:`isVariableLength() <erbsland::mem::ByteIntegerFormat::isVariableLength>` distinguishes this family from the
formats below.

.. erbsland-demo::
    :source: mem/ByteIntegerFormat/FixedFormats.cpp
    :exec: mem/byte_integer_format --demo FixedFormats
    :source-sha256: aafff61173586061dbe7e07370aff17aef4450af0e2da989dbf133db87ba3701

.. code-block:: cpp

    /// Encode and decode integers with an explicit fixed-width wire format.
    ///
    /// A `ByteIntegerFormat` selects signedness and byte width independently from
    /// the C++ value type. Multi-byte fixed formats use the byte order configured on
    /// the writer and reader.
    void fixedFormats() {
        auto writer = el::ByteWriter{};
        writer.setEndianness(el::Endianness::Big);

        // Encode a 24-bit unsigned skill identifier and a 32-bit signed modifier.
        writer.writeIntegerOrThrow<uint32_t>(0x123456U, el::ByteIntegerFormat::UnsignedFixed24Bit)
            .writeIntegerOrThrow<int32_t>(-300, el::ByteIntegerFormat::SignedFixed32Bit);
        const auto bytes = writer.toByteBlock();

        // Decode the fields with the same formats and byte order.
        auto reader = el::ByteReader{bytes};
        reader.setEndianness(el::Endianness::Big);
        const auto skillId = reader.readIntegerOrThrow<uint32_t>(el::ByteIntegerFormat::UnsignedFixed24Bit);
        const auto modifier = reader.readIntegerOrThrow<int32_t>(el::ByteIntegerFormat::SignedFixed32Bit);
        const auto idFormat = el::ByteIntegerFormat{el::ByteIntegerFormat::UnsignedFixed24Bit};

        el::io::printLine("Skill              : Flèche d'aurore"_el);
        el::io::printLine("Encoded fields     : "_el, el::ByteFormat::separated(), bytes);
        el::io::printLine("Skill identifier   : "_el, skillId);
        el::io::printLine("Signed modifier    : "_el, modifier);
        el::io::printLine("Identifier bytes   : "_el, idFormat.byteCount().toSizeT());
        el::io::printLine("Identifier signed  : "_el, el::BooleanFormat::yesNo(), idFormat.isSigned());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Skill              : Flèche d'aurore
    Encoded fields     : 12 34 56 ff ff fe d4
    Skill identifier   : 1193046
    Signed modifier    : -300
    Identifier bytes   : 3
    Identifier signed  : no

.. erbsland-demo-end::

Unsigned Prefix-Coded Integers
==============================

:cpp:enumerator:`UnsignedVariableLength <erbsland::mem::ByteIntegerFormat::UnsignedVariableLength>` favors small
non-negative values while retaining the complete 64-bit range.
The first byte tells the reader how many bytes belong to the value.
A leading zero means one byte, ``10`` means two, ``110`` means three, and the pattern continues through eight bytes.
An ``0xff`` prefix introduces a full eight-byte payload for values that need 57 through 64 bits.

.. code-block:: text

    Bytes  Leading pattern                    Payload bits
      1    0xxxxxxx                                  7
      2    10xxxxxx xxxxxxxx                        14
      3    110xxxxx xxxxxxxx xxxxxxxx               21
      4    1110xxxx + 3 payload bytes               28
      5    11110xxx + 4 payload bytes               35
      6    111110xx + 5 payload bytes               42
      7    1111110x + 6 payload bytes               49
      8    11111110 + 7 payload bytes               56
      9    11111111 + 8 payload bytes               64

Payload bits are ordered from most significant to least significant, independent of the reader or writer's configured
endianness.
The writer always chooses the shortest representation: a value at most ``2^(7n) - 1`` uses ``n`` bytes for ``n`` from
one through eight, and larger values use nine bytes.

This format works especially well for counts, lengths, and identifiers that are usually small but need a generous upper
bound.
It is less attractive for uniformly distributed 64-bit data, where most values grow to nine bytes, or for formats that
require a standardized encoding such as ASN.1 base-128.

For example, decimal ``300`` is binary ``1 0010 1100``.
It needs two bytes, so the six high payload bits join the ``10`` prefix and the remaining eight bits occupy the second
byte: ``81 2c``.

.. erbsland-demo::
    :source: mem/ByteIntegerFormat/UnsignedVariableLength.cpp
    :exec: mem/byte_integer_format --demo UnsignedVariableLength
    :source-sha256: 7e95109bae9672730a9f6f5624101216100df0993917224d9747fc4c6255fef4

.. code-block:: cpp

    /// Store unsigned values with the prefix-coded variable-length format.
    ///
    /// Small values occupy fewer bytes, while the format still covers the complete
    /// `uint64_t` range. The reader derives the encoded length from the leading byte,
    /// so consecutive values need no separate size field.
    void unsignedVariableLength() {
        auto writer = el::ByteWriter{};

        // Encode three skill-point totals of increasing size.
        writer.writeIntegerOrThrow<uint64_t>(42U, el::ByteIntegerFormat::UnsignedVariableLength)
            .writeIntegerOrThrow<uint64_t>(300U, el::ByteIntegerFormat::UnsignedVariableLength)
            .writeIntegerOrThrow<uint64_t>(0x123456U, el::ByteIntegerFormat::UnsignedVariableLength);
        const auto bytes = writer.toByteBlock();

        // Decode each self-delimiting value from the stream.
        auto reader = el::ByteReader{bytes};
        const auto novicePoints = reader.readIntegerOrThrow<uint64_t>(el::ByteIntegerFormat::UnsignedVariableLength);
        const auto adeptPoints = reader.readIntegerOrThrow<uint64_t>(el::ByteIntegerFormat::UnsignedVariableLength);
        const auto masteryPoints = reader.readIntegerOrThrow<uint64_t>(el::ByteIntegerFormat::UnsignedVariableLength);

        el::io::printLine("Skill tree         : Héritage du phénix"_el);
        el::io::printLine("Encoded totals     : "_el, el::ByteFormat::separated(), bytes);
        el::io::printLine("Novice points      : "_el, novicePoints);
        el::io::printLine("Adept points       : "_el, adeptPoints);
        el::io::printLine("Mastery points     : "_el, masteryPoints);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Skill tree         : Héritage du phénix
    Encoded totals     : 2a 81 2c d2 34 56
    Novice points      : 42
    Adept points       : 300

.. erbsland-demo-end::

Signed Prefix-Coded Integers
============================

:cpp:enumerator:`SignedVariableLength <erbsland::mem::ByteIntegerFormat::SignedVariableLength>` uses the same prefix
and payload layout after mapping a signed value to an unsigned value.
The ZigZag mapping interleaves negative and non-negative values around zero:

.. code-block:: text

    Signed value       0      -1       1      -2       2      -3       3
                       |       |       |       |       |       |       |
                       v       v       v       v       v       v       v
    Unsigned value     0       1       2       3       4       5       6

For a non-negative magnitude ``m``, the mapped value is ``2m``.
For a negative magnitude ``m``, it is ``2m - 1``.
The mapped integer is then encoded exactly like ``UnsignedVariableLength``.
This gives small positive and negative values similar costs, unlike a raw two's-complement variable representation in
which a small negative value appears to have many significant high bits.

The format is a good match for deltas, offsets, and corrections clustered around zero.
It is not automatically smaller for predominantly large positive values: the mapping spends one bit on the sign, so a
positive value may cross a size boundary earlier than its unsigned equivalent.

.. erbsland-demo::
    :source: mem/ByteIntegerFormat/SignedVariableLength.cpp
    :exec: mem/byte_integer_format --demo SignedVariableLength
    :source-sha256: 1a6b57922df3377c951988b5163bced1edafbda28fd3e8153d47d12cc02dbddf

.. code-block:: cpp

    /// Store signed values compactly with ZigZag mapping.
    ///
    /// The signed variable-length format maps values around zero to small unsigned
    /// numbers before applying prefix coding. Small positive and negative modifiers
    /// therefore require similarly compact representations.
    void signedVariableLength() {
        auto writer = el::ByteWriter{};

        // Encode nearby positive and negative skill modifiers, then a larger penalty.
        writer.writeIntegerOrThrow<int64_t>(-1, el::ByteIntegerFormat::SignedVariableLength)
            .writeIntegerOrThrow<int64_t>(1, el::ByteIntegerFormat::SignedVariableLength)
            .writeIntegerOrThrow<int64_t>(-300, el::ByteIntegerFormat::SignedVariableLength);
        const auto bytes = writer.toByteBlock();

        // Read the original signed values without handling the ZigZag mapping manually.
        auto reader = el::ByteReader{bytes};
        const auto shadowPenalty = reader.readIntegerOrThrow<int64_t>(el::ByteIntegerFormat::SignedVariableLength);
        const auto lightBonus = reader.readIntegerOrThrow<int64_t>(el::ByteIntegerFormat::SignedVariableLength);
        const auto masteryPenalty = reader.readIntegerOrThrow<int64_t>(el::ByteIntegerFormat::SignedVariableLength);

        el::io::printLine("Skill pair         : Ombre et lumière"_el);
        el::io::printLine("Encoded modifiers  : "_el, el::ByteFormat::separated(), bytes);
        el::io::printLine("Shadow modifier    : "_el, shadowPenalty);
        el::io::printLine("Light modifier     : "_el, lightBonus);
        el::io::printLine("Mastery modifier   : "_el, masteryPenalty);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Skill pair         : Ombre et lumière
    Encoded modifiers  : 01 02 82 57
    Shadow modifier    : -1
    Light modifier     : 1
    Mastery modifier   : -300

.. erbsland-demo-end::

Unsigned Base-128 Integers
==========================

:cpp:enumerator:`UnsignedBase128 <erbsland::mem::ByteIntegerFormat::UnsignedBase128>` divides an unsigned value into
seven-bit groups.
The most-significant group is written first.
Bit seven of each byte is a continuation flag: it is one when another group follows and zero in the final byte.

.. code-block:: text

    Value 16384 = binary 1 0000000 0000000

       first group      middle group       final group
      +---+-------+     +---+-------+     +---+-------+
      | 1 |0000001|     | 1 |0000000|     | 0 |0000000|
      +---+-------+     +---+-------+     +---+-------+
        ^  payload        ^  payload        ^  payload
        |                 |                 +-- no following byte
        +-----------------+-------------------- continuation bit

    Encoded bytes: 81 80 00

The writer emits the minimum number of groups.
The reader rejects a leading ``0x80`` group because it carries no information and would make the representation
non-canonical.
It also rejects values that exceed 64 bits and incomplete sequences whose last continuation bit is still set.
Configured endianness does not affect this format because the group order is part of the representation itself.

Use this format for ASN.1 identifier and object-identifier components, or when another specification explicitly asks for
a most-significant-group-first base-128 value.
Do not substitute it for the library's prefix-coded variable format: although both use a variable number of bytes, their
bit layouts and encoded values are different.

.. erbsland-demo::
    :source: mem/ByteIntegerFormat/UnsignedBase128.cpp
    :exec: mem/byte_integer_format --demo UnsignedBase128
    :source-sha256: 7edb48267d60d6a62238257eb3c8c93d4f1e88a2d4baace8d2a207c7bbd989f6

.. code-block:: cpp

    /// Encode canonical unsigned base-128 integers.
    ///
    /// Each byte contributes seven payload bits. The high bit marks every byte that
    /// is followed by another group, and the most-significant group comes first.
    /// This representation is useful for ASN.1 identifier components.
    void unsignedBase128() {
        auto writer = el::ByteWriter{};

        // Encode values around the one-byte and two-byte boundaries.
        writer.writeIntegerOrThrow<uint64_t>(127U, el::ByteIntegerFormat::UnsignedBase128)
            .writeIntegerOrThrow<uint64_t>(128U, el::ByteIntegerFormat::UnsignedBase128)
            .writeIntegerOrThrow<uint64_t>(16384U, el::ByteIntegerFormat::UnsignedBase128);
        const auto bytes = writer.toByteBlock();

        // Decode the canonical groups back into native values.
        auto reader = el::ByteReader{bytes};
        const auto firstNode = reader.readIntegerOrThrow<uint64_t>(el::ByteIntegerFormat::UnsignedBase128);
        const auto secondNode = reader.readIntegerOrThrow<uint64_t>(el::ByteIntegerFormat::UnsignedBase128);
        const auto thirdNode = reader.readIntegerOrThrow<uint64_t>(el::ByteIntegerFormat::UnsignedBase128);

        el::io::printLine("Skill route        : Constellation du mage"_el);
        el::io::printLine("Encoded nodes      : "_el, el::ByteFormat::separated(), bytes);
        el::io::printLine("First node         : "_el, firstNode);
        el::io::printLine("Second node        : "_el, secondNode);
        el::io::printLine("Third node         : "_el, thirdNode);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Skill route        : Constellation du mage
    Encoded nodes      : 7f 81 00 81 80 00
    First node         : 127
    Second node        : 128
    Third node         : 16384

.. erbsland-demo-end::
