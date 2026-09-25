..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Text Encoding; Reference
    single: String Converter
    single: String Decode Buffer
    single: Text
    single: Base-N
    single: Base16
    single: Base32
    single: Base64
    single: PEM
    single: Punycode
    single: IDNA2008
    single: Internationalized Domain Name

****************************
Text Encoding and Conversion
****************************

String Converter
================

Introduction
------------

``StringConverter`` is the explicit conversion entry point for Erbsland Core strings, editors, literals and the
supported standard-library string types.
It keeps conversion helpers out of the core string classes while still allowing concise call sites.

.. code-block:: cpp

    auto text = el::StringConverter{std::string_view{"Hello"}}.toString();
    auto utf16 = el::StringConverter{text}.toStdU16String();
    auto converted = el::StringConverter{utf16}.toString();

``toString()`` returns a read-only UTF-8 value.
It aliases compatible Core storage in tolerant mode and owns newly converted storage.
Every conversion method accepts :cpp:enum:`EncodingMode <erbsland::text::EncodingMode>`, defaulting to ``Tolerant``.

Extension libraries can support additional source types by specializing
:cpp:struct:`StringConverterTraits <erbsland::text::StringConverterTraits>`.

Encoding Mode
~~~~~~~~~~~~~

:cpp:enum:`EncodingMode <erbsland::text::EncodingMode>` controls how encoding errors are handled by
``StringConverter``.
``Tolerant`` is the default and replaces malformed input with Unicode replacement characters when transcoding.
Compatible representations may instead be copied unchanged without validation.
``Strict`` throws :cpp:class:`EncodingError <erbsland::text::EncodingError>` when malformed input is encountered.
Use strict mode when a conversion must also validate its source, including an existing Erbsland Core string.

String Decoder
~~~~~~~~~~~~~~

:cpp:class:`StringDecoder <erbsland::text::StringDecoder>` decodes
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` or
:cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>` input into Erbsland Core strings.

.. code-block:: cpp

    auto text = el::StringDecoder{bytes}.decode(el::StringEncoding::Utf8);
    auto utf16 = el::StringDecoder{bytes}.toU16String(el::StringEncoding::Utf16);
    el::StringDecoder{bytes}.validateOrThrow(el::StringEncoding::Utf8);

The decoder accepts :cpp:class:`StringEncoding <erbsland::text::StringEncoding>`,
:cpp:enum:`StringBomMode <erbsland::text::StringBomMode>`, and
:cpp:enum:`EncodingMode <erbsland::text::EncodingMode>` for all target string widths.
Only an encoded ``U+FEFF`` signature at the start of the byte input is interpreted as a BOM.
A repeated or embedded ``U+FEFF`` is invalid content and follows ``EncodingMode``; it is never returned as a character
in the decoded string.

Use ``validateOrThrow()`` when encoded bytes must be checked without constructing a decoded string.
It applies strict validation together with the selected encoding and BOM policy, traverses the input once, and allocates
no output storage.

String Encoder
~~~~~~~~~~~~~~

