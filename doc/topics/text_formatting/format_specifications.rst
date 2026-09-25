..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Format Specifications
    single: Named Format Specifications
    single: Text Format Specification
    single: Number Format Specification
    single: Boolean Format Specification
    single: Byte Format Specification

*********************
Format Specifications
*********************

A placeholder can do more than select a value.
It can state that the value must be text, a number, a Boolean, or a byte block and describe the representation in words
that remain readable when the pattern grows.

Named format specifications are the primary Erbsland Core syntax for this job.
They are type-aware, order-independent, and consistent across the supported value families.
This page develops their shared grammar before showing every option available to text, integers, floating-point values,
Booleans, and bytes.

Select the Kind of Value First
==============================

A named specification has a selector followed by a second colon and a comma-separated option list:

.. code-block:: text

    {[index]:selector:option,option,...}

The optional index has the same meaning as an ordinary positional placeholder.
The selector is one of ``text``, ``number``, ``bool``, or ``bytes``.
Even an empty option list keeps the type check, so ``{:bytes:}`` accepts a ``ByteBlock`` but rejects a string containing
hexadecimal characters.

``text`` accepts Core and standard-library strings plus ``Char``.
``number`` accepts integer and floating-point values, while individual options can narrow that choice further.
``bool`` and ``bytes`` accept only their corresponding value families.
A mismatch raises ``FormatError`` when the pattern is applied.

Write Options for People First
==============================

Options are separated by commas and may appear in any order.
Names and predefined values are ASCII case-insensitive, but there is no insignificant whitespace inside a specification.
An option may appear only once, including through a mixture of its long name and alias.

Numeric values accept ``=`` or a compact joined spelling: ``width=12``, ``width12``, and ``w12`` mean the same thing.
Enum-like values require ``=``, while flags such as ``alternate`` appear without a value.
``fill`` consumes exactly one safe Unicode code point.

The short aliases are useful in narrow patterns, but the long names should be the default in application-owned formats:

.. list-table::
    :header-rows: 1
    :widths: 32 28 40

    *   -   Value family
        -   Long option
        -   Alias
    *   -   Text, number, Boolean
        -   ``width``, ``alignment``, ``fill``
        -   ``w``, ``al``, ``fl``
    *   -   Text
        -   ``maximum``, ``escape``, ``escape-amount``
        -   ``max``, ``esc``, ``ea``
    *   -   Number
        -   ``base``, ``notation``, ``letter-case``, ``sign``, ``precision``
        -   ``bs``, ``nt``, ``lc``, ``sg``, ``pr``
    *   -   Number flags
        -   ``alternate``, ``zero-fill``
        -   ``alt``, ``zf``
    *   -   Boolean
        -   ``style``, ``capitalization``
        -   ``sty``, ``cap``
    *   -   Bytes
        -   ``separator``, ``maximum``, ``truncate``
        -   ``sep``, ``max``, ``tr``

Lay Out Text, Numbers, and Booleans Consistently
================================================

``width`` gives the minimum field width in decoded code points.
``alignment`` accepts ``left``, ``right``, or ``center``; ``fill`` selects the character used for the remaining space.
These three options are shared by text, numbers, and Booleans, which keeps aligned reports consistent even when fields
contain different kinds of values.

Shape and Escape Text
=====================

The ``text`` selector adds ``maximum``, which truncates the source to a code-point count before escaping and layout.
``escape`` selects ``none``, ``html``, ``json``, ``cpp``, ``xml``, ``regex``, ``display``, ``config``, ``config_test``,
or ``markdown``.
When an escape format is selected, ``escape-amount`` chooses ``nothing``, ``required``, ``balanced``, ``non-ascii``, or
``all``.

Truncating before escaping prevents one source character from being split across an escape sequence.
Applying width afterward makes the visible escaped result the value that participates in layout.

.. erbsland-demo::
    :source: text/StringFormat/NamedTextFormat.cpp
    :exec: text/string_format --demo NamedTextFormat
    :source-sha256: 002a9465ca20a6cf660326f5d315922a028e2a03591967d185f432a760ece75d

