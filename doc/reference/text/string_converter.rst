.. index::
    single: String Converter

****************
String Converter
****************

Introduction
============

``StringConverter`` is the explicit conversion entry point for Erbsland strings, editors, literals and the supported
standard-library string types.
It keeps conversion helpers out of the core string classes while still allowing concise call sites.

.. code-block:: cpp

    auto text = el::StringConverter{std::string_view{"Hello"}}.toString();
    auto utf16 = el::StringConverter{text}.toStdU16String();
    auto converted = el::StringConverter{utf16}.toString();

``toString()`` returns a read-only UTF-8 value.
It aliases compatible Core storage and owns newly converted storage.
All conversion methods accept :cpp:enum:`EncodingErrorMode <erbsland::text::EncodingErrorMode>`.

Extension libraries can support additional source types by specializing
:cpp:struct:`StringConverterTraits <erbsland::text::StringConverterTraits>`.

Encoding Error Mode
-------------------

:cpp:enum:`EncodingErrorMode <erbsland::text::EncodingErrorMode>` controls how encoding errors are handled during
string conversion.

String Decoder
--------------

:cpp:class:`StringDecoder <erbsland::text::StringDecoder>` decodes
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` or
:cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>` input into Erbsland strings.

.. code-block:: cpp

    auto text = el::StringDecoder{bytes}.decode(el::StringEncoding::Utf8);
    auto utf16 = el::StringDecoder{bytes}.toU16String(el::StringEncoding::Utf16);

The decoder accepts :cpp:class:`StringEncoding <erbsland::text::StringEncoding>`,
:cpp:enum:`StringBomMode <erbsland::text::StringBomMode>`, and
:cpp:enum:`EncodingErrorMode <erbsland::text::EncodingErrorMode>` for all target string widths.

String Encoder
--------------

``StringEncoder`` encodes Erbsland strings and editors into
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` data or directly into a
:cpp:class:`RingBuffer <erbsland::mem::RingBuffer>`.
Use ``encodedLength()`` to calculate the exact byte length without allocating an intermediate byte block.
The ``encodeTo()`` method reserves the complete output and commits it atomically; insufficient capacity leaves readable
ring data unchanged.
The encoder is intentionally limited to Erbsland text sources.

.. code-block:: cpp

    auto bytes = el::StringEncoder{text}.encode(el::StringEncoding::Utf8, el::StringBomMode::Reject);
    auto byteLength = el::StringEncoder{text}.encodedLength(el::StringEncoding::Utf16);
    auto result = el::StringEncoder{text}.encodeTo(ring, el::StringEncoding::Utf16);

Each standalone ``encode()`` or ``encodeTo()`` call applies its requested BOM policy independently, including calls for
empty text that produce only a BOM.
Stateful output streams suppress the BOM after their first successful write.

Extension libraries can support additional Erbsland-compatible source types by specializing
:cpp:struct:`StringEncoderTraits <erbsland::text::StringEncoderTraits>`.

String Kind
-----------

:cpp:enum:`StringKind <erbsland::text::StringKind>` selects the concrete string encoding used by generic text
construction APIs.
Use it when user code should decide whether a result is built as
:cpp:class:`U8StringEditor <erbsland::text::U8StringEditor>`,
:cpp:class:`U16StringEditor <erbsland::text::U16StringEditor>`, or :cpp:class:`U32StringEditor <erbsland::text::U32StringEditor>`.

Usage
~~~~~

Pass :cpp:enum:`StringKind <erbsland::text::StringKind>` to
:cpp:class:`AnyStringBuilder <erbsland::text::AnyStringBuilder>` when you need an empty builder for a specific output
encoding.

.. code-block:: cpp

    auto utf8 = el::AnyStringBuilder{el::StringKind::U8};
    auto utf16 = el::AnyStringBuilder{el::StringKind::U16};
    auto utf32 = el::AnyStringBuilder{el::StringKind::U32};

Standalone Conversion Helpers
-----------------------------

The ``toString()`` overloads convert common text, numeric, boolean, and ordering values into the common
:cpp:type:`String <erbsland::text::String>` type.
Boolean conversion accepts :cpp:class:`BooleanFormat <erbsland::text::BooleanFormat>`.
The ``std::strong_ordering`` overload returns ``less``, ``equal``, or ``greater``.

Interface
=========

.. doxygenclass:: erbsland::text::AnyString
    :members:
.. doxygenclass:: erbsland::text::AnyStringEditor
    :members:
.. doxygenclass:: erbsland::text::EncodingError
    :members:
.. doxygenenum:: erbsland::text::EncodingErrorMode
.. doxygenclass:: erbsland::text::StringConverter
    :members:
.. doxygenclass:: erbsland::text::StringDecoder
    :members:
.. doxygenclass:: erbsland::text::StringEncoder
    :members:
.. doxygenclass:: erbsland::text::StringEncoding
    :members:
.. doxygenenum:: erbsland::text::StringKind
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
