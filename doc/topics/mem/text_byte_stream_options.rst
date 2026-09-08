.. index::
    single: Memory; Text in byte streams
    single: Binary data; Text framing
    single: ByteTextOptions
    single: ByteTextFormat

************************************
Encoding Text in Binary Byte Streams
************************************

Text inside a binary record needs two descriptions: how characters become bytes and how a reader finds the field's
boundary.
:cpp:class:`ByteTextOptions <erbsland::mem::ByteTextOptions>` keeps these choices together so the same schema can be
used by :cpp:class:`ByteWriter <erbsland::mem::ByteWriter>` and
:cpp:class:`ByteReader <erbsland::mem::ByteReader>`.
This page explains how to choose that schema, recognize its byte layout, and place practical limits around untrusted
input.

Why Text Needs Framing
======================

A binary record is only a sequence of bytes.
Even after a reader knows that the next field contains text, it cannot discover where that text ends by looking at the
payload alone.
If a record stores a name followed by a timestamp, for example, the reader needs an explicit rule that separates the
last byte of the name from the first byte of the timestamp.

That rule is the text field's *framing*.
The boundary can be carried by a count before the payload, represented by a special end mark after it, fixed by the
width of the field, or supplied as an exact length by the surrounding record.
Each representation solves a slightly different problem.
A count permits arbitrary text and lets a parser skip the value without scanning it.
An end mark can be convenient when a producer naturally writes until the text is complete.
A fixed-width field gives following fields predictable offsets and can be overwritten in place.

Understanding What a Count Measures
-----------------------------------

A count is only useful when writer and reader agree on what it measures.
Erbsland Core text prefixes count encoded *code units*: individual bytes for UTF-8, 16-bit units for UTF-16, and 32-bit
units for UTF-32. This is different from counting Unicode code points, and further removed from counting the characters
a reader perceives on screen.

The distinction becomes visible as soon as a character needs more than one code unit.
In the following example, ``¢`` occupies two UTF-8 units but only one UTF-16 unit.
The text is identical, yet the prefix changes with the encoding:

.. code-block:: text

    UTF-8, 8-bit units
    +---------+-------------------------------+
    | count=4 | 41 | c2 a2 | 42               |  "A¢B"
    +---------+-------------------------------+

    UTF-16 big endian, 16-bit units
    +---------+-------------------------------+
    | count=3 | 00 41 | 00 a2 | 00 42         |  "A¢B"
    +---------+-------------------------------+

Choosing a Reliable Boundary
----------------------------

The permitted text determines which kind of boundary is safe.
A count can describe any payload representable by its integer format.
An end mark, in contrast, is unambiguous only when that character cannot occur inside the payload or when the format
defines how to escape it.
This is why a delimiter that works for a few sample values may still be unsuitable for the complete input domain.

The surrounding record also matters.
If it already provides the exact byte length of the text, adding another count may merely duplicate information that can
become inconsistent.
If the record has no such boundary, the text field must carry enough framing to prevent the reader from consuming bytes
belonging to the next field.

Keeping the Boundary Defensive
------------------------------

Framing also establishes how much input a decoder is willing to process.
Treat every declared size as untrusted until it has been checked against both the available bytes and a sensible
application limit.
A very large but syntactically valid count can otherwise turn a small message into an excessive allocation request.
A missing end mark can similarly make a parser scan much farther than the schema intended.

The encoded payload needs validation of its own.
Malformed UTF must be rejected consistently before security-sensitive comparisons: non-shortest UTF-8 and invalid
surrogate sequences have historically allowed filters and consumers to interpret the same bytes differently.
An embedded zero deserves the same care when decoded text later crosses into a zero-terminated native API, where it can
silently hide the remainder of a value.
Finally, normalization and case conversion belong to the application layer; visually equivalent Unicode text can have
different code-point and byte sequences.

Selecting Dynamic or Padded Framing
===================================

:cpp:enum:`ByteTextFormat <erbsland::mem::ByteTextFormat>` provides two basic layouts.
The default ``Dynamic`` format consumes only its prefix, payload, and optional end mark.
``PaddedField`` always consumes the finite number of bytes configured by
:cpp:func:`ByteTextOptions::setLength() <erbsland::mem::ByteTextOptions::setLength>`.
Its unused tail is filled with the selected padding byte.

