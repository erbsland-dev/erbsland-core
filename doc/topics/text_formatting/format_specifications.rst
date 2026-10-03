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

***************************
Named Format Specifications
***************************

A report may need a centered heading, a hexadecimal packet identifier, a rounded measurement, and a readable status
word.
Named format specifications describe those choices inside a :cpp:type:`StringFormat <erbsland::text::StringFormat>`
pattern, beside the placeholder they affect.
Their names make a longer pattern readable without having to memorize compact format codes.

This page builds those representations from the shared syntax, then explains type checks and invalid specifications.
For selecting and reordering arguments before formatting them, start with :doc:`using_string_format`.

Describing a Value's Representation
===================================

A named specification adds a selector and an option list to an ordinary placeholder:

.. code-block:: text

    {[index]:selector:option,option,...}

The optional index selects the argument.
The selector describes its value family: ``text``, ``number``, ``bool``, or ``bytes``.
For example, ``{:bool:style=yes,capitalization=capitalized}`` turns a Boolean into ``Yes`` or ``No``.
The format chooses both words, so the calling code can continue passing the Boolean value directly.

Options are separated by commas and can appear in any order.
Long names make application-owned patterns easy to read; short aliases are available when space is limited.
``width=12``, ``width12``, and ``w12`` all select the same width.
Names and predefined values are ASCII case-insensitive, but whitespace inside a specification is significant and cannot
be added merely to separate options.

Laying Out and Escaping Text
============================

For a report heading, first decide how much source text to show, then how to position it in the field.
``maximum`` limits the number of source code points; ``width`` gives the minimum output width.
``alignment`` places the value at the left, right, or center, and ``fill`` supplies the padding character.
The first pattern below limits the heading to twelve code points and centers it in eighteen positions:

.. erbsland-demo::
    :source: text/StringFormat/NamedTextFormat.cpp
    :exec: text/string_format --demo NamedTextFormat
    :sha256: e4ea08d42bff86616cc33d13b086bd34276b0a03ec5069d50255c0dc89c64674

