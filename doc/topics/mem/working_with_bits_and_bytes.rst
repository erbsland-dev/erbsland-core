.. index::
    single: Byte; Working with bits
    single: Bits; Byte operations
    single: Memory; Byte

***************************
Working with Bits and Bytes
***************************

Binary formats often begin with the smallest useful value: one byte containing flags, a compact integer, a character, or
part of a larger value.
This page introduces :cpp:class:`Byte <erbsland::mem::Byte>`, explains why it is a better fit for binary data than a
plain character or integer, and shows how to manipulate and convert bytes without hiding your intent.

A Type That Says “This Is Binary Data”
======================================

The C++ language offers several types that occupy one byte, but they communicate different meanings.
A ``char`` belongs to character data, while ``uint8_t`` is an integer that happens to be eight bits wide.
``std::byte`` clearly represents binary data, but deliberately provides only a small set of operations.

:cpp:class:`Byte <erbsland::mem::Byte>` combines the useful parts of these choices.
It is an unsigned eight-bit value that marks data as binary, while still providing shifts, rotations, masks,
comparisons, and explicit conversions.
That distinction makes code easier to review: a parameter of type ``Byte`` carries a piece of binary data, not a number
that callers should freely add or multiply.

The abstraction has no storage cost.
The library verifies at compile time that ``sizeof(Byte)`` equals ``sizeof(std::byte)``, and its small operations are
``constexpr`` functions that compile to the same direct byte operations you would write for ``std::byte``, ``char``, or
``uint8_t``.
You therefore gain a more expressive interface without making a byte larger or introducing runtime bookkeeping.

Combining and Moving Bits
=========================

When a byte contains flags or compact fields, the familiar ``|``, ``&``, ``^``, and ``~`` operators combine, retain,
toggle, and invert bits.
Their assignment forms—``|=``, ``&=``, and ``^=`` —change an existing byte.
All these operations return or retain a ``Byte``, so the result continues to be recognizable as binary data.

Shifts and rotations solve two different problems.
``<<`` and ``>>``, or the named ``shiftedLeft()`` and ``shiftedRight()`` methods, move bits and fill the vacated
positions with zero.
If the amount is eight or greater, the result is zero.
Use ``shiftLeft()`` and ``shiftRight()``, or ``<<=`` and ``>>=``, when you want to change the byte itself.

Rotations preserve all eight bits by wrapping them around the opposite edge.
The ``rotatedLeft()`` and ``rotatedRight()`` methods return a new value, while ``rotateLeft()`` and ``rotateRight()``
change a byte in place.
Rotation amounts wrap modulo eight, and a negative amount rotates in the opposite direction.

The following route marker uses its upper bits for flags and a small field.
Alongside the operators, it demonstrates ``masked()`` for extracting that field and ``matches()`` for testing it without
first converting the byte to an integer.

.. erbsland-demo::
    :source: mem/Byte/BitOperations.cpp
    :exec: mem/byte --demo BitOperations
    :source-sha256: 64d8ed9f8f938951e867be3b2d134f5c996b6cd2cb309127456b2b760f8f7686