.. code-block:: cpp

    /// Named text specifications make truncation, escaping, and layout readable at the call site.
    void namedTextFormat() {
        const auto heading = el::StringFormat{"{:text:maximum=12,width=18,alignment=center,fill=·}"_el};
        const auto json = el::StringFormat{"{:text:escape=json,escape-amount=non-ascii}"_el};

        el::io::printLine("|", heading.build("bodemvochtigheid"_el), "|"_el);
        el::io::printLine(json.build("sensor café\n"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    |···bodemvochtig···|
    sensor caf\u00E9\n

.. erbsland-demo-end::

Describe Integer Output in Its Own Vocabulary
=============================================

For integers, ``base`` accepts ``decimal``, ``hexadecimal``, ``binary``, or ``octal``.
``letter-case`` accepts ``lowercase`` or ``uppercase``, and ``sign`` accepts ``negative-only``, ``always``, or
``space``.
``precision`` gives the minimum digit count.
The ``alternate`` flag adds a base prefix, while ``zero-fill`` fills the numeric field with zeroes instead of the layout
fill character.

These integer-only choices reject floating-point arguments.
This catches a changed value type before it silently changes a protocol field or identifier.

.. erbsland-demo::
    :source: text/StringFormat/NamedIntegerFormat.cpp
    :exec: text/string_format --demo NamedIntegerFormat
    :source-sha256: a249e944bd30f7b9ac25367dee8dcd8c1c13771f537f0f7a69fe9181e003783a

.. code-block:: cpp

    /// Named number specifications can lock a field to integers and describe its base and layout explicitly.
    void namedIntegerFormat() {
        const auto packetId =
            el::StringFormat{"{:number:base=hexadecimal,alternate,letter-case=uppercase,width=10,zero-fill}"_el};
        const auto signedCount = el::StringFormat{"{:number:base=decimal,sign=always,width=7,alignment=right}"_el};

        el::io::printLine("Packet id ...: "_el, packetId.build(42));
        el::io::printLine("Difference ..: "_el, signedCount.build(17));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Packet id ...: 0X0000002A
    Difference ..:     +17

.. erbsland-demo-end::

Choose Floating-Point Notation Explicitly
=========================================

Floating-point values use ``notation`` instead of ``base``.
It accepts ``default``, ``fixed``, ``scientific``, ``general``, or ``hexadecimal``.
``precision``, ``letter-case``, and ``sign`` then refine that notation, and the shared layout options position the final
number.

Selecting ``notation`` locks the field to floating-point values; an integer supplied to that field is rejected.
Conversely, ``base`` locks a numeric field to integers.

.. erbsland-demo::
    :source: text/StringFormat/NamedFloatFormat.cpp
    :exec: text/string_format --demo NamedFloatFormat
    :source-sha256: 265fb85124024fa845e7728e4ccac04c179717e3d4320ab528f33b1883d88bfe

.. code-block:: cpp

    /// Named number specifications use notation to select floating-point formatting.
    void namedFloatFormat() {
        const auto fixed = el::StringFormat{"{:number:notation=fixed,precision=2,sign=always}"_el};
        const auto scientific = el::StringFormat{"{:number:notation=scientific,precision=3,letter-case=uppercase}"_el};

        el::io::printLine("Temperature ...: "_el, fixed.build(21.375));
        el::io::printLine("Pressure ......: "_el, scientific.build(1013.25));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Temperature ...: +21.38
    Pressure ......: 1.013E+03

.. erbsland-demo-end::

Give Booleans the Words Your Interface Uses
===========================================

The ``bool`` selector uses ``style`` to choose ``true``, ``yes``, ``on``, or ``enabled`` as the positive word and the
matching negative word automatically.
``capitalization`` accepts ``lowercase``, ``uppercase``, or ``titlecase``.
The shared width, alignment, and fill options are applied after the word is selected.

.. erbsland-demo::
    :source: text/StringFormat/NamedBooleanFormat.cpp
    :exec: text/string_format --demo NamedBooleanFormat
    :source-sha256: 31a4cf5a645c8c9a95b293159d19c5edf7b5dc1797b1ec6c0cca7fa492764dc8

.. code-block:: cpp

    /// Named Boolean specifications select the word pair, capitalization, and layout independently.
    void namedBooleanFormat() {
        const auto status =
            el::StringFormat{"{:bool:style=enabled,capitalization=uppercase,width=10,alignment=right,fill=.}"_el};
        const auto answer = el::StringFormat{"{:bool:style=yes,capitalization=titlecase}"_el};

        el::io::printLine("Sensor ...: "_el, status.build(true));
        el::io::printLine("Alarm ....: "_el, answer.build(false));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Sensor ...: ...ENABLED
    Alarm ....: No

.. erbsland-demo-end::

Keep Byte Diagnostics Bounded
=============================

The ``bytes`` selector formats a ``ByteBlock`` as hexadecimal text.
``separator`` inserts spaces between bytes, ``maximum`` limits the number of output items, and ``truncate`` chooses
``begin``, ``middle``, or ``end`` when the source exceeds that limit.
The fixed ellipsis occupies one output item, so a bounded diagnostic always respects its configured maximum.

Byte specifications intentionally omit text and numeric layout options.
They describe the byte sequence itself rather than treating its hexadecimal representation as ordinary text.

.. erbsland-demo::
    :source: text/StringFormat/NamedByteFormat.cpp
    :exec: text/string_format --demo NamedByteFormat
    :source-sha256: 30fd5ad396eda38080e05611e0595a36650e9cc86e69392e9d585c53e6a91e66

.. code-block:: cpp

    /// Named byte specifications make separators and bounded diagnostic output explicit.
    void namedByteFormat() {
        const auto packet = el::mem::ByteBlock::fromVector(
            std::vector<uint8_t>{0x10U, 0x21U, 0x32U, 0x43U, 0x54U, 0x65U, 0x76U, 0x87U, 0x98U});
        const auto full = el::StringFormat{"{:bytes:separator}"_el};
        const auto diagnostic = el::StringFormat{"{:bytes:separator,maximum=6,truncate=middle}"_el};

        el::io::printLine("Complete ......: "_el, full.build(packet));
        el::io::printLine("Diagnostic ....: "_el, diagnostic.build(packet));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Complete ......: 10 21 32 43 54 65 76 87 98
    Diagnostic ....: 10 21 32 … 87 98

.. erbsland-demo-end::

Unknown or duplicate options, missing values, unsupported combinations, and unsafe characters make construction fail
with ``FormatError``.
For the exhaustive alias and value table, see the
:doc:`text formatting and parsing reference </reference/text/formatting_and_parsing>`.