.. code-block:: cpp

    /// Named text specifications make truncation, escaping, and layout readable at the call site.
    void namedTextFormat() {
        const auto heading = el::StringFormat{"{:text:maximum=12,width=18,alignment=center,fill=·}"_el};
        const auto json = el::StringFormat{"{:text:escape=json,escape-amount=non-ascii}"_el};

        el::io::printLine("|", heading.build("bodemvochtigheid"_el), "|"_el);
        el::io::printLine(json.build("sensor café\n"_el));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m|···bodemvochtig···|
        sensor caf\u00E9\n␛[0m

.. erbsland-demo-end::

The second pattern selects JSON escaping and requests escapes for non-ASCII characters as well.
The output is an escaped fragment; the pattern does not add surrounding JSON quotation marks.
Truncation happens before escaping, so a source character's escape sequence remains complete.
Width and alignment are applied afterward to the escaped result.

``width``, ``alignment``, and ``fill`` also work for numbers and Booleans.
Width is measured in decoded code points, which may differ from terminal columns or user-perceived characters.
A minimum width does not truncate a longer value; ``maximum`` controls text truncation separately.

The ``escape`` option selects the destination syntax, such as ``json``, ``html``, or ``xml``.
``escape-amount`` controls how much is escaped within that syntax.
Choose these together according to where the fragment will be used.
The :doc:`formatting and parsing reference </reference/text/formatting_and_parsing>` lists the available escape formats.

Formatting Integer Identifiers and Counts
=========================================

A numeric identifier may need a base prefix and a fixed minimum width, while a report's difference column needs a sign
and alignment.
Named number specifications can express both:

.. erbsland-demo::
    :source: text/StringFormat/NamedIntegerFormat.cpp
    :exec: text/string_format --demo NamedIntegerFormat
    :sha256: 3729a74a7208a6a81a32ffeec51f2a663725e7a0666db0698191dbcd16ba754a

.. code-block:: cpp

    /// Named number specifications can lock a field to integers and describe its base and layout explicitly.
    void namedIntegerFormat() {
        const auto packetId = el::StringFormat{"{:number:hex,base-prefix,letter-case=uppercase,width=10,zero-fill}"_el};
        const auto signedCount = el::StringFormat{"{:number:base=decimal,sign=always,width=7,alignment=right}"_el};

        el::io::printLine("Packet id ...: "_el, packetId.build(42));
        el::io::printLine("Difference ..: "_el, signedCount.build(17));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mPacket id ...: 0X0000002A
        Difference ..:     +17␛[0m

.. erbsland-demo-end::

The packet identifier uses ``hex`` to select hexadecimal digits and ``base-prefix`` to include ``0x``.
``letter-case=uppercase`` affects both the digits and the prefix, giving ``0X0000002A`` for that packet identifier.
``zero-fill`` pads the numeric field with zeroes to its requested width.
Ordinary layout padding instead uses the ``fill`` character.

The difference uses decimal output and ``sign=always`` so a positive result visibly carries ``+``.
``sign=negative-only`` retains the usual spelling, while ``sign=space`` reserves a space for a positive value's sign.
When the requirement concerns the number of digits rather than the field width, ``precision`` sets a minimum digit
count.

``base=hexadecimal`` and ``base=hex`` are longer spellings of ``hex``.
Decimal, octal, and binary forms are available as well.
Selecting a base requires an integer argument, so a later change to a floating-point value is reported instead of
silently altering the identifier's representation.

Choosing Floating-Point Notation and Precision
==============================================

For measurements, choose notation according to how readers will compare the values.
Fixed notation is useful when the number of decimal places matters; scientific notation keeps large and small magnitudes
in a comparable form:

.. erbsland-demo::
    :source: text/StringFormat/NamedFloatFormat.cpp
    :exec: text/string_format --demo NamedFloatFormat
    :sha256: 9ee661fc859a9f2f6d99db8750e9a75f045b06fbd9cdec4e9a96953136259a63

.. code-block:: cpp

    /// Named number specifications use notation to select floating-point formatting.
    void namedFloatFormat() {
        const auto fixed = el::StringFormat{"{:number:notation=fixed,precision=2,sign=always}"_el};
        const auto scientific = el::StringFormat{"{:number:notation=scientific,precision=3,letter-case=uppercase}"_el};

        el::io::printLine("Temperature ...: "_el, fixed.build(21.375));
        el::io::printLine("Pressure ......: "_el, scientific.build(1013.25));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mTemperature ...: +21.38
        Pressure ......: 1.013E+03␛[0m

.. erbsland-demo-end::

The temperature uses two digits after the decimal point and rounds ``21.375`` to ``+21.38``.
The pressure uses scientific notation with three fractional digits and an uppercase ``E``.
Precision follows the selected notation, so changing notation can also change what a precision value means.
:doc:`converting text and scalar values </topics/text_parsing_and_encoding/converting_text_and_scalar_values>` explains
the corresponding formatting objects when you need to configure the representation in C++.

The ``notation`` option requires a floating-point argument.
An integer supplied to the scientific pattern is a type mismatch even if its value could be converted to a float.
This lets a format state the kind of value it expects as well as how that value should look.

Choosing Boolean Words and Capitalization
=========================================

A Boolean often reads better as a status or answer than as ``true`` or ``false``.
The ``bool`` selector's ``style`` chooses a pair of words: ``true`` and ``false``, ``yes`` and ``no``, ``on`` and
``off``, or ``enabled`` and ``disabled``.
Then ``capitalization`` controls the spelling of the selected word:

.. erbsland-demo::
    :source: text/StringFormat/NamedBooleanFormat.cpp
    :exec: text/string_format --demo NamedBooleanFormat
    :sha256: a86b62ba10f8098bff9baa93c9338eb075c39e350e9c23ccd18b052cdd38418f

.. code-block:: cpp

    /// Named Boolean specifications select the word pair, capitalization, and layout independently.
    void namedBooleanFormat() {
        const auto status =
            el::StringFormat{"{:bool:style=enabled,capitalization=uppercase,width=10,alignment=right,fill=.}"_el};
        const auto answer = el::StringFormat{"{:bool:style=yes,capitalization=capitalized}"_el};

        el::io::printLine("Sensor ...: "_el, status.build(true));
        el::io::printLine("Alarm ....: "_el, answer.build(false));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mSensor ...: ...ENABLED
        Alarm ....: No␛[0m

.. erbsland-demo-end::

``capitalization=capitalized`` produces an initial capital with the remaining letters lowercase, such as ``Yes`` or
``Disabled``.
The other choices are ``lowercase`` and ``uppercase``; ``cap=c``, ``cap=l``, and ``cap=u`` are their compact forms.
The shared width, alignment, and fill options are applied after the word has been selected and capitalized.

When a Boolean is already available, keeping these choices in the format avoids creating an intermediate string and then
applying a case transformation to it.
For a string supplied by another source, the helpers described in
:doc:`/topics/text_strings/transforming_strings` provide those transformations directly.

Keeping Byte Diagnostics Bounded
================================

A byte block needs a representation that makes its individual values visible.
The ``bytes`` selector renders hexadecimal bytes, with optional spaces for readability.
For a long payload, a bounded view can preserve the beginning and end around an ellipsis:

.. erbsland-demo::
    :source: text/StringFormat/NamedByteFormat.cpp
    :exec: text/string_format --demo NamedByteFormat
    :sha256: 7f18e2a07ea9a42b7faa4c7c5fb9f51bfb53a07684401d91369c995b8cbee615

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mComplete ......: 10 21 32 43 54 65 76 87 98
        Diagnostic ....: 10 21 32 … 87 98␛[0m

.. erbsland-demo-end::

``maximum=6`` counts output items, including the ellipsis, so the shortened diagnostic contains five bytes and one
ellipsis.
``truncate=middle`` selects the omitted portion; ``begin`` and ``end`` are also available.
This limit concerns byte items rather than the width of the resulting hexadecimal text.

The selector accepts a :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>`, not a string that happens to contain
hexadecimal characters.
Even an empty option list, ``{:bytes:}``, retains that type check.

Format Errors and Reusable Patterns
===================================

A :cpp:class:`FormatError <erbsland::text::FormatError>` during pattern construction identifies an invalid
specification, such as an unknown option, missing value, or unsupported combination.
An option may appear only once: mixing ``width`` and ``w`` is still a duplicate, as is combining ``hex`` with
``base=hexadecimal``.
``fill`` requires one safe Unicode code point.

Other checks depend on the argument and happen when the pattern is applied.
``text`` accepts Core strings, standard strings, and ``Char``; ``number`` accepts integers or floating-point values
unless an option narrows that family.
``bool`` and ``bytes`` accept their corresponding value types.
A mismatch raises ``FormatError`` at that point.

Keep a configured ``StringFormat`` when the same representation will be used repeatedly, then pass the original typed
values to each call.
The :doc:`text formatting and parsing reference </reference/text/formatting_and_parsing>` lists every supported option,
alias, and value for less common formatting requirements.