.. code-block:: cpp

    /// Work with flags and compact fields in a single byte.
    ///
    /// `Byte` supports the usual bitwise operators, shifts that fill with zero,
    /// rotations that preserve every bit, and named helpers for both returning a
    /// changed value and changing a byte in place. Masks can extract a field, while
    /// `matches()` tests a field without converting the byte to an integer first.
    void bitOperations() {
        const auto byteFormat =
            el::IntegerFormat::binary()
                .setFlags(
                    el::IntegerFormatFlag::BasePrefix | el::IntegerFormatFlag::ZeroFill | el::IntegerFormatFlag::Separator)
                .setFieldWidth(el::CpLength{8U});
        const auto silkRouteMarker = el::Byte{0b10110110U};
        const auto transportMask = el::Byte{0b11000000U};
        const auto verifiedFlag = el::Byte{0b00100000U};

        // Combine, retain, toggle, and invert flags with the familiar operators.
        const auto combined = silkRouteMarker | verifiedFlag;
        const auto transport = silkRouteMarker & transportMask;
        const auto toggled = silkRouteMarker ^ verifiedFlag;
        const auto inverted = ~silkRouteMarker;

        auto editedFlags = silkRouteMarker;
        editedFlags |= verifiedFlag;
        editedFlags &= el::Byte{0b11110000U};
        editedFlags ^= el::Byte{0b00010000U};

        el::io::printLine("Route              : Zijderoute"_el);
        el::io::printLine("Original marker    : "_el, byteFormat, silkRouteMarker.toUInt32());
        el::io::printLine("Combined flags     : "_el, byteFormat, combined.toUInt32());
        el::io::printLine("Transport field    : "_el, byteFormat, transport.toUInt32());
        el::io::printLine("Toggled flags      : "_el, byteFormat, toggled.toUInt32());
        el::io::printLine("Inverted flags     : "_el, byteFormat, inverted.toUInt32());
        el::io::printLine("Edited flags       : "_el, byteFormat, editedFlags.toUInt32());

        // Shift bits when their old positions can be discarded.
        const auto shiftedByOperator = (silkRouteMarker << 1U) >> 2U;
        const auto shiftedByName = silkRouteMarker.shiftedLeft(1U).shiftedRight(2U);
        auto shiftedInPlace = silkRouteMarker;
        shiftedInPlace <<= 1U;
        shiftedInPlace >>= 2U;
        shiftedInPlace.shiftLeft(1U);
        shiftedInPlace.shiftRight(1U);

        el::io::printLine("Operator shifts    : "_el, byteFormat, shiftedByOperator.toUInt32());
        el::io::printLine("Named shifts       : "_el, byteFormat, shiftedByName.toUInt32());
        el::io::printLine("In-place shifts    : "_el, byteFormat, shiftedInPlace.toUInt32());

        // Rotate bits when every bit must remain in the byte.
        const auto rotatedLeft = silkRouteMarker.rotatedLeft(2);
        const auto rotatedRight = silkRouteMarker.rotatedRight(2);
        auto rotatedInPlace = silkRouteMarker;
        rotatedInPlace.rotateLeft(2);
        rotatedInPlace.rotateRight(2);

        el::io::printLine("Rotated left       : "_el, byteFormat, rotatedLeft.toUInt32());
        el::io::printLine("Rotated right      : "_el, byteFormat, rotatedRight.toUInt32());
        el::io::printLine("Rotation restored  : "_el, el::BooleanFormat::yesNo(), rotatedInPlace == silkRouteMarker);

        // Extract and test the encoded transport field with a mask.
        const auto maskedTransport = silkRouteMarker.masked(transportMask);
        const auto isSeaRoute = silkRouteMarker.matches(transportMask, el::Byte{0b10000000U});

        el::io::printLine("Masked transport   : "_el, byteFormat, maskedTransport.toUInt32());
        el::io::printLine("Sea route          : "_el, el::BooleanFormat::yesNo(), isSeaRoute);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Route              : Zijderoute
    Original marker    : 0b1011'0110
    Combined flags     : 0b1011'0110
    Transport field    : 0b1000'0000
    Toggled flags      : 0b1001'0110
    Inverted flags     : 0b0100'1001
    Edited flags       : 0b1010'0000
    Operator shifts    : 0b0001'1011
    Named shifts       : 0b0001'1011
    In-place shifts    : 0b0001'1011
    Rotated left       : 0b1101'1010
    Rotated right      : 0b1010'1101
    Rotation restored  : yes
    Masked transport   : 0b1000'0000
    Sea route          : yes

.. erbsland-demo-end::

Crossing Type Boundaries Explicitly
===================================

Binary data eventually meets another interface: a system API expects ``std::byte``, a parser needs a wider unsigned
integer, or a byte represents a character from a known single-byte encoding.
Named conversion methods make that boundary visible.
Use ``toStdByte()``, ``toChar()``, or the width-specific ``toUInt8()``, ``toUInt16()``, ``toUInt32()``, and
``toUInt64()`` methods according to what the receiving API expects.
The name is useful even when a cast could produce the same machine instruction because it tells the reader which
representation is intended.

The factories describe the opposite direction.
``fromChar()`` copies the character's bit pattern, and ``fromUInt8()`` creates a byte from an already compatible
unsigned integer.
For wider integers, ``fromCroppedUInt16()``, ``fromCroppedUInt32()``, and ``fromCroppedUInt64()`` retain only the lowest
eight bits.
The word *cropped* is deliberate: unlike an ordinary cast, the call makes the loss of higher bits impossible to overlook
during review.

.. erbsland-demo::
    :source: mem/Byte/Conversions.cpp
    :exec: mem/byte --demo Conversions
    :source-sha256: 5651b9442ef84b85aef1adb32de8e0318fe75aa3afa95e30b678142006a9b532

.. code-block:: cpp

    /// Convert between a byte and character, integer, or `std::byte` values.
    ///
    /// Named conversions document the intended representation at each boundary.
    /// The `fromCropped...()` factories make truncation explicit when a wider
    /// integer contributes only its lowest eight bits.
    void conversions() {
        const auto byteFormat =
            el::IntegerFormat::binary()
                .setFlags(
                    el::IntegerFormatFlag::BasePrefix | el::IntegerFormatFlag::ZeroFill | el::IntegerFormatFlag::Separator)
                .setFieldWidth(el::CpLength{8U});

        // Create bytes from values read from a compact route archive.
        const auto direction = el::Byte::fromChar('E');
        const auto waypoint = el::Byte::fromUInt8(42U);
        const auto legacyMarker = el::Byte::fromCroppedUInt16(0x12abU);
        const auto mapMarker = el::Byte::fromCroppedUInt32(0x123456cdU);
        const auto catalogMarker = el::Byte::fromCroppedUInt64(0x123456789abcdef0ULL);

        el::io::printLine("Route              : Zijderoute"_el);
        el::io::printLine("Direction          : "_el, direction.toChar());
        el::io::printLine("Waypoint           : "_el, byteFormat, waypoint.toUInt32());
        el::io::printLine("Legacy marker      : "_el, byteFormat, legacyMarker.toUInt32());
        el::io::printLine("Map marker         : "_el, byteFormat, mapMarker.toUInt32());
        el::io::printLine("Catalog marker     : "_el, byteFormat, catalogMarker.toUInt32());

        // Convert one byte to the width expected by each receiving API.
        const auto standardByte = waypoint.toStdByte();
        const auto count8 = waypoint.toUInt8();
        const auto count16 = waypoint.toUInt16();
        const auto count32 = waypoint.toUInt32();
        const auto count64 = waypoint.toUInt64();

        el::io::printLine("As std::byte       : "_el, byteFormat, std::to_integer<uint32_t>(standardByte));
        el::io::printLine("As uint8_t         : "_el, byteFormat, count8);
        el::io::printLine("As uint16_t        : "_el, byteFormat, count16);
        el::io::printLine("As uint32_t        : "_el, byteFormat, count32);
        el::io::printLine("As uint64_t        : "_el, byteFormat, count64);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Route              : Zijderoute
    Direction          : E
    Waypoint           : 0b0010'1010
    Legacy marker      : 0b1010'1011
    Map marker         : 0b1100'1101
    Catalog marker     : 0b1111'0000
    As std::byte       : 0b0010'1010
    As uint8_t         : 0b0010'1010
    As uint16_t        : 0b0010'1010
    As uint32_t        : 0b0010'1010
    As uint64_t        : 0b0010'1010

.. erbsland-demo-end::

From One Byte to a Block
========================

Individual bytes are the building blocks for binary formats, but most work quickly grows into a sequence of them.
For a fixed-size local value, :cpp:class:`ByteArray <erbsland::mem::ByteArray>` applies the same bit-oriented ideas to
an entire array.
For owned application data, :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` and
:cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>` provide safe storage and copy-on-write editing, while
:cpp:class:`ByteBuffer <erbsland::mem::ByteBuffer>` is designed for a byte sequence that grows and changes.

Low-level spans are available when an operation must briefly view storage owned elsewhere.
Because their safety depends on the lifetime of that storage, prefer the owning containers whenever bytes must be kept,
shared, or passed beyond a tightly controlled call.