.. list-table:: How each option participates in the two formats
    :header-rows: 1
    :widths: 19 31 31

    * - Option
      - ``Dynamic``
      - ``PaddedField``
    * - ``encoding``
      - Encodes and decodes the payload.
      - Encodes and decodes the payload.
    * - ``length``
      - Maximum payload bytes, or the exact payload length when there is no count or end mark.
      - Required exact field width, including prefix, payload, end mark, and padding.
    * - ``countFormat``
      - Optional prefix that determines the payload length.
      - Optional prefix that determines the payload within the fixed field.
    * - ``endMark``
      - Optional delimiter or validation mark after the payload.
      - Optional delimiter or validation mark before the padding.
    * - ``padding``
      - Not used.
      - Fills every unused byte in the field.

The constructors, :cpp:func:`ByteTextOptions::format() <erbsland::mem::ByteTextOptions::format>`, and
:cpp:func:`ByteTextOptions::setFormat() <erbsland::mem::ByteTextOptions::setFormat>` make the choice explicit.
For a dynamic field, a finite ``length`` is an important input limit even when a count or end mark supplies the actual
boundary.
For a padded field, that length is mandatory.

.. erbsland-demo::
    :source: mem/ByteTextOptions/SelectFormat.cpp
    :exec: mem/byte_text_options --demo SelectFormat
    :source-sha256: 0c5c9e07abf39220fd26184c303e459a0b35d3fd2d7b8ba7dbfccb2b1ba4e38d

.. code-block:: cpp

    /// Select dynamic or fixed-field framing for encoded text.
    ///
    /// `ByteTextOptions::setFormat()` changes how the reader finds the end of a
    /// field. Dynamic fields carry their own boundary, while padded fields occupy
    /// exactly the configured byte length.
    void selectFormat() {
        auto dynamicOptions = el::ByteTextOptions::compact();
        auto paddedOptions = dynamicOptions;
        paddedOptions.setFormat(el::ByteTextFormat::PaddedField).setLength(el::ByteLength{12U}).setPadding(el::Byte{0x20U});

        // Encode the same moon phase using dynamic and fixed-field framing.
        auto dynamicWriter = el::ByteWriter{};
        dynamicWriter.writeTextOrThrow("croissant"_el, dynamicOptions);
        auto paddedWriter = el::ByteWriter{};
        paddedWriter.writeTextOrThrow("croissant"_el, paddedOptions);

        el::io::printLine("Phase             : croissant"_el);
        el::io::printLine("Dynamic bytes     : "_el, el::ByteFormat::separated(), dynamicWriter.toByteBlock());
        el::io::printLine("Padded bytes      : "_el, el::ByteFormat::separated(), paddedWriter.toByteBlock());
        el::io::printLine(
            "Fixed field       : "_el,
            el::BooleanFormat::yesNo(),
            paddedOptions.format() == el::ByteTextFormat::PaddedField);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Phase             : croissant
    Dynamic bytes     : 09 63 72 6f 69 73 73 61 6e 74
    Padded bytes      : 09 63 72 6f 69 73 73 61 6e 74 20 20
    Fixed field       : yes

.. erbsland-demo-end::

Selecting the Character Encoding
================================

Framing tells a reader which bytes belong to the text field.
The character encoding tells it how those bytes represent Unicode text.
Both are part of the wire format: using the correct count with the wrong encoding still produces the wrong value, and
may even make an otherwise complete field impossible to decode.

:cpp:func:`ByteTextOptions::setEncoding() <erbsland::mem::ByteTextOptions::setEncoding>` selects UTF-8, UTF-16, or
UTF-32, including explicit little- and big-endian variants for the multi-byte encodings.
:cpp:func:`ByteTextOptions::encoding() <erbsland::mem::ByteTextOptions::encoding>` lets code inspect that choice.

Choosing an Encoding for the Wire Format
----------------------------------------

UTF-8 is usually the best default for new formats: ASCII text stays compact, the representation has no byte-order
choice, and all Unicode characters remain available.
A protocol that permits only ASCII can still use UTF-8 on the wire, but the application must separately validate the
allowed character repertoire.

UTF-16 can be appropriate when interoperating with an existing UTF-16 format, while UTF-32 offers fixed-width code units
at a substantial size cost.
Here, *fixed-width code unit* does not mean *fixed-width character*.
A Unicode code point outside the basic multilingual plane occupies a surrogate pair—two code units—in UTF-16. What a
user sees as one character can consist of several code points as well, such as a letter followed by a combining accent.
UTF-32 gives each code point one code unit, but even it cannot make these user-visible grapheme clusters a single unit.
Consequently, none of these encodings allows a byte or code-unit count to double as a reliable display-width or
user-visible character count.

The generic ``Utf16`` and ``Utf32`` values encode in little-endian order; an explicit variant is clearer in a binary
format specification.
Byte-order marks are rejected in these framed fields because the options already define the encoding.

Keeping Two Byte Orders Separate
--------------------------------

There are two independent byte-order decisions.
The :cpp:enum:`Endianness <erbsland::mem::Endianness>` configured on the reader or writer controls fixed-width integer
fields, including a fixed-width count prefix.
The selected :cpp:class:`StringEncoding <erbsland::text::StringEncoding>` separately controls the byte order of UTF-16
or UTF-32 code units.
Changing one does not change the other.
The next example deliberately uses a little-endian count and big-endian UTF-16 to make that separation visible.

.. erbsland-demo::
    :source: mem/ByteTextOptions/SelectEncoding.cpp
    :exec: mem/byte_text_options --demo SelectEncoding
    :source-sha256: 0f04bcef688096bd82f809bda200438bd147a9736b3c9893b5d277bffcf81412

.. code-block:: cpp

    /// Select a Unicode encoding and its byte order independently from the stream.
    ///
    /// The reader or writer endianness controls an integer count prefix. The byte
    /// order in an explicit UTF-16 or UTF-32 encoding controls the text code units.
    void selectEncoding() {
        auto options = el::ByteTextOptions{};
        options.setEncoding(el::StringEncoding::Utf16BigEndian).setCountFormat(el::ByteIntegerFormat::UnsignedFixed16Bit);
        auto writer = el::ByteWriter{};
        writer.setEndianness(el::Endianness::Little);

        // Write a little-endian count followed by big-endian UTF-16 code units.
        writer.writeTextOrThrow("éclipse"_el, options);
        const auto bytes = writer.toByteBlock();
        auto reader = el::ByteReader{bytes};
        reader.setEndianness(el::Endianness::Little);

        el::io::printLine("Decoded phase     : "_el, reader.readTextOrThrow(options));
        el::io::printLine("Encoded bytes     : "_el, el::ByteFormat::separated(), bytes);
        el::io::printLine(
            "UTF-16 big endian : "_el,
            el::BooleanFormat::yesNo(),
            options.encoding() == el::StringEncoding::Utf16BigEndian);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Decoded phase     : éclipse
    Encoded bytes     : 07 00 00 e9 00 63 00 6c 00 69 00 70 00 73 00 65
    UTF-16 big endian : yes

.. erbsland-demo-end::

Terminating Text with an End Mark
=================================

Some formats mark the end of text with a distinguished character instead of placing a count before it.
The zero character used by C strings is the familiar example, but a line break, record separator, or
application-specific delimiter can serve the same purpose.
This representation is useful when compatibility requires it or when a producer can emit text progressively without
knowing its final length in advance.

How the End Mark Defines the Boundary
-------------------------------------

:cpp:func:`ByteTextOptions::setEndMark() <erbsland::mem::ByteTextOptions::setEndMark>` configures a Unicode character,
which the writer encodes with the same character encoding as the payload and appends after it.
:cpp:func:`ByteTextOptions::endMark() <erbsland::mem::ByteTextOptions::endMark>` exposes the configured value.

When no count prefix is present, the reader searches for the encoded mark at valid code-unit boundaries.
The first match ends the payload and is consumed as part of the text frame.
When a count is also present, the two mechanisms have different jobs: the count locates the end of the payload, and the
reader then verifies that the expected mark follows it.
In this second form, the mark acts as a structural check rather than something the reader must search for.

.. code-block:: text

    without count                       with count
    +---------+------+                  +-------+---------+------+
    | payload | mark |                  | count | payload | mark |
    +---------+------+                  +-------+---------+------+
       searched boundary                   counted, then validated

Choosing a Safe End Mark
------------------------

The delimiter becomes part of the schema's permitted-input rules.
If it can appear inside ordinary text, a delimiter-only reader stops at that earlier occurrence and leaves the remaining
bytes to be interpreted as following fields.
For a format without escaping, choose a character the application already forbids and validate this restriction before
writing.
If every Unicode character must remain available, prefer a count or define an unambiguous escaping rule.

A zero character is conventional in several native formats, but convention alone does not remove this ambiguity.
It is safe only when embedded zeros are forbidden or escaped consistently throughout the system.
For untrusted dynamic text, a finite maximum length provides another essential boundary: if the expected mark is
missing, the reader rejects the field instead of searching through the rest of a potentially large record.

Call :cpp:func:`ByteTextOptions::clearEndMark() <erbsland::mem::ByteTextOptions::clearEndMark>` when changing a reused
option set back to count-only framing.

.. erbsland-demo::
    :source: mem/ByteTextOptions/UseEndMark.cpp
    :exec: mem/byte_text_options --demo UseEndMark
    :source-sha256: 833b9330a0663c45c5a4856eae684b88c680a0a230ec5d846e9263a2fe53731e

.. code-block:: cpp

    /// Terminate a dynamic text field with an encoded character.
    ///
    /// Clearing the count format and setting an end mark creates a delimited field.
    /// The configured maximum length bounds the search when reading untrusted data.
    void useEndMark() {
        auto options = el::ByteTextOptions{};
        options.clearCountFormat().setEndMark(U'|').setLength(el::ByteLength{16U});
        auto writer = el::ByteWriter{};

        // Encode two moon phases whose boundaries are marked by a vertical bar.
        writer.writeTextOrThrow("pleine"_el, options).writeTextOrThrow("nouvelle"_el, options);
        const auto bytes = writer.toByteBlock();
        auto reader = el::ByteReader{bytes};

        el::io::printLine("First phase       : "_el, reader.readTextOrThrow(options));
        el::io::printLine("Second phase      : "_el, reader.readTextOrThrow(options));
        el::io::printLine("Encoded fields    : "_el, el::ByteFormat::separated(), bytes);
        el::io::printLine("Has end mark      : "_el, el::BooleanFormat::yesNo(), options.endMark().has_value());
    }

.. erbsland-ansi::
    :escape-char: ␛

    First phase       : pleine
    Second phase      : nouvelle
    Encoded fields    : 70 6c 65 69 6e 65 7c 6e 6f 75 76 65 6c 6c 65 7c
    Has end mark      : yes

.. erbsland-demo-end::

Choosing the Count Representation
=================================

The default count is an unsigned fixed 32-bit integer.
:cpp:func:`ByteTextOptions::setCountFormat() <erbsland::mem::ByteTextOptions::setCountFormat>` can select a smaller
fixed width or a variable-length representation, while
:cpp:func:`ByteTextOptions::countFormat() <erbsland::mem::ByteTextOptions::countFormat>` returns the current optional
format.
:cpp:func:`ByteTextOptions::compact() <erbsland::mem::ByteTextOptions::compact>` is the convenient default when short
fields are common because it selects ``UnsignedVariableLength``.

A fixed count has predictable size and is easy to reserve or overwrite.
Choose a width that can represent the schema's maximum code-unit count, then enforce a compatible finite byte limit.
A variable-length count saves space for small values and grows for occasional long ones, at the cost of a variable field
offset.
Signed formats have no useful meaning for lengths and malformed negative counts are rejected.
See :doc:`byte_integer_formats` for the exact integer layouts.

.. erbsland-demo::
    :source: mem/ByteTextOptions/SelectCountFormat.cpp
    :exec: mem/byte_text_options --demo SelectCountFormat
    :source-sha256: 672c9ff2eac69be0804845031773339a6a1c8f7d1d0ddf233bcdfa66e594add4

.. code-block:: cpp

    /// Select the integer representation used for a text code-unit count.
    ///
    /// A fixed-width count is simple to inspect and overwrite. A variable-length
    /// count makes short text fields compact while still supporting large values.
    void selectCountFormat() {
        const auto fixedOptions = el::ByteTextOptions{};
        auto compactOptions = fixedOptions;
        compactOptions.setCountFormat(el::ByteIntegerFormat::UnsignedVariableLength);

        // Encode the same phase with a 32-bit and a variable-length count.
        auto fixedWriter = el::ByteWriter{};
        fixedWriter.writeTextOrThrow("lune"_el, fixedOptions);
        auto compactWriter = el::ByteWriter{};
        compactWriter.writeTextOrThrow("lune"_el, compactOptions);

        el::io::printLine("Fixed count       : "_el, el::ByteFormat::separated(), fixedWriter.toByteBlock());
        el::io::printLine("Compact count     : "_el, el::ByteFormat::separated(), compactWriter.toByteBlock());
        el::io::printLine("Count configured  : "_el, el::BooleanFormat::yesNo(), compactOptions.countFormat().has_value());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Fixed count       : 04 00 00 00 6c 75 6e 65
    Compact count     : 04 6c 75 6e 65
    Count configured  : yes

.. erbsland-demo-end::

Padding Fixed-Width Fields
==========================

:cpp:func:`ByteTextOptions::setPadding() <erbsland::mem::ByteTextOptions::setPadding>` chooses the byte written after a
padded field's meaningful frame, and :cpp:func:`ByteTextOptions::padding() <erbsland::mem::ByteTextOptions::padding>`
reports it.
Padding is useful in legacy records, tables with direct offsets, and fields that must be overwritten without shifting
the remainder of a file.
For ordinary sequential records, a dynamic count-prefixed value is usually smaller and clearer.

The reader needs a count or end mark to distinguish the payload from padding.
Without either, the complete fixed field is decoded as text and the padding byte becomes part of the value—or makes the
encoding invalid.
Padding is a byte, not a character; choose a value accepted by the surrounding format and do not assume the decoder will
strip matching bytes from the payload itself.

.. code-block:: text

    fixed field length = 12 bytes
    +---------+------+-------------------+
    | payload | mark | padding...........|
    +---------+------+-------------------+
    |<----------- always 12 bytes ------>|

.. erbsland-demo::
    :source: mem/ByteTextOptions/UsePadding.cpp
    :exec: mem/byte_text_options --demo UsePadding
    :source-sha256: b4f9653b758270bc3bdd700ecca933ae1e5baa4490df928323cf5009d96b3d72

.. code-block:: cpp

    /// Fill the unused bytes of a fixed text field with a chosen padding byte.
    ///
    /// A count or end mark identifies the payload inside the field. The reader then
    /// consumes the complete field, including padding, before reading the next value.
    void usePadding() {
        auto options = el::ByteTextOptions{el::ByteTextFormat::PaddedField};
        options.clearCountFormat().setEndMark(U'|').setLength(el::ByteLength{12U}).setPadding(el::Byte{0x2eU});
        auto writer = el::ByteWriter{};

        // Store one phase in a twelve-byte, dot-padded field.
        writer.writeTextOrThrow("lune"_el, options);
        const auto bytes = writer.toByteBlock();
        auto reader = el::ByteReader{bytes};

        el::io::printLine("Decoded phase     : "_el, reader.readTextOrThrow(options));
        el::io::printLine("Padded field      : "_el, el::ByteFormat::separated(), bytes);
        el::io::printLine("Padding byte      : "_el, options.padding().toUInt32());
        el::io::printLine("Reader at end     : "_el, el::BooleanFormat::yesNo(), reader.isAtEnd());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Decoded phase     : lune
    Padded field      : 6c 75 6e 65 7c 2e 2e 2e 2e 2e 2e 2e
    Padding byte      : 46
    Reader at end     : yes

.. erbsland-demo-end::

Common Schemas
==============

New application formats commonly use UTF-8 with a fixed or variable-length unsigned count.
For example, Protocol Buffers uses a variable-length byte count for its UTF-8 ``string`` values, while DNS labels use a
small fixed-width count supplied by their surrounding format.
C interfaces and several older binary formats use a zero end mark, and fixed-width padded text remains common in
table-like legacy formats.
UTF-16 and UTF-32 are most useful when an existing file format or platform contract already requires them.

Whichever representation you choose, document the encoding, count unit, byte order, maximum length, terminator rules,
and padding in the format specification.
Use the identical options for writing and reading, decode before applying text policy, and keep semantic checks such as
normalization, identifier rules, and embedded-zero rejection at the boundary where the decoded value gains meaning.