``StringEncoder`` encodes Erbsland Core strings and editors into
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` data or directly into a
:cpp:class:`RingBuffer <erbsland::mem::RingBuffer>`.
Use ``encodedLength()`` to calculate the exact byte length without allocating an intermediate byte block.
The ``encodeTo()`` method reserves the complete output and commits it atomically; insufficient capacity leaves readable
ring data unchanged.
The encoder is intentionally limited to Erbsland Core text sources.

.. code-block:: cpp

    auto bytes = el::StringEncoder{text}.encode(el::StringEncoding::Utf8, el::StringBomMode::Reject);
    auto byteLength = el::StringEncoder{text}.encodedLength(el::StringEncoding::Utf16);
    auto result = el::StringEncoder{text}.encodeTo(ring, el::StringEncoding::Utf16);

Each standalone ``encode()`` or ``encodeTo()`` call applies its requested BOM policy independently, including calls for
empty text that produce only a BOM.
Stateful output streams suppress the BOM after their first successful write.
Explicit BOM output is generated as an encoding signature.
When the source and target representations match, the encoder copies the native units directly without a validation pass
because Erbsland Core strings are assumed to contain valid text.
When transcoding is necessary, malformed source sequences become Unicode replacement characters.
Call the matching ``isValidUtf8()``, ``isValidUtf16()``, or ``isValidUtf32()`` method before encoding when invalid
internal text must be detected.
An ordinary ``Char{0xFEFF}`` in source content is never interpreted as a signature; during transcoding it becomes a
replacement character like other invalid content.

``StringEncoder`` accepts the read-only string and editor types for all three supported string widths.

String Kind
~~~~~~~~~~~

:cpp:enum:`StringKind <erbsland::text::StringKind>` selects the concrete string encoding used by generic text
construction APIs.
Use it when user code should decide whether a result is built as UTF-8, UTF-16, or UTF-32 text.

Usage ^^^^^

Pass :cpp:enum:`StringKind <erbsland::text::StringKind>` to
:cpp:class:`AnyStringBuilder <erbsland::text::AnyStringBuilder>` when you need an empty builder for a specific output
encoding.

.. code-block:: cpp

    auto utf8 = el::AnyStringBuilder{el::StringKind::U8};
    auto utf16 = el::AnyStringBuilder{el::StringKind::U16};
    auto utf32 = el::AnyStringBuilder{el::StringKind::U32};

Any-Width String Values
~~~~~~~~~~~~~~~~~~~~~~~

:cpp:class:`AnyString <erbsland::text::AnyString>` stores a read-only UTF-8, UTF-16 or UTF-32 string without changing
its width.
It can be compared directly with another ``AnyString``, a width-specific Core string or editor, or any ``_el`` string
literal.
Comparisons are lexicographical by decoded code point and do not create converted strings.
Malformed encoded units are compared as :cpp:func:`Char::replacement() <erbsland::text::Char::replacement>`.

.. code-block:: cpp

    auto pattern = el::AnyString{u"[a-z]+"_el};

    if (pattern == "[a-z]+"_el) {
        // The UTF-16 pattern is compared directly with the UTF-8 literal.
    }

An empty ``AnyString`` has no selected width.
It compares equal to empty strings of every width and sorts before any non-empty string.

Standalone Conversion Helpers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``toString()`` overloads convert common text, numeric, boolean, and ordering values into the common
:cpp:type:`String <erbsland::text::String>` type.
Boolean conversion accepts :cpp:class:`BooleanFormat <erbsland::text::BooleanFormat>`.
The ``std::strong_ordering`` overload returns ``less``, ``equal``, or ``greater``.

String Decode Buffer
====================

Introduction
------------

``StringDecodeBuffer`` incrementally decodes byte chunks into Erbsland Core strings.
It is useful for file, terminal, and network input where byte chunks can split an encoded code point.
It is backed by :cpp:class:`ByteBuffer <erbsland::mem::ByteBuffer>` and can operate in ordinary or sensitive mode.

The buffer keeps incomplete trailing code points pending until more bytes are written or ``finish()`` is called.
Use ``peek...()`` methods to inspect decoded text without consuming bytes, and ``take...()`` methods to consume the
bytes that produced the returned complete characters.
Use ``readChar()`` to consume a single decoded character without constructing a string, and ``takeStringLine()`` to
consume decoded UTF-8 text up to and including the next LF character.

For the output direction, use ``StringEncoder::encodedLength()`` and ``StringEncoder::encodeTo()`` with a
:cpp:class:`RingBuffer <erbsland::mem::RingBuffer>`.
The encoder preflights the complete encoded length and atomically writes text into a bounded byte ring without
materializing a second byte block.

.. code-block:: cpp

    auto buffer = el::text::StringDecodeBuffer{el::ByteLength{4096}, el::StringEncoding::Utf8};
    buffer.write(bytes);
    auto text = buffer.takeString();

Unsafe Write Access
-------------------

``text::impl::UnsafeDecodeBufferAccess`` exposes the first contiguous writable byte span of a ``StringDecodeBuffer``.
It is intended for native read APIs that write directly into caller-provided memory.
After a successful native read, call ``commitWritten()`` with the number of bytes actually written.

Sensitive Decoding
------------------

Enable sensitive decoding with ``setSensitive(true)`` before writing input bytes.
This erases consumed prefixes, malformed sequences, byte-order marks, resets, and the final byte allocation.
UTF-8 results are marked sensitive; UTF-16 and UTF-32 results are ordinary because sensitivity is intentionally limited
to UTF-8 strings.
Disabling sensitive mode securely erases buffered input and resets the decoder.

Base-N Encoding and Decoding
============================

The non-flattened ``erbsland::text::base_n`` namespace provides bulk conversion between
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` and all supported string widths.
The predefined formats cover RFC 4648 Base16, Base32, Base32hex, Base64, and Base64url.
The PEM format factory adds 64-character LF-separated payload wrapping without armor.

Use the explicit namespace because these names are intentionally not added to the flattened ``erbsland`` namespace:

.. code-block:: cpp

    namespace base_n = erbsland::text::base_n;

    const auto encoded = base_n::BaseNEncoder{data, base_n::BaseNFormat::base64()}.toString();
    const auto decoded = base_n::BaseNDecoder{encoded}.toDataOrThrow();

Decoding is strict after configured whitespace is removed.
It rejects unknown characters, malformed or missing required padding, incomplete groups, and non-zero unused bits.
Use :cpp:func:`BaseNDecoder::toData() <erbsland::text::base_n::BaseNDecoder::toData>` when malformed input and size
limit failures should both produce an empty optional.
Use :cpp:func:`BaseNDecoder::toDataOrThrow() <erbsland::text::base_n::BaseNDecoder::toDataOrThrow>` when diagnostics
must distinguish parse failures from decoded-size limits.

Punycode and IDNA2008
=====================

Pure Punycode
-------------

The ``erbsland::text::punycode`` namespace provides the reversible RFC 3492 Bootstring codec.
:cpp:class:`PunycodeEncoder <erbsland::text::punycode::PunycodeEncoder>` accepts Unicode ``String`` input and produces
an ASCII payload, while :cpp:class:`PunycodeDecoder <erbsland::text::punycode::PunycodeDecoder>` performs the reverse
operation.
Default-constructed options select pure Punycode: there is no ``xn--`` prefix handling, domain splitting, normalization,
or character policy.

``encode()`` and ``decode()`` return an empty optional for data-dependent failures.
The ``OrThrow`` variants preserve the exact :cpp:class:`ParseError <erbsland::err::ParseError>` reason.

Strict IDNA2008
---------------

:cpp:class:`PunycodeOptions <erbsland::text::punycode::PunycodeOptions>` can select strict IDNA2008 processing for one
label or a complete domain.
The network factory folds ASCII uppercase, normalizes Unicode to NFC, validates Unicode 17 derived properties,
CONTEXTJ/CONTEXTO and bidi rules, verifies A-label round trips, and enforces DNS byte limits.
It deliberately does not apply UTS #46, compatibility or width mappings, Unicode-wide lowercase mapping, or Unicode dot
substitutions.

An optional :cpp:class:`CharSet <erbsland::text::CharSet>` narrows the canonical Unicode characters accepted after the
IDNA2008 checks.
Dots remain domain separators and are not tested by that filter.

See :doc:`/topics/text_parsing_and_encoding/encoding_internationalized_names` for practical codec and domain examples.

Interface
=========

.. doxygenclass:: erbsland::text::base_n::BaseNDecoder
    :members:
.. doxygenclass:: erbsland::text::base_n::BaseNEncoder
    :members:
.. doxygenclass:: erbsland::text::base_n::BaseNFormat
    :members:
.. doxygenenum:: erbsland::text::base_n::BaseNFormatFlag

.. doxygentypedef:: erbsland::text::base_n::BaseNFormatFlags
.. doxygenclass:: erbsland::text::EncodingError
    :members:
.. doxygenenum:: erbsland::text::EncodingMode
.. doxygenclass:: erbsland::text::punycode::PunycodeDecoder
    :members:
.. doxygenclass:: erbsland::text::punycode::PunycodeEncoder
    :members:
.. doxygenenum:: erbsland::text::punycode::PunycodeMode
.. doxygenclass:: erbsland::text::punycode::PunycodeOptions
    :members:
.. doxygenclass:: erbsland::text::StringConverter
    :members:
.. doxygenclass:: erbsland::text::StringDecodeBuffer
    :members:
.. doxygenclass:: erbsland::text::StringDecoder
    :members:
.. doxygenclass:: erbsland::text::StringEncoder
    :members:
.. doxygenclass:: erbsland::text::StringEncoding
    :members:
.. doxygenenum:: erbsland::text::StringKind

.. doxygenfunction:: erbsland::text::toString(StringKind kind) -> String
.. doxygenfunction:: erbsland::text::toString(const String &value) -> String

.. doxygenfunction:: erbsland::text::toString(bool value, BooleanFormat format = BooleanFormat::defaultFormat()) -> String

.. doxygenfunction:: erbsland::text::toString(std::strong_ordering value) -> String

.. doxygenfunction:: erbsland::text::toString(int8_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String

.. doxygenfunction:: erbsland::text::toString(int16_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String

.. doxygenfunction:: erbsland::text::toString(int32_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String

.. doxygenfunction:: erbsland::text::toString(int64_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String

.. doxygenfunction:: erbsland::text::toString(uint8_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String

.. doxygenfunction:: erbsland::text::toString(uint16_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String

.. doxygenfunction:: erbsland::text::toString(uint32_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String

.. doxygenfunction:: erbsland::text::toString(uint64_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String

.. doxygenfunction:: erbsland::text::toString(float value, FloatFormat format = FloatFormat::defaultFormat()) -> String

.. doxygenfunction:: erbsland::text::toString(double value, FloatFormat format = FloatFormat::defaultFormat()) -> String
.. doxygenclass:: erbsland::text::U16EncodingError
    :members:
.. doxygenclass:: erbsland::text::U32EncodingError
    :members:
.. doxygenclass:: erbsland::text::U8EncodingError
    :members:
